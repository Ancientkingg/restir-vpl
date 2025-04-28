//
// Created by Rafayel Gardishyan on 28/04/2025.
//

#ifndef LIGHT_SAMPLER_H
#define LIGHT_SAMPLER_H
#include "ray.h"
#include "triangular_light.h"
#include "vec3.h"

class reservoir {
public:
    triangular_light sample;
    point3 sample_pos;
    int M;
    int W;

    reservoir() : sample({0,0,0},{0,0,0},{0,0,0}, {0,0,0}, 0), sample_pos(0,0,0), M(0), W(0) {}

    void update(triangular_light new_sample, point3 sample_point, int w_i);

    void reset();
};

class restir_light_sampler {
public:
    restir_light_sampler(u_int x, u_int y, std::vector<triangular_light> &lights_vec) : x_pixels(x), y_pixels(y) {
        prev_reservoirs = std::vector<reservoir>(x * y);
        current_reservoirs = std::vector<reservoir>(x * y);
        num_lights = lights_vec.size();
        lights = lights_vec.data();
    }

    void set_initial_sample(u_int i, u_int j, ray &r);
    void visibility_check(u_int i, u_int j, ray &r, tinybvh::BVH &bvh);
    void temporal_update(u_int i, u_int j);
    void spatial_update(u_int i, u_int j);
    void spatial_update(u_int i, u_int j, int radius);

    void next_frame();

private:
    u_int m = 4;
    u_int x_pixels; u_int y_pixels;
    std::vector<reservoir> prev_reservoirs;
    std::vector<reservoir> current_reservoirs;
    triangular_light *lights;
    u_int num_lights;

    triangular_light pick_light();
    int get_light_weight(triangular_light &light, ray &r);
};

#endif //LIGHT_SAMPLER_H
