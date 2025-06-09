#pragma once

#include <glm/glm.hpp>
#include "lib/nanoflann.hpp"
#include <iostream>
#include <vector>
#include <limits>
#include <memory>

struct SphereCloud {
    std::unique_ptr<std::vector<glm::vec3>> points;

    // KD-tree typedef
    using KDTree = nanoflann::KDTreeSingleIndexAdaptor<
        nanoflann::L2_Simple_Adaptor<float, SphereCloud>,
        SphereCloud,
        3 // dimensions
    >;

    std::unique_ptr<KDTree> index;

    SphereCloud() = default;
    explicit SphereCloud(std::unique_ptr<std::vector<glm::vec3>> points);

    inline size_t kdtree_get_point_count() const { return points->size(); }

    inline float kdtree_get_pt(const size_t idx, int dim) const {
        return (*points)[idx][dim]; // dim = 0 (x), 1 (y), 2 (z)
    }

    template <class BBOX>
    bool kdtree_get_bbox(BBOX&) const { return false; }

    void build_index();
    glm::vec3 find_closest(const glm::vec3& point) const;
};