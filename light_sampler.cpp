#include "light_sampler.hpp"

#include <glm/vec3.hpp>
#include <vector>
#include <array>
#include <span>
#include <random>

#include "world.hpp"
#include "ray.hpp"
#include "triangular_light.hpp"
#include "hit_info.hpp"


thread_local std::mt19937 rng(std::random_device{}());
std::uniform_real_distribution<float> dist(0.0f, 1.0f);

SampleInfo::SampleInfo() : light(glm::vec3(0.0), glm::vec3(0.0), glm::vec3(0.0), glm::vec3(0.0), 0), light_point(0.0f) {
}

SampleInfo::SampleInfo(const TriangularLight& light, const glm::vec3& light_point) : light(light), light_point(light_point) {
}

Reservoir::Reservoir() : M(0),
w_sum(0), phat(0), y(), W(0) {
}

SamplerResult::SamplerResult() : light_point(0.0), light_dir(0.0),
light(glm::vec3(0.0), glm::vec3(0.0), glm::vec3(0.0), glm::vec3(0.0), 0), W(0) {
}


void Reservoir::update(const SampleInfo x_i, const double w_i, const double n_phat) {
	w_sum = w_sum + w_i;
	M = M + 1;
	if (dist(rng) < w_i / w_sum) {
		y = x_i;
		phat = n_phat;
	}
}

Reservoir Reservoir::merge(const Reservoir& r1, const Reservoir& r2) {
	// Merge the other reservoir into this one
	Reservoir s;

	s.update(r1.y, r1.phat * r1.W * r1.M, r1.phat);
	s.update(r2.y, r2.phat * r2.W * r2.M, r2.phat);

	s.M = r1.M + r2.M;
	s.W = 1.0 / s.phat * ((1.0 / s.M) * s.w_sum);

	// check if W is NaN or infinity
	//if (std::isnan(s.W) || std::isinf(s.W)) {
	//	s.W = 0;
	//}

	return s;
}

void Reservoir::replace(const Reservoir &other) {
	W = other.W;
	M = other.M;
	y = other.y;
	phat = other.phat;
}

void Reservoir::reset() {
	y = SampleInfo();
	M = 0;
	W = 0;
	w_sum = 0;
	phat = 0;
}

RestirLightSampler::RestirLightSampler(const int x, const int y,
	std::vector<TriangularLight>& lights_vec) : x_pixels(x), y_pixels(y) {
	prev_reservoirs = std::vector(y * x, Reservoir());
	current_reservoirs = std::vector(y * x, Reservoir());
	num_lights = static_cast<int>(lights_vec.size());
	lights = lights_vec.data();
}

void RestirLightSampler::reset() {
	// Reset the reservoirs
	for (int y = 0; y < y_pixels; y++) {
		for (int x = 0; x < x_pixels; x++) {
			current_reservoirs[y * x_pixels + x].reset();
			prev_reservoirs[y * x_pixels + x].reset();
		}
	}
}

std::vector<std::vector<SamplerResult> > RestirLightSampler::sample_lights(std::vector<HitInfo> hit_infos) {
	if (num_lights == 0) {
		return std::vector(y_pixels, std::vector<SamplerResult>(x_pixels));
	}
	// Swap the current and previous reservoirs: The previous current frame is now the previous frame
	swap_buffers();

	// For every pixel:
	// 1. Sample M times from the light sources; Choose one sample (reservoir)
	// 2. Check visibility of the light sample
	// 3. Temporal update - update the current reservoir with the previous one
	// 4. Spatial update - update the current reservoir with the neighbors
	// 5. Return the sample in the current reservoir

#pragma omp parallel for
	for (int y = 0; y < y_pixels; y++) {
		for (int x = 0; x < x_pixels; x++) {
			HitInfo& hi = hit_infos[y * x_pixels + x];

			Reservoir& current = current_reservoirs[y * x_pixels + x];
			Reservoir& prev = prev_reservoirs[y * x_pixels + x];

			set_initial_sample(x, y, hi);


			if (sampling_mode != SamplingMode::Uniform && sampling_mode != SamplingMode::RIS) {
				current_reservoirs[y * x_pixels + x] = temporal_update(current, prev);
			}
		}
	}

	std::vector results(y_pixels, std::vector<SamplerResult>(x_pixels));
	if (sampling_mode != SamplingMode::Uniform && sampling_mode != SamplingMode::RIS) {
		//swap_buffers();
	}
#pragma omp parallel for
	for (int y = 0; y < y_pixels; y++) {
		for (int x = 0; x < x_pixels; x++) {
			HitInfo& hi = hit_infos[y * x_pixels + x];
			if (sampling_mode != SamplingMode::Uniform && sampling_mode != SamplingMode::RIS) {
				//spatial_update(x, y);
			}
			Reservoir& res = current_reservoirs[y * x_pixels + x];
			results[y][x].light_point = res.y.light_point;
			results[y][x].light_dir = normalize(res.y.light_point - hi.r.at(hi.t));
			results[y][x].light = res.y.light;
			results[y][x].W = res.W;
		}
	}
	return results;
}

