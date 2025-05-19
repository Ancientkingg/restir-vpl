#include "photon.hpp"

#include <glm/glm.hpp>

Photon::Photon() : position(0, 0, 0), direction(0, 0, 1), color(1,1,1), intensity(1.0f) {}

Photon::Photon(glm::vec3 position, glm::vec3 direction, glm::vec3 color, glm::vec3 intensity) : 
	position(position), direction(direction), color(color), intensity(intensity) {}

Photon::Photon(glm::vec3 position, glm::vec3 direction, glm::vec3 color) :
	position(position), direction(direction), color(color), intensity(1.0f) {}