#ifndef CAMERA2_H_
#define CAMERA2_H_

#include <glm/glm.hpp>
#include <iostream>
#include <vector>

#include "ray.h"
#include "material.h"
#include "hit_info.h"
#include "light_sampler.h"
#include "world.hpp"


tinybvh::Ray toBVHRay(const Ray& r);

// Example shade function
glm::vec3 shade(const hit_info& hit, const sampler_result& sample, float pdf, World& scene);

// TODO: Cache ray hits (so we dont compute the same thing) when camera does not move

class Camera2 {
public:

    Camera2();

    Camera2(glm::vec3 camera_position, glm::vec3 camera_target);

    // this can be modified externally 
    glm::vec3 position;
    glm::vec3 target;
    glm::vec3 direction;
    glm::vec3 right;
    glm::vec3 up;

    /* Public Camera Parameters Here */
    const float aspect_ratio = 16.0 / 9; // Ratio of image width over height
    const float focal_length = 1.0;
    const int image_width = 800; // Rendered image width in pixel count
    const int image_height = (int(image_width / aspect_ratio) < 1) ? 1 : int(image_width / aspect_ratio); // Rendered image height in pixel count
    const float viewport_height = 2.0;
    const float viewport_width = viewport_height * (float(image_width) / image_height);

    const glm::vec3 viewport_u = glm::vec3(viewport_width, 0, 0);
    const glm::vec3 viewport_v = glm::vec3(0, -viewport_height, 0);
    // Calculate the horizontal and vertical delta vectors from pixel to pixel.
    const glm::vec3 pixel_delta_u = float(1.0 / image_width) * viewport_u;
    const glm::vec3 pixel_delta_v = viewport_v / float(image_height);
    const float tanfov = 2.0f * focal_length / viewport_height;

    std::vector<std::vector<Ray>> generate_rays_for_frame();

    std::vector<std::vector<hit_info>> get_hit_info_from_camera_per_frame(
        tinybvh::BVH& bvh, std::vector<material>& mats);
};

#endif // CAMERA2_H_