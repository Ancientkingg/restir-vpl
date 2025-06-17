#include "render.hpp"

#include <glm/glm.hpp>
#include <random>

#include "camera.hpp"
#include "world.hpp"
#include "restir.hpp"
#include "shading.hpp"

thread_local std::mt19937 rng_pt(std::random_device{}());
std::uniform_real_distribution<float> dist_pt(0.0f, 1.0f);

static glm::vec3 pathtrace_ray(Ray& ray, World& world, int depth, glm::vec3 throughput) {
    if (depth > MAX_RAY_DEPTH) {
        return glm::vec3(0.0f); // Return black color if max depth is reached
    }

    HitInfo hit;
    bool is_hit = world.intersect(ray, hit);

    if (!is_hit) {
        return sky_color(hit.r.direction());
    }

    // BSDF term
    auto material = hit.mat_ptr.lock();
    auto emitted = material->albedo(hit);
    glm::vec3 L = emitted * throughput;

    // If the material emits light, return the emitted radiance directly
    if (material->emits_light()) {
        return L;
    }

    Ray scattered;
    glm::vec3 attenuation;
    float pdf;
    bool is_scatter = material->scatter(ray, hit, attenuation, scattered, pdf);

    if (!is_scatter) {
        return L;
    }

    const glm::vec3 N = hit.triangle.normal(hit.uv);
    const float cos_theta = glm::dot(N, scattered.direction());
    throughput *= attenuation * (cos_theta / pdf);

    float max_component = std::max(throughput.r, std::max(throughput.g, throughput.b));
    float rr_prob = std::min(max_component, 0.95f); // Prevent long paths

    if (dist_pt(rng_pt) > rr_prob) {
        return L;
    }

    throughput /= rr_prob;

    glm::vec3 indirect = (attenuation * pathtrace_ray(scattered, world, depth + 1, throughput));

    return L + indirect;
}


std::vector<std::vector<glm::vec3>> pathtrace(RenderInfo& info) {
    auto rays = info.cam.generate_rays_for_frame();

    std::vector<std::vector<glm::vec3> > colors = std::vector<std::vector<glm::vec3> >(
        info.cam.image_height, std::vector<glm::vec3>(info.cam.image_width, glm::vec3(0.0f)));

    for (int i = 0; i < rays.size(); i++) {
        for (int j = 0; j < rays[i].size(); j++) {
            Ray& ray = rays[i][j];
            glm::vec3 color = pathtrace_ray(ray, info.world, 0, glm::vec3(1.0f));

            colors[i][j] = color;
        }
    }

    return colors;
}

std::vector<std::vector<glm::vec3>> raytrace(SamplingMode sampling_mode, ShadingMode render_mode, RenderInfo& info) {
    auto hit_infos = info.cam.get_hit_info_from_camera_per_frame(info.world);

    // send hit infos to ReSTIR
    std::vector<std::vector<SamplerResult>> light_samples_per_ray;
    if (render_mode != RENDER_NORMALS) {
        light_samples_per_ray = info.light_sampler.sample_lights(hit_infos, info.world);
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