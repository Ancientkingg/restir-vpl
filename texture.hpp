#pragma once

#include <glm/glm.hpp>

class Texture {
public:
	virtual glm::vec3 value(double u, double v, const glm::vec3& p) const = 0;
};

class SolidColor : public Texture {
public:
	SolidColor();
	SolidColor(glm::vec3 c);
	SolidColor(float red, float green, float blue);

	virtual glm::vec3 value(double u, double v, const glm::vec3& p) const override;

private:
	glm::vec3 color_value;
};