#ifndef GROUND_TRUTH_RENDERER_H_
#define GROUND_TRUTH_RENDERER_H_

#include <glm/glm.hpp>
#include <vector>

#include "camera.hpp"
#include "triangular_light.hpp"
#include "world.hpp"

float error_per_pixel(glm::vec3 truth, glm::vec3 test) {
    // calculate the mean squared error
    float error = 0.0f;
    error += (truth.x - test.x) * (truth.x - test.x);
    error += (truth.y - test.y) * (truth.y - test.y);
    error += (truth.z - test.z) * (truth.z - test.z);
    return error / 3.0f;
}

float mse(std::vector<std::vector<glm::vec3>> truth,
          std::vector<std::vector<glm::vec3>> test) {
    // check if the two vectors have the same size
    if (truth.size() != test.size()) {
        return -1;
    }
    for (int i = 0; i < truth.size(); i++) {
        if (truth[i].size() != test[i].size()) {
            return -1;
        }
    }
    // calculate the mean squared error
    float error = 0.0f;
    for (int i = 0; i < truth.size(); i++) {
        for (int j = 0; j < truth[i].size(); j++) {
            error += error_per_pixel(truth[i][j], test[i][j]);
        }
    }

    error /= (truth.size() * truth[0].size());
    return error;
}

std::vector<std::vector<glm::vec3>> render_ground_truth(World world, Camera cam,
                                                        int sample_count) {
    auto hit_infos = cam.get_hit_info_from_camera_per_frame(world);

    std::vector<std::vector<glm::vec3>> colors =
        std::vector<std::vector<glm::vec3>>(
            cam.image_height,
            std::vector<glm::vec3>(cam.image_width, glm::vec3(0.0f)));

    constexpr float EPS = 1e-4f;

    auto lights = world.get_triangular_lights();

    // loop over the pixels and lights
    for (int i = 0; i < cam.image_height; i++) {
        std::cout << i << std::endl;
        // #pragma omp parallel for
        for (int j = 0; j < cam.image_width; j++) {
            HitInfo hit = hit_infos[i * cam.image_width + j];
            if (hit.t == 1E30f) {
                colors[i][j] = glm::vec3(0.0f, 1.0f, 0.0f);
                continue;
            }

            for (auto& light : lights) {
                glm::vec3 total_light_contribution{0.0f};
                for (int k = 0; k < sample_count; k++) {
                    glm::vec3 P = hit.r.at(hit.t);  // shading point
                    glm::vec3 light_pos = sample_on_light(light);
                    glm::vec3 light_dir = light_pos - P;
                    glm::vec3 L = glm::normalize(light_dir);
                    Ray light_ray(P + EPS * L, L);
                    HitInfo light_hit;

                    // check visibility
                    if (world.is_occluded(
                            light_ray, glm::length(light_dir) - EPS * 2.0f)) {
                        continue;
                    }

                    glm::vec3 V = hit.r.direction();  // view direction

                    // 1) Compute cosines at shading point and on the light
                    float cosN = glm::dot(hit.triangle.normal(hit.uv), L);
                    float cosL = glm::dot(light_hit.triangle.normal(light_hit.uv), -L);
                    // skip back‑facing or grazing samples
                    // if (cosN <= 0.0f || cosL <= 0.0f) continue;

                    // // 2) Geometry term G = (n·L)(nₗ·(−L)) / r²
                    float r2 = glm::dot(light_dir, light_dir);
                    float G = fabs(cosN * cosL) / r2;

                    // // 3) PDF of uniform‐area sampling on this triangle
                    // // assuming light.area stores the triangle’s area:
                    float pdf = light.pdf;

                    // // 4) Evaluate the BSDF f_r(ωᵢ,ωₒ) at P
                    // //    you’ll need a function that given the hit info and
                    // //    incoming L returns the BRDF value.
                    glm::vec3 fr = hit.mat_ptr->evaluate(hit, L);

                    // // 5) Emitted radiance from the light at that sample
                    // point
                    glm::vec3 Le = light.c * light.intensity;

                    // 6) Accumulate the Monte Carlo estimate
                    //    we’re sampling N points per light uniformly, so weight
                    //    by 1/pdf and 1/N
                    total_light_contribution += fr * Le * G / pdf;
                }
                colors[i][j] +=
                    total_light_contribution * (1.0f / sample_count);
            }
        }
    }

    std::cout << "Finished rendering ground truth" << std::endl;

    return colors;
}

#endif  // GROUND_TRUTH_RENDERER_H_