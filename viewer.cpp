#include "viewer.hpp"

#include <SDL2/SDL.h>
#include <glm/glm.hpp>
#include <vector>
#include <iostream>
#include <chrono>
#include <fstream>
#include <filesystem>

#include "shading.hpp"
#include "image_writer.hpp"
#include "render.hpp"
#include "restir.hpp"
#include "camera.hpp"
#include "world.hpp"
#include "constants.hpp"

// Call this once at program start:
bool init_sdl() {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << "\n";
        return false;
    }
    return true;
}

// Call this at program end:
void shutdown_sdl() {
    SDL_Quit();
}

// Creates an SDL window + renderer + texture for live‐viewing.
// Returns true on success, and fills out the handles.
bool create_view_window(int width, int height,
                        SDL_Window *&outWindow,
                        SDL_Renderer *&outRenderer,
                        SDL_Texture *&outTexture) {
    outWindow = SDL_CreateWindow("Live View",
                                 SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                 width, height,
                                 SDL_WINDOW_SHOWN);
    if (!outWindow) {
        std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << "\n";
        return false;
    }

    outRenderer = SDL_CreateRenderer(outWindow, -1, SDL_RENDERER_ACCELERATED);
    if (!outRenderer) {
        std::cerr << "SDL_CreateRenderer Error: " << SDL_GetError() << "\n";
        return false;
    }

    // Note: SDL_PIXELFORMAT_RGB24 expects 3 bytes per pixel, R8G8B8
    outTexture = SDL_CreateTexture(outRenderer,
                                   SDL_PIXELFORMAT_RGB24,
                                   SDL_TEXTUREACCESS_STREAMING,
                                   width, height);
    if (!outTexture) {
        std::cerr << "SDL_CreateTexture Error: " << SDL_GetError() << "\n";
        return false;
    }

    return true;
}

// Call once per frame after you've filled `pixels`:
void update_and_present(SDL_Renderer *renderer,
                        SDL_Texture *texture,
                        const std::vector<std::vector<glm::vec3> > &pixels) {
    int height = int(pixels.size());
    int width = int(pixels[0].size());

    // Lock texture to get raw pointer
    void *texPixels;
    int pitch; // bytes per row
    SDL_LockTexture(texture, nullptr, &texPixels, &pitch);

    // pitch should equal width * 3 for RGB24, but we use it in case
    unsigned char *dst = static_cast<unsigned char *>(texPixels);

    // Copy rows bottom-up if you want to flip vertically,
    // otherwise just iterate y from 0→height.
    for (int y = 0; y < height; ++y) {
        unsigned char *row = dst + y * pitch;
        for (int x = 0; x < width; ++x) {
            glm::vec3 c = glm::clamp(pixels[height - 1 - y][x], 0.0f, 1.0f);
            int idx = x * 3;
            row[idx + 0] = static_cast<unsigned char>(c.r * 255.0f);
            row[idx + 1] = static_cast<unsigned char>(c.g * 255.0f);
            row[idx + 2] = static_cast<unsigned char>(c.b * 255.0f);
        }
    }

    SDL_UnlockTexture(texture);

    // Render it
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, nullptr, nullptr);
    SDL_RenderPresent(renderer);
}

std::string get_frame_filename(int i) {
    std::ostringstream oss;
    oss << "frame" << std::setfill('0') << std::setw(4) << i;
    return oss.str();
}

void accumulate(std::vector<std::vector<glm::vec3>>& colors, std::vector<std::vector<glm::vec3>>& new_color, int frame) {
	for (int j = 0; j < colors.size(); j++) {
		for (int i = 0; i < colors[j].size(); i++) {
			colors[j][i] = (colors[j][i] * static_cast<float>(frame) + new_color[j][i]) /
				static_cast<float>(frame + 1);
		}
	}
}

bool currently_outputting_render = false;
float avg_time = 0.0f; // ms
int total_frames = 1;
bool ENABLE_PT = false;

