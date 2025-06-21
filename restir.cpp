#include "restir.hpp"

#include <glm/vec3.hpp>
#include <vector>
#include <array>
#include <span>
#include <random>
#include <memory>

#include "constants.hpp"
#include "world.hpp"
#include "ray.hpp"
#include "light.hpp"
#include "hit_info.hpp"


thread_local std::mt19937 rng(std::random_device{}());
std::uniform_real_distribution<float> dist(0.0f, 1.0f);

SampleInfo::SampleInfo() : light(), light_point(0.0f) {
}

SampleInfo::SampleInfo(const std::weak_ptr<PointLight> light, const glm::vec3& light_point) : light(light), light_point(light_point) {
}

Reservoir::Reservoir() : M(0),
w_sum(0.0f), phat(0.0f), y(), W(0.0f) {
}

SamplerResult::SamplerResult() : light_point(0.0f), light_dir(0.0f),
light(), W(0.0f) {
}

//static std::shared_ptr<PointLight> empty_light = std::make_shared<PointLight>(PointLight(glm::vec3(1.0f), 1.0f, glm::vec3(0.0f), glm::vec3(1.0f,0.0f,0.0f)));


bool Reservoir::update(const SampleInfo x_i, const float w_i, const float n_phat) {
	w_sum = w_sum + w_i;
	M = M + 1;
	// Condition for when w_i is 0 and w_sum is also 0 so we get 0/0
	if (dist(rng) < (w_i + 1e-6f) / (w_sum + 1e-6f)) {
		y = x_i;
		phat = n_phat;
		return true;
	}
	return false;
}

static float calculate_reservoir_weight(const float phat, const float M, const float w_sum) {
	if (phat == 0.0f) {
		return 0.0f;
	} else {
		return (1.0f / fmax(phat, 1e-6f)) * ((1.0f / fmax(M, 1e-6f)) * w_sum);
	}
}

void Reservoir::replace(const Reservoir &other) {
	W = other.W;
	M = other.M;
	y = other.y;
	w_sum = other.w_sum;
	phat = other.phat;
}

Reservoir Reservoir::combineReservoirs(const std::span<const Reservoir*>& reservoirs) {
	Reservoir s;
	for (auto& r : reservoirs) {
		s.update(r->y, r->phat * r->W * r->M, r->phat);
	}

	s.M = 0;
	for (auto& r : reservoirs) {
		s.M += r->M;
	}

	s.W = calculate_reservoir_weight(s.phat, s.M, s.w_sum);

	return s;
}

Reservoir Reservoir::combineReservoirsUnbiased(const std::span<const Reservoir*>& reservoirs) {
	Reservoir s;
	for (auto& r : reservoirs) {
		s.update(r->y, r->phat * r->W * r->M, r->phat);
	}

	s.M = 0;
	for (auto& r : reservoirs) {
		s.M += r->M;
	}

	int Z = 0;

	for (auto& r : reservoirs) {
		if (r->phat > 0) {
			Z += r->M;
		}
	}

	// m = 1.0f / Z
	// We pass m to calculate_reservoir_weight, but in there it uses 1/m, so we can just pass Z

	s.W = calculate_reservoir_weight(s.phat, Z, s.w_sum);

	return s;
}



void Reservoir::reset() {
	y = SampleInfo();
	M = 0;
	W = 0.0f;
	w_sum = 0.0f;
	phat = 0.0f;
}

RestirLightSampler::RestirLightSampler(const int x, const int y,
	std::vector<std::weak_ptr<PointLight>>& lights_vec) : x_pixels(x), y_pixels(y) {
	prev_reservoirs = std::vector(y * x, Reservoir());
	current_reservoirs = std::vector(y * x, Reservoir());
	lights = lights_vec;
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

std::vector<std::vector<SamplerResult> > RestirLightSampler::sample_lights(std::vector<HitInfo> hit_infos, World& scene) {
	if (num_lights() == 0) {
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

			auto material = hi.mat_ptr.lock();
			if (hi.t == 1E30f || material->emits_light()) {
				continue;
			}

			Reservoir& current = current_reservoirs[y * x_pixels + x];
			current.reset();

			Reservoir& prev = prev_reservoirs[y * x_pixels + x];

			set_initial_sample(current, hi);
			visibility_check(current, hi, scene);

			if (sampling_mode != SamplingMode::Uniform && sampling_mode != SamplingMode::RIS) {
				prev.M = fmin(M_CAP * current.M, prev.M);
				current_reservoirs[y * x_pixels + x] = temporal_update(current, prev);
			}
		}
	}

	std::vector results(y_pixels, std::vector<SamplerResult>(x_pixels));
	if (sampling_mode != SamplingMode::Uniform && sampling_mode != SamplingMode::RIS) {
		 swap_buffers();
	}
#pragma omp parallel for
	for (int y = 0; y < y_pixels; y++) {
		for (int x = 0; x < x_pixels; x++) {
			if (sampling_mode != SamplingMode::Uniform && sampling_mode != SamplingMode::RIS) {
				 spatial_update(x, y, hit_infos, scene);
			}

			Reservoir& res = current_reservoirs[y * x_pixels + x];
			HitInfo& hi = hit_infos[y * x_pixels + x];

			results[y][x].light_point = res.y.light_point;
			results[y][x].light_dir = normalize(res.y.light_point - hi.r.at(hi.t));
			results[y][x].light = res.y.light;
			results[y][x].W = res.W;
		}
	}
	return results;
}

