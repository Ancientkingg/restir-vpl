#pragma once

#include <glm/glm.hpp>

class Photon {
public:
	glm::vec3 position; // Position of the photon in 3D space
	glm::vec3 direction; // Direction of the photon
	glm::vec3 color; // Color of the photon (RGB)
	glm::vec3 intensity; // Intensity of the photon

	inline float operator[](int i) const { return position[i]; }

	Photon();
	Photon(glm::vec3 position, glm::vec3 direction, glm::vec3 color, glm::vec3 intensity);
	Photon(glm::vec3 position, glm::vec3 direction, glm::vec3 color);
};
