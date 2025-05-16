#include "image_writer.hpp"

#include <vector>
#include <glm/glm.hpp>
#include <filesystem>
#include <stdio.h>


#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <fstream>

#include "lib/stb_image_write.h"

void save_png(const std::vector<std::vector<glm::vec3>>& pixels, const std::string& filename) {
    int height = pixels.size();
    int width = pixels[0].size();
    std::vector<unsigned char> data(width * height * 3);

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            glm::vec3 color = clamp(pixels[y][x], 0.0f, 1.0f);  // Clamp to [0,1]
            int index = ((height - 1 - y) * width + x) * 3; // Flip vertically
            data[index + 0] = static_cast<unsigned char>(color.r * 255.0f);
            data[index + 1] = static_cast<unsigned char>(color.g * 255.0f);
            data[index + 2] = static_cast<unsigned char>(color.b * 255.0f);
        }
    }

    // Check if the folder exists, if not create it
    std::filesystem::path path = filename;
    std::filesystem::path dir = path.parent_path();
    if (!std::filesystem::exists(dir)) {
        std::filesystem::create_directories(dir);
    }
    // Write the image to a file
    stbi_write_png(filename.c_str(), width, height, 3, data.data(), width * 3);
}



void save_pfm(const std::vector<std::vector<glm::vec3>>& pixels, const std::string& file_path) {
    std::ofstream file(file_path, std::ios::binary);
    if (!file) {
        printf("Failed to open %s for writing. Please check path and permissions.\n", file_path.c_str());
        return;
    }

    int height = pixels.size();
    int width = height > 0 ? pixels[0].size() : 0;



    // Write the PFM header
    file << "PF\n"
            << width << " " << height << "\n"
            <<"-1.0\n";

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            const glm::vec3& pixel = pixels[y][x];
            file.write(reinterpret_cast<const char*>(&pixel.r), sizeof(float));
            file.write(reinterpret_cast<const char*>(&pixel.g), sizeof(float));
            file.write(reinterpret_cast<const char*>(&pixel.b), sizeof(float));
        }
    }

    file.close();
}

 