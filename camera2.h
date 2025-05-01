#ifndef CAMERA2_H_
#define CAMERA2_H_

#include <glm/glm.hpp>
#include <iostream>
#include <vector>

#include "ray.h"
#include "material.h"
#include "hit_info.h"


tinybvh::Ray toBVHRay(const Ray& r) {
    return tinybvh::Ray(toBVHVec(r.origin()), toBVHVec(r.direction()));
}
// TODO: Cache ray hits (so we dont compute the same thing) when camera does not move

class Camera2 {
    public:

    Camera2(): Camera2(glm::vec3(0, 40, 1), glm::vec3(0, 40, 0)) {}

    Camera2(glm::vec3 camera_position, glm::vec3 camera_target)
        : position(camera_position), target(camera_target) {
        direction = glm::normalize(target - position); // Forward vector
        right = glm::normalize(
            glm::cross(glm::vec3(0, 1, 0), direction));  // Right vector
        up = glm::cross(direction, right);  // Up vector in camera space
    }

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

    std::vector<std::vector<Ray>> generate_rays_for_frame(){
        std::vector<std::vector<Ray>> rays(image_height, std::vector<Ray>(image_width));

        for(int i = 0; i < image_height; i++){
            for(int j = 0; j < image_width; j++){
                glm::vec3 pixel_direction = direction -
                    (float(i) / image_width - 0.5f) * 2.f * tanfov * image_width /
                        image_height * right -
                    (0.5f - float(j) / image_height) * 2.f * tanfov * up;

                rays[i][j] = Ray(position, glm::normalize(pixel_direction));
            }
        }

        return rays;
    }

    std::vector<std::vector<hit_info>> get_hit_info_from_camera_per_frame(
        tinybvh::BVH& bvh, std::vector<material>& mats) {
        std::vector<std::vector<hit_info>> hit_infos(
            image_height, std::vector<hit_info>(image_width));

        auto rays = generate_rays_for_frame();

        for(int i = 0; i < image_height; i++){
            for(int j = 0; j < image_width; j++){
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
};

#endif // CAMERA2_H_