static void progress_bar(int current_frame, float time, int framecount) {
	avg_time = (avg_time * total_frames + time) / (total_frames + 1);
	total_frames++;

	int barWidth = 70;
	std::cout << "[";
	float progress = static_cast<float>(current_frame + 1) / static_cast<float>(framecount);
	progress = fmin(progress, 1.0f);
	int pos = barWidth * progress;
	for (int i = 0; i < barWidth; ++i) {
		if (i < pos) std::cout << "=";
		else if (i == pos) std::cout << ">";
		else std::cout << " ";
	}
	float estimated_time = avg_time * (framecount - total_frames); // ms
	int minutes = static_cast<int>(estimated_time / 60000);
	int seconds = static_cast<int>((estimated_time - (minutes * 60000)) / 1000);
	std::cout << "] " 
        << int(progress * 100.0f) << "%" 
        << " | Frame: " << std::setfill('0') << std::setw(2) << current_frame << "/" << framecount 
        << " | ETL: " 
            << std::setfill('0') << std::setw(2) << minutes << "m" 
            << std::setfill('0') << std::setw(2) << seconds << "s\r";
	std::cout.flush();
}

void render(Camera &cam, World &world, int framecount, bool accumulate_flag, SamplingMode sampling_mode, ShadingMode shading_mode) {
    if (shading_mode != RENDER_SHADING) {
        framecount = 1;
    }

    Camera render_cam = Camera(cam.position, cam.target);
    render_cam.image_width = RENDER_WIDTH;
    const float x = int(RENDER_WIDTH / render_cam.aspect_ratio);
    render_cam.image_height = (x < 1) ? 1 : x;

    render_cam.yaw = cam.yaw;
    render_cam.pitch = cam.pitch;
    render_cam.updateDirection();

    // Build the world and load materials
    world.bvh();
    world.get_materials(!ENABLE_TEXTURES);

    auto lights = world.get_lights();
    auto light_sampler = RestirLightSampler(render_cam.image_width, render_cam.image_height, lights);
    light_sampler.sampling_mode = sampling_mode;
    light_sampler.m = 32;

    std::vector<std::vector<glm::vec3> > accumulated_colors;
    if (accumulate_flag) {
        accumulated_colors =
            std::vector(render_cam.image_height,
                std::vector<glm::vec3>(render_cam.image_width, glm::vec3(0.0f)));
    }

    std::string sampling_mode_str;
    std::ostringstream oss;
    oss << light_sampler.sampling_mode;
    sampling_mode_str = oss.str();

    if (ENABLE_PT) {
        sampling_mode_str = "PT";
    }

	// Reset the progress bar
    avg_time = 0.0f; // ms
    total_frames = 1;

    // generate short timestamp
    const auto id = std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count() % 1000000);

    // create folder with name id

	std::string folder_path = "./images/" + id + "/";
    if (!std::filesystem::exists(folder_path)) {
        std::filesystem::create_directories(folder_path);
	}

    std::ofstream duration_file("./images/" + id + "/durations.csv", std::ios::app);

    for (int i = 0; i < framecount; i++) {
        auto render_start = std::chrono::high_resolution_clock::now();

        RenderInfo info = RenderInfo{render_cam, world, light_sampler};

        std::vector<std::vector<glm::vec3>> colors;

        if (!ENABLE_PT) {
            colors = raytrace(light_sampler.sampling_mode, shading_mode, info);
        }
        else {
            colors = pathtrace(info);
        }

		if (accumulate_flag) {
			accumulate(accumulated_colors, colors, i);
		}

        auto render_stop = std::chrono::high_resolution_clock::now();

        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(render_stop - render_start).count();

        // Save duration to file in csv format
        if (duration_file.is_open()) {
            duration_file << duration;
            if (i < framecount - 1) {
				duration_file << ","; // Add comma if not the last frame
            }
        } else {
            std::cerr << "Could not open durations file for writing." << std::endl;
		}

		progress_bar(i, duration, framecount);

        // output frame
        const auto filename = get_frame_filename(i);

        // Add this flag because PFM is big
        if constexpr (SAVE_INTERMEDIATE == true) {
            if (i % SAVE_INTERVAL == 0) {
                if (accumulate_flag) {
                    save_pfm(accumulated_colors, "./images/" + id + "/" + sampling_mode_str + "_" + filename + ".pfm");
                }
                else {
                    save_pfm(colors, "./images/" + id + "/" + sampling_mode_str + "_" + filename + ".pfm");
                }
            }
        }
    }

	//if (accumulate_flag) {
    std::clog << "Output accumulated frame" << std::endl;
    auto filename = get_frame_filename(framecount);

    save_pfm(accumulated_colors, "./images/" + id + "/accumulate_" + sampling_mode_str + "_" + filename + ".pfm");
	//}

    if (duration_file.is_open()) {
        duration_file.close();
	}

    currently_outputting_render = false;
}