void RestirLightSampler::set_initial_sample(Reservoir& r, const HitInfo& hi) {
	// Sample M times from the light sources
	for (int k = 0; k < m; k++) {
		float light_choose_pdf;
		auto l = pick_light(light_choose_pdf).lock();

		if (!l) {
			std::cerr << "Error: No light source found!" << std::endl;
			exit(1);
		}

		float light_pos_pdf;
		const glm::vec3 sample_point = l->sample_on_light(light_pos_pdf);

		SampleInfo sample = SampleInfo(l, sample_point);

		float W, phat;
		get_light_weight(sample, hi, W, phat);
		r.update(sample, W, phat);

		if (sampling_mode == SamplingMode::Uniform)
			break;
	}
	//r.W = (1.0f / r.phat) * ((1.0f / r.M) * r.w_sum);
	r.W = calculate_reservoir_weight(r.phat, r.M, r.w_sum);
}

bool RestirLightSampler::is_visible(Reservoir& res, const HitInfo& hi, World& scene) {
	// Check the visibility of the light sample

	// Point of intersection [x]
	const glm::vec3 I = hi.r.at(hi.t);

	// Distance between the light and the intersection point
	const float dist = glm::length(res.y.light_point - I);

	const glm::vec3 L = (res.y.light_point - I) / dist; // Direction to light

	Ray shadow_ray = Ray(I + 0.001f * L, L);
	const bool is_occluded = scene.is_occluded(shadow_ray, dist - 1e-2f);

	return !is_occluded;
}

bool RestirLightSampler::visibility_check(Reservoir& res, const HitInfo& hi, World& scene, bool reset_phat) {
	const bool visible = is_visible(res, hi, scene);

	if (!visible) {
		res.W = 0;
		if (reset_phat) {
			res.phat = 0;
		}
	}

	return visible;
}

Reservoir RestirLightSampler::temporal_update(const Reservoir& current, const Reservoir& prev) {
	std::array<const Reservoir*, 2> refs = { &current, &prev };
	return Reservoir::combineReservoirs(refs);
}

void RestirLightSampler::spatial_update(const int x, const int y, const std::vector<HitInfo>& hit_infos, World& scene) {
	std::vector<const Reservoir*> candidates;
	candidates.push_back(&prev_reservoirs[y * x_pixels + x]);

	const HitInfo& current_hit = hit_infos[y * x_pixels + x];

	const glm::vec3 N = current_hit.triangle.normal(current_hit.uv);

	// Generate neighbours by randomly sampling a NEIGHBOUR_RADIUS radius around the current pixel
	std::array<glm::ivec2, NEIGHBOUR_K> offsets;

	int c = 0;
	while (c < NEIGHBOUR_K) {
		const float phi = dist(rng) * 2.0f * glm::pi<float>();
		const float r = dist(rng) * NEIGHBOUR_RADIUS;

		const int x_offset = r * cosf(phi);
		const int y_offset = r * sinf(phi);

		if (x_offset == 0 && y_offset == 0) {
			// Skip the current pixel
			continue;
		}

		offsets[c] = glm::ivec2(x_offset, y_offset);
		c++;
	}

	for (glm::ivec2& offset : offsets) {
		const int nx = x + offset.x;
		const int ny = y + offset.y;

		const bool x_within_bounds = nx >= 0 && nx < x_pixels;
		const bool y_within_bounds = ny >= 0 && ny < y_pixels;

		if (x_within_bounds && y_within_bounds) {
			const HitInfo& hi = hit_infos[ny * x_pixels + nx];

			// If the neighbour has no hit or is a light source, skip it, since it's reservoir is not valid
			auto material = hi.mat_ptr.lock();
			const bool invalid_sample = hi.t == 1E30f || material->emits_light();

			// Check if the normals are similar
			const glm::vec3 N2 = hi.triangle.normal(hi.uv);
			const bool different_normals = glm::distance(N, N2) > NORMAL_DEVIATION;

			// Check if the hits are not far away from each other
			const float dist = glm::distance(current_hit.r.at(current_hit.t), hi.r.at(hi.t));
			const bool different_t = dist > T_DEVIATION;

			Reservoir& candidate = prev_reservoirs[ny * x_pixels + nx];

			if (!invalid_sample && !different_normals && !different_t) {
				//visibility_check(candidate, current_hit, scene, true);
				if (is_visible(candidate, current_hit, scene)) {
					candidates.push_back(&candidate);
				}
				
			}
		}
	}

	current_reservoirs[y * x_pixels + x] = Reservoir::combineReservoirs(std::span(candidates));
}

