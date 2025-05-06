#include "camera2.hpp"

#include <glm/glm.hpp>
#include <iostream>
#include <vector>

#include "ray.h"
#include "material.h"
#include "hit_info.h"
#include "light_sampler.h"
#include "world.hpp"


tinybvh::Ray toBVHRay(const Ray& r) {
    return tinybvh::Ray(toBVHVec(r.origin()), toBVHVec(r.direction()));
}

tinybvh::Ray toBVHRay(const Ray& r, const float max_t) {
    return tinybvh::Ray(toBVHVec(r.origin()), toBVHVec(r.direction()), max_t);
}

// Example shade function
glm::vec3 shade(const hit_info& hit, const sampler_result& sample, float pdf, World& scene) {
    // Compute cosine between surface normal and light direction
    float cos_theta = glm::dot(hit.normal, sample.light_dir);
    if (cos_theta <= 0.0f || pdf <= 0.0f) {
        return glm::vec3(0.0f);
    }

    glm::vec3 I = hit.r.at(hit.t);
    glm::vec3 L = sample.light_dir;
    float dist = glm::length(sample.light_point - I);

    if (Ray r = Ray(I + 0.001f * L, L); scene.is_occluded(r, dist)) {
        return glm::vec3(0.0f); // Shadowed
    }

    // Evaluate BRDF at the hit point
    glm::vec3 brdf = hit.mat_ptr->evaluate(hit, sample.light_dir);

    // Evaluate light radiance from the light sample
    float Li = sample.light.intensity;

    // Final contribution
    return (brdf * Li * cos_theta) / pdf;
}

// TODO: Cache ray hits (so we dont compute the same thing) when camera does not move

Camera2::Camera2() : Camera2(glm::vec3(0, 40, 1), glm::vec3(0, 40, 0)) {}

Camera2::Camera2(glm::vec3 camera_position, glm::vec3 camera_target)
    : position(camera_position), target(camera_target) {
    direction = glm::normalize(target - position);
    glm::vec3 world_up = glm::vec3(0, 1, 0);
    right = glm::normalize(glm::cross(direction, world_up));
    up = glm::normalize(glm::cross(right, direction));
}

std::vector<std::vector<Ray>> Camera2::generate_rays_for_frame() {
    std::vector<std::vector<Ray>> rays(image_height, std::vector<Ray>(image_width));

    float halfWidth = float(image_width) * 0.5f;
    float halfHeight = float(image_height) * 0.5f;

    float scaleX = tan(fov/2) * focal_length;
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

std::vector<std::vector<hit_info>> Camera2::get_hit_info_from_camera_per_frame(
    tinybvh::BVH& bvh, std::vector<Material *>& mats) {
    std::vector<std::vector<hit_info>> hit_infos(
        image_height, std::vector<hit_info>(image_width));

    auto rays = generate_rays_for_frame();

    #pragma omp parallel for
    for (int i = 0; i < image_height; i++) {
        for (int j = 0; j < image_width; j++) {
            hit_infos[i][j].r = rays[i][j];
            tinybvh::Ray r = toBVHRay(rays[i][j]);
            bvh.Intersect(r);
            hit_infos[i][j].t = r.hit.t;

            hit_infos[i][j].triangle[0] = toGLMVec(bvh.verts[r.hit.prim * 3 + 0]);
            hit_infos[i][j].triangle[1] = toGLMVec(bvh.verts[r.hit.prim * 3 + 1]);
            hit_infos[i][j].triangle[2] = toGLMVec(bvh.verts[r.hit.prim * 3 + 2]);

            hit_infos[i][j].normal = glm::normalize(
                glm::cross(hit_infos[i][j].triangle[1] - hit_infos[i][j].triangle[0],
                    hit_infos[i][j].triangle[2] - hit_infos[i][j].triangle[0]));
            if (glm::dot(hit_infos[i][j].normal, rays[i][j].direction()) > 0.0f)
                hit_infos[i][j].normal = -hit_infos[i][j].normal;

            int m_id = bvh.verts[r.hit.prim * 3][3];
            hit_infos[i][j].mat_ptr = mats[m_id];
        }
    }

    return hit_infos;
}