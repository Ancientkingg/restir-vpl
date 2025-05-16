#pragma once

#include <vector>
#include <glm/glm.hpp>
#include <string>

void save_png(const std::vector<std::vector<glm::vec3>>& pixels, const std::string& filename);

void save_pfm(const std::vector<std::vector<glm::vec3>>& pixels, const std::string& file_path);

