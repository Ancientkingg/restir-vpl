#pragma once

#include <glm/glm.hpp>
#include <array>
#ifndef TINY_BVH_H_
#include "lib/tiny_bvh.h"
#endif

struct Vertex {
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 texcoord;
};

inline Vertex operator+(const Vertex& v, const glm::vec3& offset) {
	return Vertex{ v.position + offset, v.normal, v.texcoord };
}

inline Vertex operator*(const Vertex& v, const float m) {
	return Vertex{ v.position * m, v.normal, v.texcoord };
}

inline Vertex operator*(const float m, const Vertex& v) {
	return v * m;
}

struct Triangle {
	Vertex v0, v1, v2;
	int material_id;

	Triangle() = default;

	Triangle(const Vertex& v0, const Vertex& v1, const Vertex& v2, int material_id);

	Triangle(const glm::vec3 v0, const glm::vec3 v1, const glm::vec3 v2, int material_id);

	Triangle(const Vertex vertices[3], int material_id);

	glm::vec3 normal() const;

	std::array<tinybvh::bvhvec4, 3> toBvhVec4() const;
};


inline Triangle operator+(const Triangle& t, const glm::vec3& v) {
	return Triangle(t.v0 + v, t.v1 + v, t.v2 + v, t.material_id);
}

std::vector<tinybvh::bvhvec4> toBVHVec(const std::vector<Triangle>& triangles);