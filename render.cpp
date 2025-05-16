#include "render.hpp"

#include <glm/glm.hpp>

#include "camera.hpp"
#include "world.hpp"
#include "light_sampler.hpp"
#include "shading.hpp"

std::vector<std::vector<glm::vec3>> raytrace(SamplingMode sampling_mode, ShadingMode render_mode, RenderInfo& info) {

    auto hit_infos = info.cam.get_hit_info_from_camera_per_frame(info.world);

    // send hit infos to ReSTIR
    std::vector<std::vector<SamplerResult>> light_samples_per_ray;
    if (render_mode != RENDER_NORMALS) {
        light_samples_per_ray = info.light_sampler.sample_lights(hit_infos);
    }

    std::vector<std::vector<glm::vec3> > colors = std::vector<std::vector<glm::vec3> >(
        info.cam.image_height, std::vector<glm::vec3>(info.cam.image_width, glm::vec3(0.0f)));

    // loop over hit_infos and light_samples_per_ray at the same time and feed them into the shade
#pragma omp parallel for
    for (int i = 0; i < hit_infos.size(); i++) {
        HitInfo hit = hit_infos[i];
		int j = i / info.cam.image_width;
		int k = i % info.cam.image_width;

        SamplerResult sample;

        if (render_mode != RENDER_NORMALS) {
            sample = light_samples_per_ray[j][k];
        }
        
        glm::vec3 color;
        if (render_mode == RENDER_SHADING) {
            if (sampling_mode == SamplingMode::Uniform) {
                color = shadeUniform(hit, sample, info.world, info.light_sampler);
            }
            else {
				color = shadeRIS(hit, sample, info.world);
            }
        }
        else if (render_mode == RENDER_DEBUG) {
            color = shade_debug(hit, sample, info.world);
        }
        else if (render_mode == RENDER_NORMALS) {
            color = shade_normal(hit, sample, info.world);
        }

        colors[j][k] = color;
    }

    return colors;
}