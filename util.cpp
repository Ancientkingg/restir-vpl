#include <glm/glm.hpp>

#define RANDVEC3 glm::vec3(float(rand()) / RAND_MAX, float(rand()) / RAND_MAX, float(rand()) / RAND_MAX)

glm::vec3 random_in_unit_sphere() {
	glm::vec3 p;
	do {
		p = 2.0f * RANDVEC3 - glm::vec3(1, 1, 1);
	} while (glm::dot(p, p) >= 1.0f);
	return p;
}

glm::vec3 random_in_hemisphere(const glm::vec3 normal) {
	glm::vec3 in_unit_sphere = random_in_unit_sphere();

	if (glm::dot(in_unit_sphere, normal) > 0.0) {
		return in_unit_sphere;
	}
	return -in_unit_sphere;
}

glm::vec3 reflect(const glm::vec3& v, const glm::vec3& n) {
	return v - 2.0f * glm::dot(v, n) * n;
}

bool near_zero(const glm::vec3& v) {
	float theta = 1e-8;
	return (fabs(v[0]) < theta) && (fabs(v[1]) < theta) && (fabs(v[2]) < theta);
}

glm::vec3 refract(const glm::vec3& uv, const glm::vec3& n, float etai_over_etat) {
	float cos_theta = fminf(glm::dot(-uv, n), 1.0f);
	glm::vec3 r_out_perp = etai_over_etat * (uv + cos_theta * n);
	glm::vec3 r_out_parallel = -sqrtf(fabsf(1.0 - glm::dot(r_out_perp, r_out_perp))) * n;
	return r_out_perp + r_out_parallel;
}