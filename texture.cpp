#include "texture.hpp"

#include <glm/glm.hpp>
#include <cstdlib>
#include <string>
#include <iostream>
#include <chrono>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_FAILURE_USERMSG
#include "lib/stb_image.h"

#include "interval.hpp"

SolidColor::SolidColor() {}

SolidColor::SolidColor(glm::vec3 c) : color_value(c) {}

SolidColor::SolidColor(float red, float green, float blue) : SolidColor(glm::vec3(red, green, blue)) {}

glm::vec3 SolidColor::value(double u, double v, const glm::vec3& p) const {
	return color_value;
}

Image::Image() {}

Image::Image(const char* image_filename) {
	auto filename = std::string(image_filename);
	auto imagedir = std::string("./objects");

	if (load(imagedir + "/" + image_filename)) return;

	std::clog << "ERROR: Could not load image file '" << image_filename << "'.\n";
}

Image::~Image() {
	delete[] bdata;
	STBI_FREE(fdata);
}

bool Image::load(const std::string& filename) {
	std::clog << "Loading image file '" << filename << "'..." << std::endl;
	auto start = std::chrono::high_resolution_clock::now();


	auto n = bytes_per_pixel;
	fdata = stbi_loadf(filename.c_str(), &image_width, &image_height, &n, bytes_per_pixel);
	if (fdata == nullptr) {
		std::cerr << "ERROR: Could not load image file '" << filename << "': " << stbi_failure_reason() << std::endl;
		return false;
	}

	bytes_per_scanline = bytes_per_pixel * image_width;
	convert_to_bytes();

	auto stop = std::chrono::high_resolution_clock::now();
	std::clog << "Loading image file '" << filename << "' took " 
		<< std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count()
		<< " milliseconds" << std::endl;

	return true;
}

int Image::width() const {
	return image_width;
}

int Image::height() const {
	return image_height;
}

const uint8_t* Image::data(int x, int y) const {
	static uint8_t magenta[] = { 255, 0, 255 };
	if (bdata == nullptr) {
		std::cerr << "ERROR: Image data is not available." << std::endl;
		return magenta;
	}

	x = clamp(x, 0, image_width);
	y = clamp(y, 0, image_height);

	int index = y * bytes_per_scanline + x * bytes_per_pixel;
	return bdata + index;
}

int Image::clamp(int x, int low, int high) {
	if (x < low) return low;
	if (x >= high) return high - 1;
	return x;
}

uint8_t Image::float_to_byte(float value) {
	if (value <= 0.0f) return 0;
	if (1.0f <= value) return 255;
	return static_cast<uint8_t>(value * 256.0);
}

void Image::convert_to_bytes() {
	int total_bytes = image_width * image_height * bytes_per_pixel;
	bdata = new uint8_t[total_bytes];

	auto* bptr = bdata;
	auto* fptr = fdata;

	for (size_t i = 0; i < total_bytes; i++, fptr++, bptr++) {
		*bptr = float_to_byte(*fptr);
	}
}

ImageTexture::ImageTexture(const char* filename) : image(filename) {}



glm::vec3 ImageTexture::value(double u, double v, const glm::vec3& p) const {
	if (image.height() <= 0) return glm::vec3(0.0f, 1.0f, 1.0f);

	u = Interval(0, 1).clamp(u);
	v = 1.0 - Interval(0, 1).clamp(v);

	auto i = static_cast<int>(u * image.width());
	auto j = static_cast<int>(v * image.height());
	auto pixel = image.data(i, j);
	
	float color_scale = 1.0f / 255.0f;

	return color_scale * glm::vec3(pixel[0], pixel[1], pixel[2]);
}