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
#include "viewer.h"

std::string get_frame_filename(int i) {
    std::ostringstream oss;
    oss << "frame" << std::setfill('0') << std::setw(4) << i << ".png";
    return oss.str();
}

void render(Camera2 &cam, World &world, int framecount) {
    auto bvh = world.bvh();
    auto lights = world.get_triangular_lights();
    auto mats = world.get_materials();
    auto light_sampler = restir_light_sampler(cam.image_width, cam.image_height, lights);
    for (int i = 0; i < framecount; i++) {
        auto render_start = std::chrono::high_resolution_clock::now();

        // potentially update camera position
        auto hit_infos = cam.get_hit_info_from_camera_per_frame(bvh, mats);

        // send hit infos to ReSTIR
        auto light_samples_per_ray = light_sampler.sample_lights(hit_infos);

        std::vector<std::vector<glm::vec3> > colors = std::vector<std::vector<glm::vec3> >(
            cam.image_height, std::vector<glm::vec3>(cam.image_width, glm::vec3(0.0f)));

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

void render_live(Camera2 &cam, World &world, bool progressive = true) {
    auto bvh = world.bvh();
    auto lights = world.get_triangular_lights();
    auto mats = world.get_materials();
    auto light_sampler = restir_light_sampler(cam.image_width, cam.image_height, lights);

    // 1) Init SDL
    if (!init_sdl()) return;

    // Suppose these are your target dimensions:
    int width = cam.image_width;
    int height = cam.image_height;

    // 2) Create window/renderer/texture
    SDL_Window *window = nullptr;
    SDL_Renderer *renderer = nullptr;
    SDL_Texture *texture = nullptr;
    if (!create_view_window(width, height, window, renderer, texture))
        return;

    // --- Your rendering loop (or however you fill `pixels`) ---
    bool running = true;
    bool camera_moved = false;
    SDL_Event e;

    std::vector<std::vector<glm::vec3> > accumulated_colors =
            std::vector(cam.image_height,
                        std::vector<glm::vec3>(cam.image_width, glm::vec3(0.0f)));
    int frame = 0;

    while (running) {
        // 3) Handle events
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                running = false;
            }

            if (e.type == SDL_KEYDOWN) {
                // Change camera direction
                if (e.key.keysym.sym == SDLK_UP) {
                    cam.position += cam.up * 0.1f;
                } else if (e.key.keysym.sym == SDLK_DOWN) {
                    cam.position -= cam.up * 0.1f;
                } else if (e.key.keysym.sym == SDLK_LEFT) {
                    cam.position -= cam.right * 0.1f;
                } else if (e.key.keysym.sym == SDLK_RIGHT) {
                    cam.position += cam.right * 0.1f;
                } else if (e.key.keysym.sym == SDLK_w) {
                    cam.position += cam.forward * 0.1f;
                } else if (e.key.keysym.sym == SDLK_s) {
                    cam.position -= cam.forward * 0.1f;
                } else if (e.key.keysym.sym == SDLK_p) {
                    progressive = !progressive;
                }
                camera_moved = true;
            }
        }

        if (camera_moved) {
            std::clog << "Camera moved, resetting light sampler" << std::endl;
            light_sampler.reset();
            accumulated_colors =
                    std::vector(cam.image_height,
                                std::vector<glm::vec3>(cam.image_width, glm::vec3(0.0f)));
            frame = 0;
            camera_moved = false;
        }

        auto render_start = std::chrono::high_resolution_clock::now();

        // potentially update camera position
        auto hit_infos = cam.get_hit_info_from_camera_per_frame(bvh, mats);

        // send hit infos to ReSTIR
        auto light_samples_per_ray = light_sampler.sample_lights(hit_infos);

        std::vector<std::vector<glm::vec3> > colors = std::vector<std::vector<glm::vec3> >(
            cam.image_height, std::vector<glm::vec3>(cam.image_width, glm::vec3(0.0f)));

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

        std::clog << "Rendering frame " << frame << " took ";
        std::clog << std::chrono::duration_cast<std::chrono::milliseconds>(render_stop - render_start).count();
        std::clog << " milliseconds" << std::endl;

        // update the accumulated colors
        for (int j = 0; j < cam.image_height; j++) {
            for (int i = 0; i < cam.image_width; i++) {
                accumulated_colors[j][i] = ((accumulated_colors[j][i] * static_cast<float>(frame)) + colors[j][i]) /
                                           static_cast<float>(frame + 1);
            }
        }
        frame++;

        /// output frame
        if (progressive) {
            update_and_present(renderer, texture, accumulated_colors);
        } else {
            update_and_present(renderer, texture, colors);
        }
    }
    // Clean up
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    shutdown_sdl();
}

int main() {
    auto loading_start = std::chrono::high_resolution_clock::now();
    World world;
    world.add_obj("objects/whiteMonkey.obj", false);
    world.add_obj("objects/blueMonkey_rotated.obj", false);

    world.add_obj("objects/multiple_color_lights.obj", true);


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
    // render(cam2, world, 20);
    render_live(cam2, world);
}