void RestirLightSampler::set_initial_sample(const int x, const int y, const HitInfo& hi) {
	// Create a reservoir for the pixel
	Reservoir& r = current_reservoirs[y * x_pixels + x];
	r.reset();
	// Sample M times from the light sources
	for (int k = 0; k < m; k++) {
		TriangularLight l = pick_light();
		const glm::vec3 sample_point = sample_on_light(l);

		SampleInfo sample = SampleInfo(l, sample_point);

		double W, phat;
		get_light_weight(sample, hi, W, phat);
		r.update(sample, W, phat);

		r.W = 1.0 / phat * ((1.0 / r.M) * r.w_sum);

		if (sampling_mode == SamplingMode::Uniform)
			break;
	}
}

void RestirLightSampler::visibility_check(const int x, const int y, const HitInfo& hi, World& world) {
	// Check the visibility of the light sample
	auto& res = current_reservoirs[y * x_pixels + x];
	// Shadow dir = light sample - hit point
	const glm::vec3 shadow_dir = res.y.light_point - hi.r.at(hi.t);
	// Create a ray from the hit point to the light sample
	Ray shadow(hi.r.at(hi.t), normalize(shadow_dir));
	// Check if the ray intersects with the scene
	HitInfo shadow_hit;
	world.intersect(shadow, shadow_hit);
	// If the ray intersects with the scene, discard the sample
	if (shadow_hit.t > 1e-3f && shadow_hit.t < length(shadow_dir)) {
		res.reset();
	}
}

Reservoir RestirLightSampler::temporal_update(const Reservoir& current, const Reservoir& prev) {
	// Update the current reservoir with the previous one	
	return Reservoir::merge(current, prev);
}

void RestirLightSampler::spatial_update(const int x, const int y) {
	int c = 0;

	Reservoir& current = current_reservoirs[y * x_pixels + x];
	Reservoir& prev = prev_reservoirs[y * x_pixels + x];
	current.replace(prev);

	// 2) Gather the 8 neighbors also from prev_reservoirs
	for (int yy = -1; yy <= 1; ++yy) {
		for (int xx = -1; xx <= 1; ++xx) {
			if (xx == 0 && yy == 0) continue;

			const int nx = x + xx;
			if (const int ny = y + yy; nx >= 0 && nx < x_pixels && ny >= 0 && ny < y_pixels) {
				current = Reservoir::merge(current, prev);
			}
		}
	}
}


void RestirLightSampler::spatial_update(const int x, const int y, const int radius) {
	for (int r = 0; r < radius; r++) {
		spatial_update(x, y);
	}
}

void RestirLightSampler::swap_buffers() {
	// Swap the current and previous reservoirs
	current_reservoirs.swap(prev_reservoirs);
}

[[nodiscard]] TriangularLight RestirLightSampler::pick_light() const {
	// Pick a random light source uniformly (standard, change this if you want to use a different sampling strategy)
	const int index = sampleLightIndex();
	return lights[index];
}

int RestirLightSampler::sampleLightIndex() const {
	// Thread-local pair to track last used upper bound and distribution
	thread_local int cached_num_lights = -1;

	if (cached_num_lights != num_lights) {
		cached_num_lights = num_lights;
	}

	const int out = static_cast<int>(dist(rng) * cached_num_lights);

	//std::cout << "Sampled light index: " << out << std::endl;

	return out;
}

void RestirLightSampler::get_light_weight(const SampleInfo& sample,
														 const HitInfo &hi, double& W, double& phat) const {
	// if not hit
	if (hi.t == 1E30f) {
		W = 0.0;
		phat = 0.0;
		return;
	}

	const glm::vec3 hit_point = hi.r.at(hi.t);
	const glm::vec3 light_dir = sample.light_point - hit_point;
	const glm::vec3 L = glm::normalize(light_dir);
	const float cos_theta = glm::dot(L, hi.triangle.normal(hi.uv));
	const glm::vec3 brdf = hi.mat_ptr->evaluate(hi, L);

	const float geometry_term = glm::dot(light_dir, light_dir) / abs(glm::dot(
		sample.light.triangle.normal({ 0, 0 }), L));

	const float radiance = sample.light.intensity;
	const float target = radiance * glm::length(brdf);

	const float light_choose_pdf = 1.0f / static_cast<float>(num_lights);
	const float light_point_pdf = 1.0f / sample.light.area();

	const float source = light_choose_pdf * light_point_pdf * geometry_term;

	W = std::max(0.f, source / target);
	phat = source;
}