#include "camera.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>

#include "ray.hpp"
#include "material.hpp"
#include "hit_info.hpp"
#include "light_sampler.hpp"
#include "world.hpp"
#include "tiny_bvh_types.hpp" 

tinybvh::Ray toBVHRay(const Ray& r) {
	return tinybvh::Ray(toBVHVec(r.origin()), toBVHVec(r.direction()));
}

tinybvh::Ray toBVHRay(const Ray& r, const float max_t) {
	return tinybvh::Ray(toBVHVec(r.origin()), toBVHVec(r.direction()), max_t);
}

// TODO: Cache ray hits (so we dont compute the same thing) when camera does not move

Camera::Camera() : Camera(glm::vec3(0, 40, 1), glm::vec3(0, 40, 0)) {}

Camera::Camera(glm::vec3 camera_position, glm::vec3 camera_target)
	: position(camera_position), target(camera_target) {
	direction = glm::normalize(target - position);
	glm::vec3 world_up = glm::vec3(0, 1, 0);
	right = glm::normalize(glm::cross(direction, world_up));
	up = glm::normalize(glm::cross(right, direction));
	forward = glm::normalize(glm::cross(up, right));
}

void Camera::rotate(float angle, glm::vec3 axis) {
	// Change the direction of the camera and recalculate the right up and forward vectors
	// Create rotation matrix for the direction vector, around the axis, starting at camera pos
	glm::mat4 rotation_matrix = glm::rotate(glm::mat4(1.0f), angle, axis);
	glm::vec4 new_direction = rotation_matrix * glm::vec4(direction, 1.0f);
	direction = glm::normalize(glm::vec3(new_direction));
	right = glm::normalize(glm::cross(direction, up));
	up = glm::normalize(glm::cross(right, direction));
	forward = glm::normalize(glm::cross(up, right));
}

void Camera::updateDirection() {
	glm::vec3 dir;
	dir.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	dir.y = sin(glm::radians(pitch));
	dir.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
	direction = glm::normalize(dir);

	right = glm::normalize(glm::cross(direction, glm::vec3(0.0f, 1.0f, 0.0f))); // world up
	up = glm::normalize(glm::cross(right, direction));
	forward = direction;
}

std::vector<std::vector<Ray>> Camera::generate_rays_for_frame() {
	std::vector<std::vector<Ray>> rays(image_height, std::vector<Ray>(image_width));

	float halfWidth = float(image_width) * 0.5f;
	float halfHeight = float(image_height) * 0.5f;

	float scaleX = tan(fov / 2) * focal_length;
	float scaleY = scaleX / aspect_ratio;


#pragma omp parallel for
	// (0, 0) = top_right; first one is height second one is width;
	for (int i = 0; i < image_height; i++) {
		for (int j = 0; j < image_width; j++) {

			// Centered pixel coordinates in [–1, +1]
			float ndc_x = (j - halfWidth) / halfWidth;
			float ndc_y = (i - halfHeight) / halfHeight;

			glm::vec3 ray_dir =
				direction + ndc_x * scaleX * right + ndc_y * scaleY * up;

			rays[i][j] = Ray(position, glm::normalize(ray_dir));
		}
	}

	return rays;
}

std::vector<HitInfo> Camera::get_hit_info_from_camera_per_frame(
	tinybvh::BVH& bvh, std::vector<Material*>& mats) {
	std::vector<HitInfo> hit_infos(
		image_height * image_width);

	auto rays = generate_rays_for_frame();

#pragma omp parallel for
	for (int i = 0; i < image_height; i++) {
		for (int j = 0; j < image_width; j++) {
			hit_infos[i * image_width + j].r = rays[i][j];
			tinybvh::Ray r = toBVHRay(rays[i][j]);
			bvh.Intersect(r);
			hit_infos[i * image_width + j].t = r.hit.t;

			hit_infos[i * image_width + j].triangle[0] = toGLMVec(bvh.verts[r.hit.prim * 3 + 0]);
			hit_infos[i * image_width + j].triangle[1] = toGLMVec(bvh.verts[r.hit.prim * 3 + 1]);
			hit_infos[i * image_width + j].triangle[2] = toGLMVec(bvh.verts[r.hit.prim * 3 + 2]);

			hit_infos[i * image_width + j].normal = glm::normalize(
				glm::cross(hit_infos[i * image_width + j].triangle[1] - hit_infos[i * image_width + j].triangle[0],
					hit_infos[i * image_width + j].triangle[2] - hit_infos[i * image_width + j].triangle[0]));
			if (glm::dot(hit_infos[i * image_width + j].normal, rays[i][j].direction()) > 0.0f)
				hit_infos[i * image_width + j].normal = -hit_infos[i * image_width + j].normal;

			int m_id = bvh.verts[r.hit.prim * 3][3];
			hit_infos[i * image_width + j].mat_ptr = mats[m_id];
		}
	}

	return hit_infos;
}