//
// Created by Rafayel Gardishyan on 23/04/2025.
//

#ifndef RAY_H
#define RAY_H

#include <glm/vec3.hpp>

class Ray {
public:
    Ray() {}

    Ray(const glm::vec3& origin, const glm::vec3& direction) : orig(origin), dir(direction) {}

    const glm::vec3& origin() const  { return orig; }
    const glm::vec3 &direction() const { return dir; }

    glm::vec3 at(float t) const {
        return orig + dir * t;
    }

private:
    glm::vec3 orig;
    glm::vec3 dir;
};

#endif