void RestirLightSampler::swap_buffers() {
	// Swap the current and previous reservoirs
	current_reservoirs.swap(prev_reservoirs);
}

[[nodiscard]] std::weak_ptr<PointLight> RestirLightSampler::pick_light(float& pdf) const {
	// Pick a random light source uniformly (standard, change this if you want to use a different sampling strategy)
	const int index = sample_light_index();
	pdf = 1.0f / static_cast<float>(num_lights());
	auto out = lights[index];
	return out;
}


int RestirLightSampler::sample_light_index() const {
	// Thread-local pair to track last used upper bound and distribution
	thread_local int cached_num_lights = -1;

	const int n = num_lights();

	if (cached_num_lights != n) {
		cached_num_lights = n;
	}

	const int out = static_cast<int>(dist(rng) * cached_num_lights);

	return out;
}

static float luminance(const glm::vec3& color) {
	return 0.2126f * color.r + 0.7152f * color.g + 0.0722f * color.b;
}

void RestirLightSampler::get_light_weight(const SampleInfo& sample,
														 const HitInfo &hi, float& W, float& phat) const {
	auto light = sample.light.lock();

	// Geometry setup
	const glm::vec3 hit_point = hi.r.at(hi.t);
	const glm::vec3 light_vec = sample.light_point - hit_point;

	const float _dist2 = glm::dot(light_vec, light_vec);
	const float _dist = sqrtf(_dist2);
	constexpr float _r = 3.0f;
	constexpr float _r2 = _r * _r;
#ifdef PL_ATTENUATION
	const float dist2 = (_dist2 + _r2 + _dist * sqrtf(_dist2 + _r2)) / 2.0f;
#else
    const float dist2 = _dist2;
#endif

	const glm::vec3 L = glm::normalize(light_vec);                // Direction to light

	const glm::vec3 N = hi.triangle.normal(hi.uv);                // Surface normal
	const glm::vec3 Nl = light->normal(sample.light_point);       // Light normal

	const float cos_theta_light = glm::dot(Nl, -L);               // Light angle
	const float cos_theta = glm::dot(N, L);                       // Surface angle

	if (cos_theta <= 0.0f) {
		W = 0.0f;
		phat = 0.0f;
		return;
	}

	// BRDF
	auto material = hi.mat_ptr.lock();
	const glm::vec3 fr = material->evaluate(hi, L);               // f_r
	const glm::vec3 Le = light->intensity * light->c;             // L_i

	// Target importance (importance of this sample for the current pixel)
	const float G = cos_theta;
	const float target = luminance(Le * fr * G);

	if (cos_theta_light <= 0.0f) {
		W = 0.0f;
		phat = target;
		return;
	}

	// Source PDF: converting from area to solid angle
	const float light_choose_pdf = 1.0f / static_cast<float>(num_lights());
	const float light_area_pdf = 1.0f / light->area();
	const float source = light_choose_pdf * light_area_pdf * (dist2 / cos_theta_light); // dA → dOmega

	// Final weight and importance
	W = target / source;

	// Clamping is really bad in reality (since it makes the output image biased) but it helps against fireflies
	// FIX: Fix fireflies in a better way that makes the final image not biased.
	//W = fmin(W, 20.0f);
	phat = target; // This is the "target pdf" value used by ReSTIR
}