#pragma once

#include <glm/vec3.hpp>
#include <vector>
#include <array>
#include <span>
#include <random>

#include "triangular_light.hpp"
#include "ray.hpp"
#include "world.hpp"


struct SamplerResult {
    glm::vec3 light_point;
    glm::vec3 light_dir;
    TriangularLight light;

    SamplerResult();
};

class Reservoir {
public:
    TriangularLight sample;
    glm::vec3 sample_pos;
    int M;
    float W;

    Reservoir();
    void update(const TriangularLight& new_sample, const glm::vec3 sample_point, const float w_i);
    void reset();
};

inline Reservoir merge_reservoirs(std::span<const Reservoir> reservoirs);

class RestirLightSampler {
public:
    RestirLightSampler(const int x, const int y,
        std::vector<TriangularLight>& lights_vec);

    void reset();

    std::vector<std::vector<SamplerResult> > sample_lights(std::vector<HitInfo> hit_infos);

    void set_initial_sample(const int x, const int y, const HitInfo& hi);

    void visibility_check(const int x, const int y, const HitInfo& hi, World& world);

    void temporal_update(const int x, const int y);

    void spatial_update(const int x, const int y);

    void spatial_update(const int x, const int y, const int radius);

    void next_frame();

private:
    int m = 2;
    int x_pixels;
    int y_pixels;
    std::vector<Reservoir> prev_reservoirs;
    std::vector<Reservoir> current_reservoirs;
    TriangularLight* lights;
    int num_lights;

    [[nodiscard]] TriangularLight pick_light() const;

    [[nodiscard]] int sampleLightIndex() const;

    [[nodiscard]] float get_light_weight(const TriangularLight& light, const glm::vec3 light_point,
        const HitInfo& hi) const;
};