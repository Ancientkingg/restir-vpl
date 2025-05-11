#pragma once

#include <glm/vec3.hpp>

class alignas(16) Ray {
public:
    Ray();

    Ray(const glm::vec3& origin, const glm::vec3& direction);

    const glm::vec3& origin() const;
    const glm::vec3& direction() const;

    glm::vec3 at(float t) const;

private:
    alignas(16) glm::vec3 orig;
    alignas(16) glm::vec3 dir;
};