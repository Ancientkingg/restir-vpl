//
// Created by Rafayel Gardishyan on 28/04/2025.
//

#ifndef LIGHT_SAMPLER_H
#define LIGHT_SAMPLER_H
#include "triangular_light.h"
#include "ray.h"
#include <glm/vec3.hpp>

#include "world.hpp"


struct sampler_result {
    glm::vec3 light_point;
    glm::vec3 light_dir;
    triangular_light light;

    sampler_result() : light_point(0.0), light_dir(0.0),
                       light(glm::vec3(0.0), glm::vec3(0.0), glm::vec3(0.0), glm::vec3(0.0), 0) {
    }
};

class reservoir {
public:
    triangular_light sample;
    glm::vec3 sample_pos;
    int M;
    float W;

    reservoir() : sample(glm::vec3(0.0), glm::vec3(0.0), glm::vec3(0.0), glm::vec3(0.0), 0), sample_pos(0, 0, 0), M(0),
                  W(0) {
    }

    void update(const triangular_light &new_sample, const glm::vec3 sample_point, const float w_i) {
        M++;
        if (W == 0) {
            W = w_i;
            sample = new_sample;
            return;
        }
        W += w_i;
        if (const float p = w_i / W;
            rand() / static_cast<float>(RAND_MAX) < p) {
            sample = new_sample;
            sample_pos = sample_point;
        }
    }

    void reset() {
        sample = {glm::vec3(0.0), glm::vec3(0.0), glm::vec3(0.0), glm::vec3(0.0), 0};
        M = 0;
        W = 0;
    }
};

inline reservoir merge_reservoirs(const std::vector<reservoir> &reservoirs) {
    reservoir merged;
    for (const auto &res: reservoirs) {
        merged.update(res.sample, res.sample_pos, res.W);
    }
    return merged;
}

class restir_light_sampler {
public:
    restir_light_sampler(const int x, const int y,
                         std::vector<triangular_light> &lights_vec) : x_pixels(x), y_pixels(y) {
        prev_reservoirs = std::vector(y, std::vector<reservoir>(x));
        current_reservoirs = std::vector(y, std::vector<reservoir>(x));
        num_lights = static_cast<int>(lights_vec.size());
        lights = lights_vec.data();
    }

    void reset() {
        // Reset the reservoirs
        for (int y = 0; y < y_pixels; y++) {
            for (int x = 0; x < x_pixels; x++) {
                current_reservoirs.at(y).at(x).reset();
                prev_reservoirs.at(y).at(x).reset();
            }
        }
    }

    std::vector<std::vector<sampler_result> > sample_lights(std::vector<std::vector<hit_info> > hit_infos) {
        // Swap the current and previous reservoirs
        next_frame();
        // For every pixel:
        // 1. Sample M times from the light sources; Choose one sample (reservoir)
        // 2. Check visibility of the light sample
        // 3. Temporal update - update the current reservoir with the previous one
        // 4. Spatial update - update the current reservoir with the neighbors
        // 5. Return the sample in the current reservoir

#pragma omp parallel for
        for (int y = 0; y < y_pixels; y++) {
            for (int x = 0; x < x_pixels; x++) {
                hit_info &hi = hit_infos.at(y).at(x);
                set_initial_sample(x, y, hi);
                // visibility_check(x, y, hi, world);
                temporal_update(x, y);
            }
        }

        std::vector<std::vector<sampler_result> > results(y_pixels, std::vector<sampler_result>(x_pixels));
#pragma omp parallel for
        for (int y = 0; y < y_pixels; y++) {
            for (int x = 0; x < x_pixels; x++) {
                hit_info &hi = hit_infos.at(y).at(x);
                spatial_update(x, y);
                auto res = current_reservoirs.at(y).at(x);
                results[y][x].light_point = res.sample_pos;
                results[y][x].light_dir = normalize(res.sample_pos - hi.r.at(hi.t));
                results[y][x].light = res.sample;
            }
        }
        return results;
    }

