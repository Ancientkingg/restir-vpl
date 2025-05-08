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
        for (int j = 0; j < hit_infos.size(); j++) {
            if (hit_infos[j].empty()) continue; // No hits for this
            for (int i = 0; i < hit_infos[0].size(); i++) {
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

const float moveSpeed = 0.5f;
const float mouseSensitivity = 0.02f;

int mouseDeltaX = 0;
int mouseDeltaY = 0;

struct KeyState {
    bool w = false;
    bool a = false;
    bool s = false;
    bool d = false;
    bool space = false;
    bool shift = false;
    bool ctrl = false;
};

View view = VIEW_SHADING;

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

    // Handle key input such as combos
    KeyState keys;
    SDL_SetRelativeMouseMode(SDL_TRUE);

    std::clog << "=======================================================" << "\r\n";

    while (running) {
        // 3) Handle events
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                running = false;
            }

            if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP) {
                const bool isDown = (e.type == SDL_KEYDOWN);
                SDL_Keycode key = e.key.keysym.sym;

                switch (key) {
				case SDLK_ESCAPE:
					running = false;
					break;
                case SDLK_w: keys.w = isDown; break;
                case SDLK_a: keys.a = isDown; break;
                case SDLK_s: keys.s = isDown; break;
                case SDLK_d: keys.d = isDown; break;
                case SDLK_SPACE: keys.space = isDown; break;
                case SDLK_LCTRL: keys.ctrl = isDown; break;
				case SDLK_LSHIFT: keys.shift = isDown; break;
				case SDLK_b: view = VIEW_DEBUG; break;
				case SDLK_n: view = VIEW_NORMALS; break;
				case SDLK_v: view = VIEW_SHADING; break;
                case SDLK_p:
                    if (isDown) progressive = !progressive;
                    break;
                default: break;
                }
            }

            if (e.type == SDL_MOUSEMOTION) {
                mouseDeltaX += e.motion.xrel;
                mouseDeltaY += e.motion.yrel;
            }
        }

        // Apply camera movement
        glm::vec3 movement(0.0f);
        if (keys.w) movement += cam.forward;
        if (keys.s) movement -= cam.forward;
        if (keys.a) movement -= cam.right;
        if (keys.d) movement += cam.right;
        if (keys.space) movement += glm::vec3(0.0f,1.0f,0.0f);
        if (keys.ctrl) movement -= glm::vec3(0.0f, 1.0f, 0.0f);

		if (keys.shift) movement *= 7.0f; // Speed up when shift is pressed

        if (glm::length(movement) > 0.0f) {
            cam.position += glm::normalize(movement) * moveSpeed;
            camera_moved = true;
        }

        // Apply camera rotation from mouse
        if (mouseDeltaX != 0 || mouseDeltaY != 0) {
            cam.yaw += mouseDeltaX * mouseSensitivity;
            cam.pitch -= mouseDeltaY * mouseSensitivity;

            // Clamp pitch to prevent flipping
            cam.pitch = glm::clamp(cam.pitch, -89.0f, 89.0f);

            cam.updateDirection(); // Recalculate vectors
            camera_moved = true;

            mouseDeltaX = 0;
            mouseDeltaY = 0;
        }

        if (camera_moved) {
            //std::clog << "Camera moved, resetting light sampler" << std::endl;
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
        for (int j = 0; j < hit_infos.size(); j++) {
            if (hit_infos[j].empty()) continue; // No hits for this
            for (int i = 0; i < hit_infos[0].size(); i++) {
                hit_info hit = hit_infos[j][i];
                sampler_result sample = light_samples_per_ray[j][i];

                float pdf = sample.light.pdf;

                glm::vec3 color;
				if (view == VIEW_SHADING) {
					color = shade(hit, sample, pdf, world);
				}
				else if (view == VIEW_DEBUG) {
					color = shade_debug(hit, sample, pdf, world);
				}
				else if (view == VIEW_NORMALS) {
					color = shade_normal(hit, sample, pdf, world);
				}

                colors[j][i] += color;
            }
        }

        auto render_stop = std::chrono::high_resolution_clock::now();

        float duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(render_stop - render_start).count();

        std::clog << "Frame " << frame
            << " | Time: " << duration_ms << " ms"
            << " | Camera: (" << cam.position.x << ", " << cam.position.y << ", " << cam.position.z << ")"
			<< " | View: " << ((view == VIEW_SHADING) ? "Shading" : (view == VIEW_DEBUG) ? "Debug" : "Normals")
            << "         \r" << std::flush;

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

int main(int argc, char* argv[]) {
    auto loading_start = std::chrono::high_resolution_clock::now();
    World world;
//     world.add_obj("objects/whiteMonkey.obj", false);
//     world.add_obj("objects/blueMonkey_rotated.obj", false);
// 
    //world.add_obj("objects/bigCubeLight.obj", true);
    world.add_obj("objects/bistro_normal.obj", false);
    world.place_obj("objects/bigCubeLight.obj", true, glm::vec3(3, 23, -27));
    //world.add_obj("objects/bistro_lights.obj", true);


    tinybvh::bvhvec4 transpose =
            tinybvh::bvhvec4(0.0f, 40.0f, -1.0f, 0.0f);

    // Move the mesh 100 units back in the z direction
    //for (int i = 0; i < world.triangle_soup.size(); i++) {
    //    world.triangle_soup[i] = transpose + world.triangle_soup[i];
    //}

    //for (int i = 0; i < world.lights.size(); i++) {
    //    world.lights[i] = transpose + world.lights[i];
    //}

    auto loading_stop = std::chrono::high_resolution_clock::now();

    std::clog << "Loading took ";
    std::clog << std::chrono::duration_cast<std::chrono::milliseconds>(loading_stop - loading_start).count();
    std::clog << " milliseconds" << std::endl;


    Camera2 cam2;
    // render(cam2, world, 20);
    render_live(cam2, world);

    return 0;
}
