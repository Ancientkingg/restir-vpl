#pragma once

#ifndef TINY_BVH_H_
#include "lib/tiny_bvh.h"
#endif
#include <glm/vec3.hpp>

inline tinybvh::bvhvec3 toBVHVec(const glm::vec3& v) {
	return tinybvh::bvhvec3(v.x, v.y, v.z);
}

inline glm::vec3 toGLMVec(const tinybvh::bvhvec3& v) {
	return glm::vec3(v.x, v.y, v.z);
}