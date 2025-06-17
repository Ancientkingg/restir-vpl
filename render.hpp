#pragma once

#include <glm/vec3.hpp>

#include "camera.hpp"
#include "world.hpp"
#include "restir.hpp"
#include "shading.hpp" 

struct RenderInfo {
    Camera& cam;
    World& world;
    RestirLightSampler& light_sampler;
};

std::vector<std::vector<glm::vec3>> raytrace(SamplingMode sampling_mode, ShadingMode render_mode, RenderInfo& info);

std::vector<std::vector<glm::vec3>> pathtrace(RenderInfo& info);