    void set_initial_sample(const int x, const int y, const hit_info &hi) {
        // Create a reservoir for the pixel
        reservoir &res = current_reservoirs.at(y).at(x);
        res.reset();
        // Sample M times from the light sources
        for (int k = 0; k < m; k++) {
            triangular_light l = pick_light();
            const glm::vec3 sample_point = sample_on_light(l);
            res.update(l, sample_point, get_light_weight(l, sample_point, hi));
        }
    }

    void visibility_check(const int x, const int y, const hit_info &hi, World &world) {
        // Check visibility of the light sample
        auto &res = current_reservoirs.at(y).at(x);
        // Shadow dir = light sample - hit point
        const glm::vec3 shadow_dir = res.sample_pos - hi.r.at(hi.t);
        // Create a ray from the hit point to the light sample
        Ray shadow(hi.r.at(hi.t), normalize(shadow_dir));
        // Check if the ray intersects with the scene
        hit_info shadow_hit;
        world.intersect(shadow, shadow_hit);
        // If the ray intersects with the scene, discard the sample
        if (shadow_hit.t > 1e-3f && shadow_hit.t < length(shadow_dir)) {
            res.reset();
        }
    }

    void temporal_update(const int x, const int y) {
        // Update the current reservoir with the previous one
        const auto &prev_res = prev_reservoirs.at(y).at(x);
        auto &curr_res = current_reservoirs.at(y).at(x);
        curr_res.update(prev_res.sample, prev_res.sample_pos, prev_res.W);
    }

    void spatial_update(const int x, const int y) {
        // Update the current reservoir with the neighbors
        std::vector<reservoir> all_reservoirs;
        auto &curr_res = current_reservoirs.at(y).at(x);
        all_reservoirs.push_back(curr_res);
        for (int xx = -1; xx <= 1; xx++) {
            for (int yy = -1; yy <= 1; yy++) {
                if (xx == 0 && yy == 0) continue;
                const int nx = x + xx;
                if (const int ny = y + yy; nx >= 0 && nx < x_pixels && ny >= 0 && ny < y_pixels) {
                    all_reservoirs.push_back(current_reservoirs.at(ny).at(nx));
                }
            }
        }
        // Merge the reservoirs
        const auto merged_res = merge_reservoirs(all_reservoirs);
        // Update the current reservoir with the merged one
        curr_res.update(merged_res.sample, merged_res.sample_pos, merged_res.W);
    }

    void spatial_update(const int x, const int y, const int radius) {
        for (int r = 0; r < radius; r++) {
            spatial_update(x, y);
        }
    }

    void next_frame() {
        // Swap the current and previous reservoirs
        std::swap(prev_reservoirs, current_reservoirs);
    }

private:
    int m = 2;
    int x_pixels;
    int y_pixels;
    std::vector<std::vector<reservoir> > prev_reservoirs;
    std::vector<std::vector<reservoir> > current_reservoirs;
    triangular_light *lights;
    int num_lights;

    [[nodiscard]] triangular_light pick_light() const {
        // Pick a random light source uniformly (standard, change this if you want to use a different sampling strategy)
        const int index = rand() % num_lights;
        return lights[index];
    }

    [[nodiscard]] float get_light_weight(const triangular_light &light, const glm::vec3 light_point,
                                         const hit_info &hi) const {
        const glm::vec3 hit_point = hi.r.at(hi.t);
        const glm::vec3 light_dir = light_point - hit_point;
        const float cos_theta = glm::dot(glm::normalize(light_dir), hi.normal);
        const glm::vec3 brdf = hi.mat_ptr->evaluate(hi, light_dir);

        const float target = cos_theta * glm::length(brdf);
        const float source = 1.0f / static_cast<float>(num_lights);

        return source / target;
    }
};

#endif //LIGHT_SAMPLER_H
