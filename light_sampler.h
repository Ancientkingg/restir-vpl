//
// Created by Rafayel Gardishyan on 28/04/2025.
//

#ifndef LIGHT_SAMPLER_H
#define LIGHT_SAMPLER_H
#include "triangular_light.h"
#include "ray.h"

class temp_hit_info {
public:
    ray r;
    float t;
    vec3 triangle[3]; // the vertices of the triangle in the right order for normal calculation
    // don't care about the rest
};

class reservoir {
public:
    triangular_light sample;
    point3 sample_pos;
    int M;
    float W;

    reservoir() : sample({0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, 0), sample_pos(0, 0, 0), M(0), W(0) {
    }

    void update(triangular_light new_sample, point3 sample_point, float w_i) {
        M++;
        if (W == 0) {
            W = w_i;
            sample = new_sample;
            return;
        }
        W += w_i;
        float p = float(w_i) / float(W);
        if (rand() / float(RAND_MAX) < p) {
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
    restir_light_sampler(u_int x, u_int y, std::vector<triangular_light> &lights_vec) : x_pixels(x), y_pixels(y) {
        prev_reservoirs = std::vector<reservoir>(x * y);
        current_reservoirs = std::vector<reservoir>(x * y);
        num_lights = lights_vec.size();
        lights = lights_vec.data();
    }

    void set_initial_sample(u_int i, u_int j, temp_hit_info &hi) {
        // Create a reservoir for the pixel
        reservoir &res = current_reservoirs.at(i * y_pixels + j);
        res.reset();
        // Sample M times from the light sources
        for (int k = 0; k < m; k++) {
            triangular_light l = pick_light();
            point3 sample_point = sample_on_light(l);
            res.update(l, sample_point, get_light_weight(l, sample_point, hi.r.at(t)));
        }
    }

    void visibility_check(u_int i, u_int j, ray &r, tinybvh::BVH &bvh) {
        // Check visibility of the light sample
        auto &res = current_reservoirs.at(i * y_pixels + j);
        tinybvh::Ray shadow(
            toBVHVec(r.origin() + (r.direction() * 1e-4f)), // avoid self‐intersection
            toBVHVec(res.sample_pos - r.origin())
        );
        bvh.Intersect(shadow);
        if (shadow.hit.t < (res.sample_pos - r.origin()).length()) {
            res.reset();
        }
    }

    void temporal_update(u_int i, u_int j) {
        // Update the current reservoir with the previous one
        auto &prev_res = prev_reservoirs.at(i * y_pixels + j);
        auto &curr_res = current_reservoirs.at(i * y_pixels + j);
        curr_res.update(prev_res.sample, prev_res.sample_pos, prev_res.W);
    }

    void spatial_update(u_int i, u_int j) {
        // Update the current reservoir with the neighbors
        std::vector<reservoir> all_reservoirs;
        auto &curr_res = current_reservoirs.at(i * y_pixels + j);
        all_reservoirs.push_back(curr_res);
        for (int x = -1; x <= 1; x++) {
            for (int y = -1; y <= 1; y++) {
                if (x == 0 && y == 0) continue;
                int ni = i + x;
                int nj = j + y;
                if (ni >= 0 && ni < x_pixels && nj >= 0 && nj < y_pixels) {
                    all_reservoirs.push_back(current_reservoirs.at(ni * y_pixels + nj));
                }
            }
        }
        // Merge the reservoirs
        auto merged_res = merge_reservoirs(all_reservoirs);
        // Update the current reservoir with the merged one
        curr_res.update(merged_res.sample, merged_res.sample_pos, merged_res.W);
    }

    void spatial_update(u_int i, u_int j, int radius) {
        for (int r = 0; r < radius; r++) {
            spatial_update(i, j);
        }
    }

    void next_frame() {
        // Swap the current and previous reservoirs
        std::swap(prev_reservoirs, current_reservoirs);
    }

private:
    u_int m = 4;
    u_int x_pixels;
    u_int y_pixels;
    std::vector<reservoir> prev_reservoirs;
    std::vector<reservoir> current_reservoirs;
    triangular_light *lights;
    u_int num_lights;

    triangular_light pick_light() const {
        // Pick a random light source uniformly (standard, change this if you want to use a different sampling strategy)
        int index = rand() % num_lights;
        return lights[index];
    }

    static float get_light_weight(triangular_light &light, point3 light_point, temp_hit_info &hi) {
        point3 hit_point = hi.r.at(hi.t);
        // w = light_intensity * cos(theta) * solid_angle
        // theta = angle between light direction and triangle normal
        // solid_angle = area of light / distance^2
        vec3 light_dir = light_point - hit_point;
        vec3 triangle_normal = unit_vector(cross(hi.triangle[1] - hi.triangle[0], hi.triangle[2] - hi.triangle[0]));
        float cos_theta = dot(unit_vector(light_dir), triangle_normal);
        float distance = light_dir.length();
        float area_of_light = 0.5f * cross(light.v1 - light.v0, light.v2 - light.v0).length();
        float solid_angle = area_of_light / (distance * distance);

        return light.intensity * cos_theta * solid_angle;
    }
};

#endif //LIGHT_SAMPLER_H
