#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

#include "camera2.h"
#include "material.h"
#include "rtweekend.h"
#include "world.hpp"

std::string get_frame_filename(int i) {
    std::ostringstream oss;
    oss << "frame" << std::setfill('0') << std::setw(4) << i << ".ppm";
    return oss.str();
}

void render(Camera2& cam, tinybvh::BVH& bvh, int framecount){
    for(int i = 0; i < framecount; i ++){
        // potentially update camera position
        auto hit_infos = cam.get_hit_info_from_camera_per_frame(bvh);
        
        /// send hit infos to ReSTIR
        // auto sampler_results = ??.sample_lights(hit_infos, bvh);

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

    World *world = new World();
    world->add_obj("objects/whiteMonkey.obj", false);
    world->add_obj("objects/blueMonkey_rotated.obj", false);

    world->add_obj("objects/bigCubeLight.obj", true);

    tinybvh::bvhvec4 transpose =
            tinybvh::bvhvec4(0, 40, -1, 0);

    // Move the mesh 100 units back in the z direction
    for (int i = 0; i < world->triangle_soup.size(); i++) {
        world->triangle_soup[i] = transpose + world->triangle_soup[i];
    }


    std::vector<triangular_light> lights = world->get_triangular_lights();
    tinybvh::BVH bvh = world->bvh();
    std::vector<material> mats = world->get_materials();

    // TODO:
    // Camera2 cam2;
    // render(cam2, bvh);

    cam.render(bvh, lights, mats, "image.ppm");
}
