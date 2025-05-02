#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

#include "material.h"
#include "rtweekend.h"
#include "camera2.hpp"
#include "world.hpp"
#include "light_sampler.h"

#include <filesystem>

std::string get_frame_filename(int i) {
    std::ostringstream oss;
    oss << "frame" << std::setfill('0') << std::setw(4) << i << ".ppm";
    return oss.str();
}

void render(Camera2& cam, World& world, int framecount){
    auto bvh = world.bvh();
    auto lights = world.get_triangular_lights();
    auto mats = world.get_materials();
    auto light_sampler = restir_light_sampler(cam.image_width, cam.image_height, lights);
    for(int i = 0; i < framecount; i ++){
        // potentially update camera position
        auto hit_infos = cam.get_hit_info_from_camera_per_frame(bvh, mats);
        
        // send hit infos to ReSTIR
        auto light_samples_per_ray = light_sampler.sample_lights(hit_infos, world);

        /// do the color calculation
        // ?????
        // std::vector<std::vector<glm::vec3?>> colors = ???.??(sampler_results, ???);

        /// output frame
        auto filename = get_frame_filename(i);
        // write_image(filename, colors);
    }
}

int main() {
    camera cam;

    cam.aspect_ratio = 16.0 / 9; // Ratio of image width over height
    cam.image_width = 500;
    cam.spp = 100;


    World world;
    world.add_obj("objects/whiteMonkey.obj", false);
    world.add_obj("objects/blueMonkey_rotated.obj", false);

    world.add_obj("objects/bigCubeLight.obj", true);

    tinybvh::bvhvec4 transpose =
            tinybvh::bvhvec4(0.0f, 40.0f, -1.0f, 0.0f);

    // Move the mesh 100 units back in the z direction
    for (int i = 0; i < world.triangle_soup.size(); i++) {
        world.triangle_soup[i] = transpose + world.triangle_soup[i];
    }


    std::vector<triangular_light> lights = world.get_triangular_lights();
    tinybvh::BVH bvh = world.bvh();
    std::vector<material> mats = world.get_materials();

    Camera2 cam2;
    // TODO:
    // render(cam2, bvh); (add more arguments as needed)

    cam.render(bvh, lights, mats, "image.ppm");
}
