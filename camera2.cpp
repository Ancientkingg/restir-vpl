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

// Example shade function
glm::vec3 shade(const hit_info& hit, const sampler_result& sample, float pdf, World& scene) {
    // Compute cosine between surface normal and light direction
    float cos_theta = glm::dot(hit.normal, sample.light_dir);
    if (cos_theta <= 0.0f || pdf <= 0.0f) {
        return glm::vec3(0.0f);
    }

    // Check if the light is visible (shadow ray)
    Ray shadow_ray(hit.r.at(hit.t) + 1e-4f * sample.light_dir, sample.light_dir);
    hit_info shadow_hit;

    bool intersected = scene.intersect(shadow_ray, shadow_hit);
    bool is_light = shadow_hit.mat_ptr->emits_light();

    if (intersected && shadow_hit.mat_ptr->emits_light() == false) {
        return glm::vec3(0.0f); // Occluded
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
    direction = glm::normalize(target - position); // Forward vector
    right = glm::normalize(
        glm::cross(glm::vec3(0, 1, 0), direction));  // Right vector
    up = glm::cross(direction, right);  // Up vector in camera space
}

std::vector<std::vector<Ray>> Camera2::generate_rays_for_frame() {
    std::vector<std::vector<Ray>> rays(image_height, std::vector<Ray>(image_width));

    for (int i = 0; i < image_height; i++) {
        for (int j = 0; j < image_width; j++) {
            glm::vec3 pixel_direction = direction -
                (float(i) / image_width - 0.5f) * 2.f * tanfov * image_width /
                image_height * right -
                (0.5f - float(j) / image_height) * 2.f * tanfov * up;

            rays[i][j] = Ray(position, glm::normalize(pixel_direction));
        }
    }

    return rays;
}

std::vector<std::vector<hit_info>> Camera2::get_hit_info_from_camera_per_frame(
    tinybvh::BVH& bvh, std::vector<material>& mats) {
    std::vector<std::vector<hit_info>> hit_infos(
        image_height, std::vector<hit_info>(image_width));

    auto rays = generate_rays_for_frame();

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
            hit_infos[i][j].mat = &mats[m_id];
        }
    }

    return hit_infos;
}