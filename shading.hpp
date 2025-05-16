#pragma once

#include <glm/glm.hpp>

#include "hit_info.hpp"
#include "light_sampler.hpp" 

enum ShadingMode {
    RENDER_NORMALS,
    RENDER_DEBUG,
    RENDER_SHADING
};

glm::vec3 sky_color(const glm::vec3& direction);

glm::vec3 shade_normal(const HitInfo& hit, const SamplerResult& sample, World& scene);

glm::vec3 shade_debug(const HitInfo& hit, const SamplerResult& sample, World& scene);

glm::vec3 shadeRIS(const HitInfo& hit, const SamplerResult& sample, World& scene);
glm::vec3 shadeUniform(const HitInfo& hit, const SamplerResult& sample, World& scene, RestirLightSampler& sampler);