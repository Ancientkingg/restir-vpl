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
    double W;

    SamplerResult();
};

struct SampleInfo {
    TriangularLight light;
	glm::vec3 light_point;

    SampleInfo();
	SampleInfo(const TriangularLight& light, const glm::vec3& light_point);
};

class Reservoir {
public:
    SampleInfo y;
    double w_sum;
    long M;
    double phat;
    double W;

    Reservoir();
    void update(const SampleInfo x_i, const double w_i, const double n_phat);
    void merge(const Reservoir& other);
    void replace(const Reservoir& other);
    void reset();
};

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

    void swap_buffers();

    int m = 3;

private:
    int x_pixels;
    int y_pixels;
    std::vector<Reservoir> prev_reservoirs;
    std::vector<Reservoir> current_reservoirs;
    TriangularLight* lights;
    int num_lights;

    [[nodiscard]] TriangularLight pick_light() const;

    [[nodiscard]] int sampleLightIndex() const;

    void get_light_weight(const SampleInfo& sample,
        const HitInfo& hi, double& w, double& phat) const;
};