#include <glm/glm.hpp>

class Texture {
public:
	virtual glm::vec3 value(double u, double v, const glm::vec3& p) const = 0;
};

class SolidColor : public Texture {
public:
	SolidColor() {}
	SolidColor(glm::vec3 c) : color_value(c) {}
	SolidColor(float red, float green, float blue) : SolidColor(glm::vec3(red, green, blue)) {}

	virtual glm::vec3 value(double u, double v, const glm::vec3& p) const override {
		return color_value;
	}

private:
	glm::vec3 color_value;
};