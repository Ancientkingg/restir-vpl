#pragma once

#include <vector>
#include <glm/glm.hpp>
#include <string>

void save_image(const std::vector<std::vector<glm::vec3>>& pixels, const std::string& filename);
