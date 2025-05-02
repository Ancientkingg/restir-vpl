//
// Created by Rafayel Gardishyan on 02/05/2025.
//

#include <vector>
#include <glm/glm.hpp>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "lib/stb_image_write.h"

#include "lib/glm/glm/common.hpp"
#include "lib/glm/glm/vec3.hpp"

#ifndef IMAGE_WRITER_H
#define IMAGE_WRITER_H

inline void save_image(const std::vector<std::vector<glm::vec3>>& pixels, const std::string& filename) {
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

    stbi_write_png(filename.c_str(), width, height, 3, data.data(), width * 3);
}

#endif //IMAGE_WRITER_H
