//
// Created by Rafayel Gardishyan on 28/04/2025.
//

#ifndef LIGHT_SAMPLER_H
#define LIGHT_SAMPLER_H
#include "triangular_light.h"
#include "ray.h"


struct sampler_result {
    vec3 light_point;
    vec3 light_dir;
    triangular_light light;
};

class reservoir {
public:
    triangular_light sample;
    point3 sample_pos;
    int M;
    float W;

    reservoir() : sample({0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, 0), sample_pos(0, 0, 0), M(0), W(0) {
    }

    void update(const triangular_light &new_sample, const point3 sample_point, const float w_i) {
        M++;
        if (W == 0) {
            W = w_i;
            sample = new_sample;
            return;
        }
        W += w_i;
        if (const float p = static_cast<float>(w_i) / static_cast<float>(W); rand() / static_cast<float>(RAND_MAX) < p) {
            sample = new_sample;
            sample_pos = sample_point;
        }
    }

    void reset() {
        sample = {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, 0};
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
    restir_light_sampler(int x, int y, std::vector<triangular_light> &lights_vec) : x_pixels(x), y_pixels(y) {
        prev_reservoirs = std::vector<std::vector<reservoir>>(y, std::vector<reservoir>(x));
        current_reservoirs = std::vector<std::vector<reservoir>>(y, std::vector<reservoir>(x));
        num_lights = lights_vec.size();
        lights = lights_vec.data();
    }

    std::vector<std::vector<sampler_result>> sample_lights(std::vector<std::vector<hit_info> > hit_infos,
                                                            const tinybvh::BVH &bvh) {
        // Swap the current and previous reservoirs
        next_frame();
        // For every pixel:
        // 1. Sample M times from the light sources; Choose one sample (reservoir)
        // 2. Check visibility of the light sample
        // 3. Temporal update - update the current reservoir with the previous one
        // 4. Spatial update - update the current reservoir with the neighbors
        // 5. Return the sample in the current reservoir
        std::vector<std::vector<sampler_result> > results;
        for (int y = 0; y < y_pixels; y++) {
            std::vector<sampler_result> row;
            for (int x = 0; x < x_pixels; x++) {
                hit_info &hi = hit_infos.at(y).at(x);
                set_initial_sample(x, y, hi);
                visibility_check(x, y, hi, bvh);
                temporal_update(x, y);
                spatial_update(x, y);
                auto res = current_reservoirs.at(y).at(x);
                sampler_result result
                {
                    res.sample_pos,
                    unit_vector(res.sample_pos - hi.r.at(hi.t)),
                    res.sample
                };
                row.push_back(result);
            }
            results.push_back(row);
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
            const point3 sample_point = sample_on_light(l);
            res.update(l, sample_point, get_light_weight(l, sample_point, hi));
        }
    }

    void visibility_check(const int x, const int y, const hit_info &hi, const tinybvh::BVH &bvh) {
        // Check visibility of the light sample
        auto &res = current_reservoirs.at(y).at(x);
        // Shadow dir = light sample - hit point
        const vec3 shadow_dir = res.sample_pos - hi.r.at(hi.t);
        // Create a ray from the hit point to the light sample
        const ray shadow(hi.r.at(hi.t), unit_vector(shadow_dir));
        // Check if the ray intersects with the scene
        tinybvh::Ray shadow_ray(toBVHVec(shadow.origin()), toBVHVec(shadow.direction()));
        bvh.Intersect(shadow_ray);
        // If the ray intersects with the scene, discard the sample
        if (shadow_ray.hit.t < shadow_dir.length()) {
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
    int m = 4;
    int x_pixels;
    int y_pixels;
    std::vector<std::vector<reservoir>> prev_reservoirs;
    std::vector<std::vector<reservoir>> current_reservoirs;
    triangular_light *lights;
    int num_lights;

    triangular_light pick_light() const {
        // Pick a random light source uniformly (standard, change this if you want to use a different sampling strategy)
        const int index = rand() % num_lights;
        return lights[index];
    }

    static float get_light_weight(const triangular_light &light, const point3 light_point, const hit_info &hi) {
        const point3 hit_point = hi.r.at(hi.t);
        // w = light_intensity * cos(theta) * solid_angle
        // theta = angle between light direction and triangle normal
        // solid_angle = area of light / distance^2
        const vec3 light_dir = light_point - hit_point;
        const float cos_theta = dot(unit_vector(light_dir), to_vec3(hi.normal));
        const float distance = light_dir.length();
        const float area_of_light = 0.5f * cross(light.v1 - light.v0, light.v2 - light.v0).length();
        const float solid_angle = area_of_light / (distance * distance);

        return light.intensity * cos_theta * solid_angle;
    }
};

#endif //LIGHT_SAMPLER_H
