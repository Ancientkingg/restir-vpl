//
// Created by Rafayel Gardishyan on 23/04/2025.
//

#ifndef CAMERA_H
#define CAMERA_H
#include <iostream>

#include "color.h"
#include "ray.h"
#include "rtweekend.h"
#include "triangular_light.h"
#include "glm/vec3.hpp"
#include "lib/tiny_bvh.h"
#include "tiny_bvh_types.h"

inline triangular_light sample_light(const std::vector<triangular_light>& lights) {
    // Sample a random light from the list (uniform)
    int index = rand() % lights.size();
    return lights[index];
}

class camera {
public:
    /* Public Camera Parameters Here */
    float aspect_ratio = 16.0 / 9; // Ratio of image width over height
    int image_width = 800; // Rendered image width in pixel count
    int spp = 1; // Samples per pixel
    glm::vec3 ambient = glm::vec3(0.1f, 0.1f, 0.1f); // Ambient light color

    camera() {
        initialize();
    }

    void render(tinybvh::BVH& bvh, const std::vector<triangular_light>& lights,
        const std::vector<material>& materials,
        const std::string& file_name = "image.ppm") {
        initialize();

        std::cout << "P3\n" << image_width << ' ' << image_height << "\n255\n";
        for (int j = 0; j < image_height; j++) {
            std::clog << "\rScanlines remaining: " << (image_height - j) << ' ' << std::flush;
            for (int i = 0; i < image_width; i++) {
                glm::vec3 pixel_color{ 0, 0, 0 };

                for (int s = 0; s < spp; s++) {
                    // Generate a random offset for jittering
                    float u = (i + (float(rand()) / RAND_MAX)) / image_width;
                    float v = (j + (float(rand()) / RAND_MAX)) / image_height;

                    // Render one ray per pixel
                    pixel_color += render_one_ray(bvh, lights, materials, i, j);
                }
                // Average the color over the number of samples
                pixel_color /= float(spp);

                write_color(std::cout, pixel_color);


                // Calculate the color based on the hit point and normal
                // write_color(std::cout, ambient + Li);

                // color debug = color{fabs(triangle_normal.x()), fabs(triangle_normal.y()), fabs(triangle_normal.z())};
                // write_color(std::cout, debug);
            }
        }

        std::clog << "\rDone.                 \n";
    }

private:
    int image_height{}; // Rendered image height
    glm::vec3 center; // Camera center
    glm::vec3 pixel00_loc; // Location of pixel 0, 0
    glm::vec3 pixel_delta_u; // Offset to pixel to the right
    glm::vec3 pixel_delta_v; // Offset to pixel below

    glm::vec3 render_one_ray(
        tinybvh::BVH& bvh,
        const std::vector<triangular_light>& lights,
        const std::vector<material>& materials,
        int i, int j) {
        // for now using only one material
        // get material at index 0
        // auto m = materials.at(0);


        auto pixel_center = pixel00_loc + (float(i) * pixel_delta_u) + (float(j) * pixel_delta_v);
        auto ray_direction = glm::normalize(pixel_center - center);
        tinybvh::Ray r(toBVHVec(center), toBVHVec(ray_direction));

        bvh.Intersect(r);

        auto hit_t = r.hit.t;

        if (hit_t == 1E30f) {
            return ambient; // No intersection, return ambient color
        }

        auto hit_point = Ray(center, ray_direction).at(hit_t);

        const glm::vec3 triangle[3] = {
            toGLMVec(bvh.verts[r.hit.prim * 3 + 0]),
            toGLMVec(bvh.verts[r.hit.prim * 3 + 1]),
            toGLMVec(bvh.verts[r.hit.prim * 3 + 2])
        };
        auto triangle_normal = glm::normalize(cross(triangle[1] - triangle[0], triangle[2] - triangle[0]));

        if (dot(triangle_normal, ray_direction) > 0.0f)
            triangle_normal = -triangle_normal;

        auto light = sample_light(lights);
        glm::vec3 light_position = sample_on_light(light);

        auto light_direction = glm::normalize(light_position - hit_point);

        // //shoot a “shadow” ray to see if the light is visible
        // tinybvh::Ray shadow(
        //     toBVHVec(hit_point + (triangle_normal * 1e-4f)), // avoid self‐intersection
        //     toBVHVec(light_direction)
        // );
        // bvh.Intersect(shadow);
        //
        // // if (shadow.hit.t < (light_position - hit_point).length()) {
        // //     return m.c; // Shadowed
        // // }


        // L = k_d * I * max(0, N . L) + k_s * I * max(0, R . H)^p
        auto h = glm::normalize(light_direction + ray_direction);

        int m_id = bvh.verts[r.hit.prim * 3][3];
        material m = materials[m_id];

        glm::vec3 L_d = m.k_d * light.intensity * light.c * std::max(0.0f, dot(triangle_normal, light_direction));
        glm::vec3 L_s = m.k_s * light.intensity * light.c * std::pow(std::max(0.0f, dot(triangle_normal, h)), m.p);
        glm::vec3 L = L_d + L_s;

        return m.c * L;
    }

    void initialize() {
        image_height = int(image_width / aspect_ratio);
        image_height = (image_height < 1) ? 1 : image_height;

        center = glm::vec3(0, 40, 1);

        // Determine viewport dimensions.
        auto focal_length = 1.0;
        auto viewport_height = 2.0;
        auto viewport_width = viewport_height * (float(image_width) / image_height);

        // Calculate the vectors across the horizontal and down the vertical viewport edges.
        auto viewport_u = glm::vec3(viewport_width, 0, 0);
        auto viewport_v = glm::vec3(0, -viewport_height, 0);

        // Calculate the horizontal and vertical delta vectors from pixel to pixel.
        pixel_delta_u = viewport_u / float(image_width);
        pixel_delta_v = viewport_v / float(image_height);

        // Calculate the location of the upper left pixel.
        auto viewport_upper_left =
            center - glm::vec3(0, 0, focal_length) - viewport_u / 2.0f - viewport_v / 2.0f;
        pixel00_loc = viewport_upper_left + 0.5f * (pixel_delta_u + pixel_delta_v);
    }
};

#endif //CAMERA_H
