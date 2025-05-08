#include "texture.hpp"

#include <glm/glm.hpp>

SolidColor::SolidColor() {}

SolidColor::SolidColor(glm::vec3 c) : color_value(c) {}

SolidColor::SolidColor(float red, float green, float blue) : SolidColor(glm::vec3(red, green, blue)) {}

glm::vec3 SolidColor::value(double u, double v, const glm::vec3& p) const {
	return color_value;
}