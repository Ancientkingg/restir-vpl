#pragma once

#include <glm/vec3.hpp>

class Ray {
public:
    Ray();

    Ray(const glm::vec3& origin, const glm::vec3& direction);

    const glm::vec3& origin() const;
    const glm::vec3& direction() const;

    glm::vec3 at(float t) const;

private:
    glm::vec3 orig;
    glm::vec3 dir;
};