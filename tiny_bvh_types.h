#ifndef TINY_BVH_TYPES_H
#define TINY_BVH_TYPES_H

#include "lib/tiny_bvh.h"
#include <glm/vec3.hpp>

// write to and from functions for vec3 from glm
inline tinybvh::bvhvec3 toBVHVec(const glm::vec3& v) {
	return tinybvh::bvhvec3(v.x, v.y, v.z);
}

inline glm::vec3 toGLMVec(const tinybvh::bvhvec3& v) {
	return glm::vec3(v.x, v.y, v.z);
}

#endif