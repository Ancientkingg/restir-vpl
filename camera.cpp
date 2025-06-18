#include "camera.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <string>
#include <fstream>

#include "ray.hpp"
#include "material.hpp"
#include "hit_info.hpp"
#include "restir.hpp"
#include "world.hpp"
#include "tiny_bvh_types.hpp" 

tinybvh::Ray toBVHRay(const Ray& r) {
	return tinybvh::Ray(toBVHVec(r.origin()), toBVHVec(r.direction()));
}

tinybvh::Ray toBVHRay(const Ray& r, const float max_t) {
	return tinybvh::Ray(toBVHVec(r.origin()), toBVHVec(r.direction()), max_t);
}


void Camera::save_to_file(std::string filename) {
	std::ofstream file(filename);
	if (!file.is_open()) {
		std::cerr << "Error: Could not open file " << filename << " for writing." << std::endl;
		return;
	}
	file << position.x << " " << position.y << " " << position.z << "\n";
	file << yaw << "\n";
	file << pitch << "\n";
	file.close();
	std::cout << "Camera parameters saved to " << filename << std::endl;
}

void Camera::load_from_file(std::string filename) {
	std::ifstream file(filename);
	if (!file.is_open()) {
		std::cerr << "Error: Could not open file " << filename << " for reading." << std::endl;
		return;
	}

	// Read position, yaw, and pitch
	file >> position.x >> position.y >> position.z;
	file >> yaw;
	file >> pitch;

	file.close();
	std::cout << "Camera parameters loaded from " << filename << std::endl;

	// Recalculate orientation from yaw and pitch
	glm::vec3 dir;
	dir.x = cosf(glm::radians(yaw)) * cosf(glm::radians(pitch));
	dir.y = sinf(glm::radians(pitch));
	dir.z = sinf(glm::radians(yaw)) * cosf(glm::radians(pitch));
	direction = glm::normalize(dir);

	forward = direction;
	right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));
	up = glm::normalize(glm::cross(right, forward));

	// Recalculate target
	target = position + forward;
}

Camera::Camera() : Camera(glm::vec3(0, 0, 3), glm::vec3(0, 0, 1)) {
	updateDirection();
}

Camera::Camera(glm::vec3 camera_position, glm::vec3 camera_target)
	: position(camera_position), target(camera_target) {
	direction = glm::normalize(target - position);
	glm::vec3 world_up = glm::vec3(0, 1, 0);
	right = glm::normalize(glm::cross(direction, world_up));
	up = glm::normalize(glm::cross(right, direction));
	forward = glm::normalize(glm::cross(up, right));

	updateDirection();
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

	updateDirection();
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

std::vector<HitInfo> Camera::calculate_hit_info(World& world) {
	std::vector<HitInfo> hit_infos(image_height * image_width);
	auto rays = generate_rays_for_frame();

#pragma omp parallel for
	for (int i = 0; i < image_height; i++) {
		for (int j = 0; j < image_width; j++) {
			Ray ray = rays[i][j];
			HitInfo hit;
			if (world.intersect(ray, hit)) {
				hit_infos[i * image_width + j] = hit;
			}
			else {
				hit.r = ray;
				hit.t = 1E30f;
				hit_infos[i * image_width + j] = hit;
			}
		}
	}

	return hit_infos;
}

std::vector<HitInfo> Camera::get_hit_info_from_camera_per_frame(World& world) {

	if (last_pos != position ||
		last_right != right ||
		last_up != up ||
		last_forward != forward)  {

		last_pos = position;
		last_right = right;
		last_up = up;
		last_forward = forward;
		
		hit_infos = calculate_hit_info(world);
	}

	return hit_infos;
}