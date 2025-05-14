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

Reservoir::Reservoir() : sample(glm::vec3(0.0), glm::vec3(0.0), glm::vec3(0.0), glm::vec3(0.0), 0), sample_pos(0, 0, 0), M(0),
W(0), phat(0) {
}

SamplerResult::SamplerResult() : light_point(0.0), light_dir(0.0),
light(glm::vec3(0.0), glm::vec3(0.0), glm::vec3(0.0), glm::vec3(0.0), 0) {
}


void Reservoir::update(const TriangularLight& new_sample, const glm::vec3 sample_point, const double w_i, const double n_phat) {
	M++;
	if (W == 0) {
		W = w_i;
		sample = new_sample;
		sample_pos = sample_point;
		return;
	}
	W += w_i;
	if (const double p = w_i / W;
		dist(rng) < p) {
		sample = new_sample;
		sample_pos = sample_point;
		phat = n_phat;
	}
	// W = (W + w_i) / M / phat;
}

void Reservoir::merge(const Reservoir &other) {
	// Merge the other reservoir into this one
	if (other.W == 0) return;
	if (W == 0) {
		sample = other.sample;
		sample_pos = other.sample_pos;
		W = other.W;
		M = other.M;
		return;
	}
	M += other.M;
	W += other.W;

	if (const float p = other.W / W;
		dist(rng) < p) {
		sample = other.sample;
		sample_pos = other.sample_pos;
		phat = other.phat;
	}
	// else do nothing
	// W = (W + other.W) / M / phat;
}

void Reservoir::replace(const Reservoir &other) {
	W = other.W;
	M = other.M;
	sample = other.sample;
	sample_pos = other.sample_pos;
	phat = other.phat;
}

void Reservoir::reset() {
	sample = { glm::vec3(0.0), glm::vec3(0.0), glm::vec3(0.0), glm::vec3(0.0), 0 };
	M = 0;
	W = 0;
	phat = 0;
	sample_pos = { 0.0, 0.0, 0.0 };
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
	// Swap the current and previous reservoirs
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
			set_initial_sample(x, y, hi);
			// visibility_check(x, y, hi, world);
			temporal_update(x, y);
		}
	}

	std::vector results(y_pixels, std::vector<SamplerResult>(x_pixels));
	swap_buffers();
#pragma omp parallel for
	for (int y = 0; y < y_pixels; y++) {
		for (int x = 0; x < x_pixels; x++) {
			HitInfo& hi = hit_infos[y * x_pixels + x];
			spatial_update(x, y);
			auto res = current_reservoirs[y * x_pixels + x];
			results[y][x].light_point = res.sample_pos;
			results[y][x].light_dir = normalize(res.sample_pos - hi.r.at(hi.t));
			results[y][x].light = res.sample;
			results[y][x].W = res.W / static_cast<double>(res.M) / res.phat;
		}
	}
	return results;
}

void RestirLightSampler::set_initial_sample(const int x, const int y, const HitInfo& hi) {
	// Create a reservoir for the pixel
	Reservoir& res = current_reservoirs[y * x_pixels + x];
	res.reset();
	// Sample M times from the light sources
	for (int k = 0; k < m; k++) {
		TriangularLight l = pick_light();
		const glm::vec3 sample_point = sample_on_light(l);
		double w; double phat;
		get_light_weight(l, sample_point, hi, &w, &phat);
		res.update(l, sample_point, w, phat);
	}
}

void RestirLightSampler::visibility_check(const int x, const int y, const HitInfo& hi, World& world) {
	// Check the visibility of the light sample
	auto& res = current_reservoirs[y * x_pixels + x];
	// Shadow dir = light sample - hit point
	const glm::vec3 shadow_dir = res.sample_pos - hi.r.at(hi.t);
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

void RestirLightSampler::temporal_update(const int x, const int y) {
	// Update the current reservoir with the previous one
	current_reservoirs[y * x_pixels + x].merge(prev_reservoirs[y * x_pixels + x]);
}

void RestirLightSampler::spatial_update(const int x, const int y) {
	int c = 0;

	Reservoir &current = current_reservoirs[y * x_pixels + x];
	current.replace(prev_reservoirs[y * x_pixels + x]);

	// 2) Gather the 8 neighbors also from prev_reservoirs
	for (int yy = -1; yy <= 1; ++yy) {
		for (int xx = -1; xx <= 1; ++xx) {
			if (xx == 0 && yy == 0) continue;

			const int nx = x + xx;
			if (const int ny = y + yy; nx >= 0 && nx < x_pixels && ny >= 0 && ny < y_pixels) {
				current.merge(prev_reservoirs[ny * x_pixels + nx]);
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

	return static_cast<int>(dist(rng) * cached_num_lights);
}

void RestirLightSampler::get_light_weight(const TriangularLight &light, const glm::vec3 light_point,
														 const HitInfo &hi, double *w, double *phat) const {
	// if not hit
	if (hi.t == 1E30f) {
		return;
	}

	const glm::vec3 hit_point = hi.r.at(hi.t);
	const glm::vec3 light_dir = light_point - hit_point;
	const float cos_theta = glm::dot(glm::normalize(light_dir), hi.triangle.normal(hi.uv));
	const glm::vec3 brdf = hi.mat_ptr->evaluate(hi, light_dir);

	const float target = cos_theta * light.intensity * glm::length(brdf);
	const float source = 1.0f / static_cast<float>(num_lights) / light.area() * (
							 glm::dot(light_dir, light_dir) / abs(glm::dot(
								 light.triangle.normal({0, 0}), glm::normalize(light_dir))));

	*w = std::max(0.f, source / target);
	*phat = source;
}
