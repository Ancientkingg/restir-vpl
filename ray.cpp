#include "ray.hpp"

Ray::Ray() {}

Ray::Ray(const glm::vec3& origin, const glm::vec3& direction) : orig(origin), dir(direction) {}

const glm::vec3& Ray::origin() const { return orig; }

const glm::vec3& Ray::direction() const { return dir; }

glm::vec3 Ray::at(float t) const {
	return orig + dir * t;
}