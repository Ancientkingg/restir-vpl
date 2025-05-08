#include "render.hpp"

#include <glm/glm.hpp>

#include "camera.hpp"
#include "world.hpp"
#include "light_sampler.hpp"
#include "shading.hpp"

std::vector<std::vector<glm::vec3>> raytrace(ShadingMode render_mode, RenderInfo& info) {

    // potentially update camera position
    std::vector<Material*> mats = info.world.get_materials();
    tinybvh::BVH& bvh = info.world.bvh();
    auto hit_infos = info.cam.get_hit_info_from_camera_per_frame(bvh, mats);

    // send hit infos to ReSTIR
    auto light_samples_per_ray = info.light_sampler.sample_lights(hit_infos);

    std::vector<std::vector<glm::vec3> > colors = std::vector<std::vector<glm::vec3> >(
        info.cam.image_height, std::vector<glm::vec3>(info.cam.image_width, glm::vec3(0.0f)));

    // loop over hit_infos and light_samples_per_ray at the same time and feed them into the shade
#pragma omp parallel for
    for (int j = 0; j < hit_infos.size(); j++) {
        if (hit_infos[j].empty()) continue; // No hits for this
        for (int i = 0; i < hit_infos[0].size(); i++) {
            HitInfo hit = hit_infos[j][i];
            SamplerResult sample = light_samples_per_ray[j][i];

            float pdf = sample.light.pdf;

            glm::vec3 color;
            if (render_mode == RENDER_SHADING) {
                color = shade(hit, sample, pdf, info.world);
            }
            else if (render_mode == RENDER_DEBUG) {
                color = shade_debug(hit, sample, pdf, info.world);
            }
            else if (render_mode == RENDER_NORMALS) {
                color = shade_normal(hit, sample, pdf, info.world);
            }

            colors[j][i] += color;
        }
    }

    return colors;
}