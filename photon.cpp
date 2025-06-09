#include "photon.hpp"

#include <glm/glm.hpp>
#include <iostream>
#include <random>

#include "constants.hpp"
#include "ray.hpp"
#include "hit_info.hpp"

Photon::Photon() : position(0, 0, 0), direction(0, 0, 1), flux(1,1,1) {}

thread_local std::mt19937 rng3(std::random_device{}());
std::uniform_real_distribution<float> dist3(0.0f, 1.0f);

Photon::Photon(glm::vec3 position, glm::vec3 direction, glm::vec3 color, float intensity) : 
	position(position), direction(glm::normalize(direction)), flux(color * intensity) {
	if (std::isnan(intensity) || std::isinf(intensity)) {
		std::cerr << "Error: Photon intensity is NaN or Inf!" << std::endl;
	}
}

Photon::Photon(glm::vec3 position, glm::vec3 direction, glm::vec3 color) :
	position(position), direction(glm::normalize(direction)), flux(color) {}

void Photon::shoot(World& scene, int max_bounces, size_t& photon_count, const size_t max_photons) {

	if (photon_count >= max_photons || bounces >= max_bounces) {
		return; // Stop if we reached the maximum number of photons or bounces
	}

	HitInfo hit_point;
	auto r = Ray(position, direction);
	if (!scene.intersect(r, hit_point)) {
		return; // If the photon does not hit anything, stop
	}

	// Get the triangle that was hit
	const Triangle& triangle = hit_point.triangle;
	const glm::vec3& normal = triangle.normal(hit_point.uv); // Get the normal of the triangle at the hit point
	const auto mat_ptr = hit_point.mat_ptr.lock();

	// Get the hit point
	position = r.at(hit_point.t); // Update the position of the photon to the hit point

	// Place a virtual point light at the hit point slightly offset in the direction of the normal
	glm::vec3 light_position = position + 0.001f * normal; // Offset to avoid self-occlusion

	const float cos_theta = glm::dot(normal, -direction); // Cosine of the angle between the surface normal and the direction
	if (cos_theta <= 0.0f) return; // Ignore if backfacing or grazing

	float pdf_dir;
	glm::vec3 new_dir = mat_ptr->sample_direction(-direction, normal, pdf_dir);

	HitInfo hi;
	const glm::vec3 brdf = mat_ptr.get()->evaluate(hi, -direction);
	flux = flux * brdf * cos_theta / pdf_dir;

	if (!mat_ptr.get()->emits_light()) {
		photon_count++; // Increment the number of photons shot
		scene.spawn_vpl(light_position, normal, flux, 1.0f);
	}

	bounces++; // Increment the number of bounces

	// Russian Roulette
	if (bounces >= MIN_BOUNCES) {
		const float max_flux = fmax(fmax(flux.r, flux.g), flux.b);
		const float rr_prob = glm::clamp(max_flux, 0.05f, 0.95f); // Clamp the probability to avoid too low or too high values

		if (dist3(rng3) > rr_prob) return;
		flux /= rr_prob;
	}

	direction = new_dir;
	shoot(scene, max_bounces, photon_count, max_photons);
}