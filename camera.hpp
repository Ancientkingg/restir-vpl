#pragma once

#include <glm/glm.hpp>
#include <iostream>
#include <vector>
#include <string>

#include "ray.hpp"
#include "material.hpp"
#include "hit_info.hpp"
#include "restir.hpp"
#include "world.hpp"
#include "constants.hpp"


tinybvh::Ray toBVHRay(const Ray& r);
tinybvh::Ray toBVHRay(const Ray& r, const float max_t);

// TODO: Cache ray hits (so we dont compute the same thing) when camera does not move

class Camera {
public:

    Camera();

    Camera(glm::vec3 camera_position, glm::vec3 camera_target);

    // this can be modified externally 
    glm::vec3 position;
    glm::vec3 target;
    glm::vec3 direction;
    glm::vec3 right;
    glm::vec3 up;
    glm::vec3 forward;

    float yaw = -90.0f;
    float pitch = 0.0f;

    void updateDirection();

    void rotate(float angle, glm::vec3 axis);

    /* Public Camera Parameters Here */
    const float aspect_ratio = ASPECT_RATIO; // Ratio of image width over height
    const float focal_length = 1.0;
    int image_width = LIVE_WIDTH; // Rendered image width in pixel count
    int image_height = (int(image_width / aspect_ratio) < 1) ? 1 : int(image_width / aspect_ratio); // Rendered image height in pixel count
    const float fov = glm::radians(75.0f); // horizontal fov

    std::vector<std::vector<Ray>> generate_rays_for_frame();

    std::vector<HitInfo> get_hit_info_from_camera_per_frame(World& world);

    void save_to_file(std::string filename);
	void load_from_file(std::string filename);

private:
    std::vector<HitInfo> hit_infos;

    glm::vec3 last_pos;
	glm::vec3 last_right, last_up, last_forward;

    std::vector<HitInfo> calculate_hit_info(World& world);
};