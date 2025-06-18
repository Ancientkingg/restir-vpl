#include "render.hpp"

#include <glm/glm.hpp>
#include <random>
#include <chrono>
#include <iostream>
#include <atomic>
#include <thread>
#include <omp.h>

#include "camera.hpp"
#include "world.hpp"
#include "restir.hpp"
#include "shading.hpp"

#define EPS 0.001f
#define M_PI 3.14159265358979323846f

thread_local std::mt19937 rng_pt(std::random_device{}());
std::uniform_real_distribution<float> dist_pt(0.0f, 1.0f);

static glm::vec3 pathtrace_ray(Ray& ray, World& world, int depth, glm::vec3 throughput, std::vector<std::weak_ptr<PointLight>>& lights) {
    if (depth > MAX_RAY_DEPTH)
        return glm::vec3(0.0f);

    HitInfo hit;
    if (!world.intersect(ray, hit)) {
        if (depth == 0) {
            return sky_color(ray.direction());
        }
        else {
            return glm::vec3(0.0f);
        }
    }

	auto material = hit.mat_ptr.lock();

	glm::vec3 L = glm::vec3(0.0f);

    // Emitted light
    if (material->emits_light()) {
        L += material->evaluate(hit, -ray.direction());
        return L;
    }

    // Direct lighting
    glm::vec3 P = hit.r.at(hit.t);
    glm::vec3 N = hit.triangle.normal(hit.uv);
    size_t nLights = lights.size();
	glm::vec3 L_direct = glm::vec3(0.0f);

	if (nLights == 0) {
		// No lights, return accumulated light
		return L;
	}

    std::uniform_int_distribution<size_t> lightDist(0, nLights - 1);
    size_t idx = lightDist(rng_pt);
    auto light = lights[idx].lock();

    glm::vec3 toL = light->position - P;
    const float _dist2 = glm::dot(toL, toL);
    const float dist_simple = sqrtf(_dist2);

    constexpr float _r = 3.0f;
    constexpr float r2 = _r * _r;
    const float dist2 = (_dist2 + r2 + dist_simple * sqrtf(_dist2 + r2)) / 2.0f;

    glm::vec3 L_dir = toL / dist_simple;

    glm::vec3 fr = material->evaluate(hit, L_dir);
    

    Ray shadow_ray = Ray(P + EPS * L_dir, L_dir);
    if (!world.is_occluded(shadow_ray, dist_simple - 0.1f)) {
        
        glm::vec3 Li = (light->intensity * light->c) / dist2;
        float cos_theta = fmax(glm::dot(N, L_dir), 0.0f);

        L_direct = fr * cos_theta * Li * float(nLights);
    }

    L += L_direct * throughput;

    Ray scattered;
    glm::vec3 attenuation;
    float pdf;
    if (!material->scatter(ray, hit, attenuation, scattered, pdf))
            return L;

    if (pdf == 0.0f) {
    	// If pdf is zero, we cannot continue the path, return the accumulated light
    	return L;
    }

	float cos_theta = fmax(glm::dot(N, scattered.direction()), 0.0f);
    throughput *= fr * cos_theta / pdf;


    // Russian roulette
    float rr = glm::clamp(
        std::max({ throughput.r, throughput.g, throughput.b }),
        0.05f,
        0.95f
    );

    if (dist_pt(rng_pt) <= rr) {
        throughput /= rr;

        // Recursive call for indirect lighting
        glm::vec3 indirect = pathtrace_ray(scattered, world, depth + 1, throughput, lights);
        L += throughput * indirect;
    }


    return L;
}

std::vector<std::vector<glm::vec3>> pathtrace(RenderInfo& info) {
    auto rays = info.cam.generate_rays_for_frame();
    auto lights = info.world.get_lights();

    std::vector<std::vector<glm::vec3> > colors = std::vector<std::vector<glm::vec3> >(
        info.cam.image_height, std::vector<glm::vec3>(info.cam.image_width, glm::vec3(0.0f)));

    int total_pixels = info.cam.image_height * info.cam.image_width;

    //#pragma omp parallel for
    for (int i = 0; i < rays.size(); i++) {
        for (int j = 0; j < rays[i].size(); j++) {
            Ray& ray = rays[i][j];
            glm::vec3 color = pathtrace_ray(ray, info.world, 0, glm::vec3(1.0f), lights);
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