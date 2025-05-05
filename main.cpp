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

// void write_image(std::vector<std::vector<glm::vec3>> colors) {
//     // write vector as ppm to image file
// 	std::ofstream out("image.ppm");
// 	out << "P3\n" << colors[0].size() << ' ' << colors.size() << "\n255\n";
// 	for (const auto& row : colors) {
// 		for (const auto& color : row) {
// 			int r = static_cast<int>(std::clamp(color.r * 255.0f, 0.0f, 255.0f));
// 			int g = static_cast<int>(std::clamp(color.g * 255.0f, 0.0f, 255.0f));
// 			int b = static_cast<int>(std::clamp(color.b * 255.0f, 0.0f, 255.0f));
// 			out << r << ' ' << g << ' ' << b << '\n';
// 		}
// 	}
// 	out.close();
//
// 	std::cout << "Image saved as image.ppm" << std::endl;
// }

void render(Camera2& cam, World& world, int framecount){
    auto bvh = world.bvh();
    auto lights = world.get_triangular_lights();
    auto mats = world.get_materials();
    auto light_sampler = restir_light_sampler(cam.image_width, cam.image_height, lights);
    for(int i = 0; i < framecount; i++){
        // potentially update camera position
        auto hit_infos = cam.get_hit_info_from_camera_per_frame(bvh, mats);
        
        // send hit infos to ReSTIR
        auto light_samples_per_ray = light_sampler.sample_lights(hit_infos, world);

		std::vector<std::vector<glm::vec3>> colors = std::vector<std::vector<glm::vec3>>(cam.image_height, std::vector<glm::vec3>(cam.image_width, glm::vec3(0.0f)));

        // loop over hit_infos and light_samples_per_ray at the same time and feed them into the shade 
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

        /// output frame
        auto filename = get_frame_filename(i);
        save_image(colors, "./images/" + filename);
    }
}

int main() {
    camera cam;

    cam.aspect_ratio = 16.0 / 9; // Ratio of image width over height
    cam.image_width = 500;
    cam.spp = 100;

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

    std::cout << "Loading took ";
    std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(loading_stop - loading_start).count();
    std::cout << " milliseconds" << std::endl;


    std::vector<triangular_light> lights = world.get_triangular_lights();
    tinybvh::BVH bvh = world.bvh();
    std::vector<Material *> mats = world.get_materials();

    Camera2 cam2;
    // TODO:
    // render(cam2, bvh); (add more arguments as needed)

    auto render_start = std::chrono::high_resolution_clock::now();

    render(cam2, world, 1);

    auto render_stop = std::chrono::high_resolution_clock::now();

    std::cout << "Rendering took ";
    std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(render_stop - render_start).count();
    std::cout << " milliseconds" << std::endl;

    //cam.render(bvh, lights, mats, "image.ppm");
}
