#include <glm/vec3.hpp>

glm::vec3 random_in_unit_sphere();
glm::vec3 random_in_hemisphere(const glm::vec3 normal);
glm::vec3 reflect(const glm::vec3& v, const glm::vec3& n);
bool near_zero(const glm::vec3& v);
glm::vec3 refract(const glm::vec3& uv, const glm::vec3& n, float etai_over_etat);