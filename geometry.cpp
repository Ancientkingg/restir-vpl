#include "geometry.hpp"

#include <glm/glm.hpp>
#include <array>
#ifndef TINY_BVH_H_
#include "lib/tiny_bvh.h"
#endif

#include "constants.hpp"
glm::vec3 Triangle::calculateNormal() {
	_normal = glm::normalize(glm::cross(v1.position - v0.position, v2.position - v0.position));
	if (glm::dot(_normal, v0.normal) < 0.0f) {
		_normal = -_normal;
	}

	return _normal;
}

Triangle::Triangle(const Vertex& v0, const Vertex& v1, const Vertex& v2, int material_id)
	: v0(v0), v1(v1), v2(v2), material_id(material_id) {

	calculateNormal();
}

Triangle::Triangle(const glm::vec3 v0, const glm::vec3 v1, const glm::vec3 v2, int material_id)
	: v0({ v0, {}, {} }), v1({ v1, {}, {} }), v2({ v2, {}, {} }), material_id(material_id) {

	calculateNormal();
}

Triangle::Triangle(const Vertex vertices[3], int material_id)
	: v0(vertices[0]), v1(vertices[1]), v2(vertices[2]), material_id(material_id) {

	calculateNormal();
}

glm::vec3 Triangle::normal(glm::vec2 uv) const {
#ifdef INTERPOLATE_NORMALS
	const float u = uv.x;
	const float v = uv.y;
	const float w = 1.0f - u - v;

	glm::vec3 n = v0.normal * u +
		v1.normal * v +
		v2.normal * w;

	return glm::normalize(n);
#else
	return _normal;
#endif
}

std::array<tinybvh::bvhvec4, 3> Triangle::toBvhVec4() const {
	return {
		tinybvh::bvhvec4(v0.position.x, v0.position.y, v0.position.z, static_cast<float>(material_id)),
		tinybvh::bvhvec4(v1.position.x, v1.position.y, v1.position.z, static_cast<float>(material_id)),
		tinybvh::bvhvec4(v2.position.x, v2.position.y, v2.position.z, static_cast<float>(material_id))
	};
}

std::vector<tinybvh::bvhvec4> toBVHVec(const std::vector<Triangle>& triangles) {
	std::vector<tinybvh::bvhvec4> bvh_vecs;
	bvh_vecs.reserve(triangles.size() * 3);
	for (const auto& triangle : triangles) {
		auto vecs = triangle.toBvhVec4();
		bvh_vecs.insert(bvh_vecs.end(), vecs.begin(), vecs.end());
	}
	return bvh_vecs;
}
