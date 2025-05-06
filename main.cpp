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
#include <chrono>


#include "image_writer.h"

std::string get_frame_filename(int i) {
    std::ostringstream oss;
    oss << "frame" << std::setfill('0') << std::setw(4) << i << ".png";
    return oss.str();
}

void render(Camera2& cam, World& world, int framecount){
    auto bvh = world.bvh();
    auto lights = world.get_triangular_lights();
    auto mats = world.get_materials();
    auto light_sampler = restir_light_sampler(cam.image_width, cam.image_height, lights);
    for(int i = 0; i < framecount; i++){
        auto render_start = std::chrono::high_resolution_clock::now();

        // potentially update camera position
        auto hit_infos = cam.get_hit_info_from_camera_per_frame(bvh, mats);
        
        // send hit infos to ReSTIR
        auto light_samples_per_ray = light_sampler.sample_lights(hit_infos);

		std::vector<std::vector<glm::vec3>> colors = std::vector<std::vector<glm::vec3>>(cam.image_height, std::vector<glm::vec3>(cam.image_width, glm::vec3(0.0f)));

        // loop over hit_infos and light_samples_per_ray at the same time and feed them into the shade 
        #pragma omp parallel for
        for (size_t j = 0; j < hit_infos.size(); j++) {
			if (hit_infos[j].empty()) continue; // No hits for this 
            for (size_t i = 0; i < hit_infos[0].size(); i++) {
				hit_info hit = hit_infos[j][i];
				sampler_result sample = light_samples_per_ray[j][i];

                float pdf = 1.0 / sample.light.area();
				glm::vec3 color = shade(hit, sample, pdf, world);
				colors[j][i] += color;
            }
        }

        auto render_stop = std::chrono::high_resolution_clock::now();

        std::clog << "Rendering frame " << i << " took ";
        std::clog << std::chrono::duration_cast<std::chrono::milliseconds>(render_stop - render_start).count();
        std::clog << " milliseconds" << std::endl;

        /// output frame
        auto filename = get_frame_filename(i);
        save_image(colors, "./images/" + filename);
    }
}

int main() {
    auto loading_start = std::chrono::high_resolution_clock::now();
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

    auto loading_stop = std::chrono::high_resolution_clock::now();

    std::clog << "Loading took ";
    std::clog << std::chrono::duration_cast<std::chrono::milliseconds>(loading_stop - loading_start).count();
    std::clog << " milliseconds" << std::endl;


    Camera2 cam2;
    render(cam2, world, 1);
}
