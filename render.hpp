#pragma once

#include <glm/vec3.hpp>

#include "camera.hpp"
#include "world.hpp"
#include "light_sampler.hpp"
#include "shading.hpp" 

struct RenderInfo {
    Camera& cam;
    World& world;
    RestirLightSampler& light_sampler;
};

std::vector<std::vector<glm::vec3>> raytrace(ShadingMode render_mode, RenderInfo& info);