struct KeyState {
    bool w = false;
    bool a = false;
    bool s = false;
    bool d = false;
    bool l = false;
    bool o = false;
    bool i = false;
    bool space = false;
    bool shift = false;
    bool ctrl = false;
    bool enter = false;
    bool equals = false;
    bool backspace = false;
};

void render_live(Camera &cam, World &world, bool progressive) {
    ShadingMode render_mode = RENDER_SHADING;
    const float moveSpeed = 0.5f;
    const float mouseSensitivity = 0.02f;

    int mouseDeltaX = 0;
    int mouseDeltaY = 0;

    // Build the world and load materials
    world.bvh();
    world.get_materials(!ENABLE_TEXTURES);

    auto lights = world.get_lights();
    auto light_sampler = RestirLightSampler(cam.image_width, cam.image_height, lights);

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
        const Uint8 *state = SDL_GetKeyboardState(NULL);

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
                    case SDLK_1:
                        light_sampler.sampling_mode = SamplingMode::Uniform; camera_moved = true; ENABLE_PT = false; break;
                    case SDLK_2:
                        light_sampler.sampling_mode = SamplingMode::RIS; camera_moved = true; ENABLE_PT = false; break;
                    case SDLK_3:
                        light_sampler.sampling_mode = SamplingMode::ReSTIR; camera_moved = true; ENABLE_PT = false; break;
                    case SDLK_4:
                        ENABLE_PT = true; camera_moved = true; break;
                    case SDLK_w: keys.w = isDown;
                        break;
                    case SDLK_a: keys.a = isDown;
                        break;
                    case SDLK_s: keys.s = isDown;
                        break;
                    case SDLK_d: keys.d = isDown;
                        break;
                    case SDLK_o: keys.o = isDown;
                        break;
					case SDLK_i: keys.i = isDown;
						break;
                    case SDLK_SPACE: keys.space = isDown;
                        break;
                    case SDLK_LCTRL: keys.ctrl = isDown;
                        break;
                    case SDLK_LSHIFT: keys.shift = isDown;
                        break;
                    case SDLK_KP_ENTER: keys.enter = isDown;
                        break;
					case SDLK_RETURN: keys.enter = isDown;
						break;
                    case SDLK_EQUALS: keys.equals = isDown;
                        break;
                    case SDLK_l: keys.l = isDown;
                        break;
                    case SDLK_BACKSPACE: keys.backspace = isDown;
                        break;
					case SDLK_g: 
                        if (isDown) {
                            DISABLE_GI = !DISABLE_GI;
                            world.point_lights.clear();
                            world.vpls.clear();
                            lights = world.get_lights();
                            auto mode = light_sampler.sampling_mode;
                            light_sampler = RestirLightSampler(cam.image_width, cam.image_height, lights);
							light_sampler.sampling_mode = mode;
							camera_moved = true;
                        }
						break;
                    case SDLK_b: render_mode = RENDER_DEBUG; ENABLE_PT = false; camera_moved = true;
                        break;
                    case SDLK_n: render_mode = RENDER_NORMALS; ENABLE_PT = false; camera_moved = true;
                        break;
					case SDLK_v: render_mode = RENDER_SHADING; camera_moved = true;
                        break;
                    case SDLK_UP: 
                        if (isDown) {
                            light_sampler.m++; light_sampler.reset();
                        }
                        break;
                    case SDLK_DOWN: 
                        if (isDown) {
                            light_sampler.m--; light_sampler.reset();
                        }
                        break;
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

		light_sampler.m = std::max(1, light_sampler.m);

        if (keys.enter && !currently_outputting_render) {
			// Render the current frame
			std::clog << "\nOutput render with current camera" << std::endl;
            currently_outputting_render = true;
            render(cam, world, RENDER_FRAME_COUNT, progressive, light_sampler.sampling_mode, render_mode);
			keys.enter = false;
        }

        if (keys.backspace) {
            // Remove most recently spawned light
            std::clog << "\nRemoving most recently spawned light" << "\n";
            world.remove_last_point_light();
            auto lights = world.get_lights();
            light_sampler = RestirLightSampler(cam.image_width, cam.image_height, lights);
            light_sampler.reset();
            keys.backspace = false;
        }

        if (keys.l) {
            // Spawn a point light
            std::clog << "\nSpawning point light at: " << cam.position.x << ", "
                << cam.position.y << ", " << cam.position.z << "\n";
            // generate random color
            glm::vec3 color = glm::vec3(1.0f, 1.0f, 1.0f);
            world.spawn_point_light(cam.position, cam.forward, color, 1.0f);
            auto lights = world.get_lights();
            light_sampler = RestirLightSampler(cam.image_width, cam.image_height, lights);
            light_sampler.reset();
            keys.l = false;
        }


        if (keys.equals && !currently_outputting_render) {
			// Render the current frame
			std::clog << "\nOutput ground truth render with current camera: DISABLED" << std::endl;
            currently_outputting_render = true;
            keys.equals = false;
        }

        if (keys.o) {
            // Save current position and orientation to a file
			std::clog << "\nSaving current camera position and orientation to file" << std::endl;

			cam.save_to_file("camera_position.txt");
			keys.o = false;
        }

        if (keys.i) {
            // Load camera position and orientation from file
            std::clog << "\nLoading camera position and orientation from file" << std::endl;
            cam.load_from_file("camera_position.txt");
            cam.updateDirection(); // Recalculate vectors
            camera_moved = true;
        }

        // Apply camera movement
        glm::vec3 movement(0.0f);
        if (keys.w) movement += cam.forward;
        if (keys.s) movement -= cam.forward;
        if (keys.a) movement -= cam.right;
        if (keys.d) movement += cam.right;
        if (keys.space) movement += glm::vec3(0.0f, 1.0f, 0.0f);
        if (keys.ctrl) movement -= glm::vec3(0.0f, 1.0f, 0.0f);

        float sprintSpeed = 1.0f;
        if (keys.shift) {
            sprintSpeed *= 6.0f;
        }

        if (glm::length(movement) > 0.0f) {
            cam.position += glm::normalize(movement) * moveSpeed * sprintSpeed;
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

        RenderInfo info = RenderInfo{cam, world, light_sampler};

        std::vector<std::vector<glm::vec3>> colors;

        if (!ENABLE_PT) {
            colors = raytrace(light_sampler.sampling_mode, render_mode, info);
        }
        else {
			colors = pathtrace(info);
            //colors = raytrace(light_sampler.sampling_mode, RENDER_NORMALS, info);
        }

        auto render_stop = std::chrono::high_resolution_clock::now();

        float duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(render_stop - render_start).count();

        std::string sampling_mode_str = light_sampler.sampling_mode == SamplingMode::Uniform ? "Uniform" :
			(light_sampler.sampling_mode == SamplingMode::RIS ? "RIS    " : "ReSTIR ");

        if (ENABLE_PT) {
			sampling_mode_str = "PT";
        }

        std::clog << "Frame " << frame
                << " | Time: " << duration_ms << " ms"
                << " | View: " << ((render_mode == RENDER_SHADING)
                                       ? "Shading"
                                       : (render_mode == RENDER_DEBUG)
                                             ? "Debug  "
                                             : "Normals")
			    << " | M: " << light_sampler.m
			    << " | Sampling Mode: " << sampling_mode_str
                << " | Camera: (" << std::fixed << std::setprecision(2) << cam.position.x << ", " << cam.position.y << ", " << cam.position.z << ")"
                << "    \r" << std::flush;

        // update the accumulated colors
        if (progressive) {
		    accumulate(accumulated_colors, colors, frame);
            frame++;
        }
        else {
            accumulated_colors =
                std::vector(cam.image_height,
                    std::vector<glm::vec3>(cam.image_width, glm::vec3(0.0f)));
            frame = 0;
        }
        accumulate(accumulated_colors, colors, frame);
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
