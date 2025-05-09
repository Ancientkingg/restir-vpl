#pragma once

#include <glm/glm.hpp>
#include <cstdlib>
#include <string>

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

class Image {
public:
	Image();
	Image(const char* filename);
	~Image();

	bool load(const std::string& filename);
	int width() const;
	int height() const;

	const uint8_t* data(int x, int y) const;
private:
	const int bytes_per_pixel = 3;
	float* fdata = nullptr;
	uint8_t* bdata = nullptr;
	int image_width = 0;
	int image_height = 0;
	int bytes_per_scanline = 0;

	static int clamp(int x, int low, int high);
	static uint8_t float_to_byte(float value);

	void convert_to_bytes();

};

class ImageTexture : public Texture {
public:
	ImageTexture(const char* filename);
	

	virtual glm::vec3 value(double u, double v, const glm::vec3& p) const override;
private:
	Image image;
};

