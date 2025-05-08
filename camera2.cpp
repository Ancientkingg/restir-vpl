#include "camera2.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
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

glm::vec3 sky_color(const glm::vec3& direction) {
    float t = 0.5f * (direction.y + 1.0f);
    glm::vec3 top = glm::vec3(0.5f, 0.7f, 1.0f);    // Sky blue
    glm::vec3 bottom = glm::vec3(1.0f);            // Horizon white
    return (1.0f - t) * bottom + t * top;
}

glm::vec3 shade(const hit_info& hit, const sampler_result& sample, float pdf, World& scene) {
    // Ray has no intersection
    if (hit.t == 1E30f) {
        return sky_color(hit.r.direction());
    }

    glm::vec3 ambient = sky_color(hit.normal) * hit.mat_ptr->albedo(hit);

    // Compute cosine between surface normal and light direction
    float cos_theta = glm::dot(hit.normal, sample.light_dir);
    if (cos_theta <= 0.0f || pdf <= 0.0f) {
        // Still apply ambient sky light even if direct light is not contributing
        return ambient * 0.05f;
    }

    glm::vec3 I = hit.r.at(hit.t);
    glm::vec3 L = sample.light_dir;
    float dist = glm::length(sample.light_point - I);

    glm::vec3 Li;

    if (Ray shadow_ray = Ray(I + 0.001f * L, L); scene.is_occluded(shadow_ray, dist)) {
        // Use sky radiance instead if light is occluded
        Li = sky_color(L);
    }
    else {
        // Use direct light sample radiance
        Li = sample.light.intensity * sample.light.c;
    }

    // Evaluate BRDF at the hit point
    glm::vec3 brdf = hit.mat_ptr->evaluate(hit, sample.light_dir);

    // Calculate geometry term for solid angle
    glm::vec3 light_normal = sample.light.normal;
    float cos_theta_prime = glm::dot(-sample.light_dir, light_normal);
    float dist2 = dist * dist;
    float geometry_term = cos_theta_prime / dist2;


    // Final contribution
    glm::vec3 direct = (brdf * Li * cos_theta * geometry_term) / pdf;
    return ambient * direct;
}

// TODO: Cache ray hits (so we dont compute the same thing) when camera does not move

Camera2::Camera2() : Camera2(glm::vec3(0, 40, 1), glm::vec3(0, 40, 0)) {}

Camera2::Camera2(glm::vec3 camera_position, glm::vec3 camera_target)
    : position(camera_position), target(camera_target) {
    direction = glm::normalize(target - position);
    glm::vec3 world_up = glm::vec3(0, 1, 0);
    right = glm::normalize(glm::cross(direction, world_up));
    up = glm::normalize(glm::cross(right, direction));
    forward = glm::normalize(glm::cross(up, right));
}

void Camera2::rotate(float angle, glm::vec3 axis) {
    // Change the direction of the camera and recalculate the right up and forward vectors
    // Create rotation matrix for the direction vector, around the axis, starting at camera pos
    glm::mat4 rotation_matrix = glm::rotate(glm::mat4(1.0f), angle, axis);
    glm::vec4 new_direction = rotation_matrix * glm::vec4(direction, 1.0f);
    direction = glm::normalize(glm::vec3(new_direction));
    right = glm::normalize(glm::cross(direction, up));
    up = glm::normalize(glm::cross(right, direction));
    forward = glm::normalize(glm::cross(up, right));
}

void Camera2::updateDirection() {
    glm::vec3 dir;
    dir.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    dir.y = sin(glm::radians(pitch));
    dir.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    direction = glm::normalize(dir);

    right = glm::normalize(glm::cross(direction, glm::vec3(0.0f, 1.0f, 0.0f))); // world up
    up = glm::normalize(glm::cross(right, direction));
    forward = direction;
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