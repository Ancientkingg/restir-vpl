#pragma once

#include <glm/glm.hpp>

#include "world.hpp"

class Photon {
public:
	glm::vec3 position; // Position of the photon in 3D space
	glm::vec3 direction; // Direction of the photon
	glm::vec3 flux; // Flux of the photon

	inline float operator[](int i) const { return position[i]; }

	Photon();
	Photon(glm::vec3 position, glm::vec3 direction, glm::vec3 color, float intensity);
	Photon(glm::vec3 position, glm::vec3 direction, glm::vec3 color);

	void shoot(World& scene, int max_bounces, size_t& photon_count, const size_t max_photons);
private:
	int bounces = 0; // Number of bounces the photon has made
};
