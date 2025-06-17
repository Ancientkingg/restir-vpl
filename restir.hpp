#pragma once

#include <glm/vec3.hpp>
#include <vector>
#include <array>
#include <span>
#include <random>
#include <iostream>

#include "light.hpp"
#include "ray.hpp"
#include "world.hpp"
#include "hit_info.hpp"


enum class SamplingMode {
    ReSTIR,
    Uniform,
    RIS,
};

inline std::ostream& operator<<(std::ostream& out, const SamplingMode& mode) {
    switch (mode) {
    case SamplingMode::ReSTIR:
        out << std::string("ReSTIR");
        break;
    case SamplingMode::Uniform:
        out << std::string("Uniform");
        break;
    case SamplingMode::RIS:
        out << std::string("RIS");
        break;
    }
    return out;
}


struct SamplerResult {
    glm::vec3 light_point;
    glm::vec3 light_dir;
    float W;
    std::weak_ptr<PointLight> light;

    SamplerResult();
};

struct SampleInfo {
    std::weak_ptr<PointLight> light;
	glm::vec3 light_point;

    SampleInfo();
	SampleInfo(const std::weak_ptr<PointLight> light, const glm::vec3& light_point);
};

class Reservoir {
public:
    SampleInfo y;
    float w_sum;
    int M;
    float phat;
    float W;

    Reservoir();
    bool update(const SampleInfo x_i, const float w_i, const float n_phat);
    static Reservoir combineReservoirs(const std::span<const Reservoir*>& reservoirs);
	static Reservoir combineReservoirsUnbiased(const std::span<const Reservoir*>& reservoirs);
    void replace(const Reservoir& other);
    void reset();
};

class RestirLightSampler {
public:
    RestirLightSampler(const int x, const int y,
        std::vector<std::weak_ptr<PointLight>>& lights_vec);

    void reset();

    std::vector<std::vector<SamplerResult>> sample_lights(std::vector<HitInfo> hit_infos, World& scene);

    void set_initial_sample(Reservoir& r, const HitInfo& hi);

    bool visibility_check(Reservoir& res, const HitInfo& hi, World& world, bool reset_phat = false);
    bool is_visible(Reservoir& res, const HitInfo& hi, World& world);

    Reservoir temporal_update(const Reservoir& current, const Reservoir& prev);

    void spatial_update(const int x, const int y, const std::vector<HitInfo>& hit_infos, World& scene);

    void swap_buffers();

    int m = 3;

    SamplingMode sampling_mode = SamplingMode::Uniform;

	inline int num_lights() const {
		return static_cast<int>(lights.size());
	}
private:
    int x_pixels;
    int y_pixels;
    std::vector<Reservoir> prev_reservoirs;
    std::vector<Reservoir> current_reservoirs;
    std::vector<std::weak_ptr<PointLight>> lights;

    [[nodiscard]] std::weak_ptr<PointLight> pick_light(float& pdf) const;

    [[nodiscard]] int sample_light_index() const;

    void get_light_weight(const SampleInfo& sample,
        const HitInfo& hi, float& W, float& phat) const;
};