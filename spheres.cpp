#include "spheres.hpp"

#include <glm/glm.hpp>
#include "lib/nanoflann.hpp"
#include <iostream>
#include <vector>
#include <limits>
#include <memory>


SphereCloud::SphereCloud(std::unique_ptr<std::vector<glm::vec3>> pts) : points(std::move(pts)) {
    build_index();
}

void SphereCloud::build_index() {
    if (points->empty()) return;
    index = std::make_unique<KDTree>(3, *this, nanoflann::KDTreeSingleIndexAdaptorParams(10));
    index->buildIndex();
}

glm::vec3 SphereCloud::find_closest(const glm::vec3& point) const {
    if (!index || points->empty()) {
        std::cerr << "Error: KD-tree not built or SphereCloud is empty!" << std::endl;
        return glm::vec3(0.0f);
    }

    size_t retIndex = 0;
    float outDistSqr = 0.0f;
    nanoflann::KNNResultSet<float> resultSet(1);
    resultSet.init(&retIndex, &outDistSqr);
    index->findNeighbors(resultSet, &point[0], nanoflann::SearchParameters());

    return (*points)[retIndex];
}