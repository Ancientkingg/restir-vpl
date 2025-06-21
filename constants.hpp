#pragma once

constexpr auto BASE_LIGHT_INTENSITY = 50.0f;

constexpr auto ENABLE_TEXTURES = false;

constexpr auto N_PHOTONS = 100000;
constexpr auto N_INDIRECT_PHOTONS = 100000;
constexpr auto MAX_BOUNCES = 8;
constexpr auto MIN_BOUNCES = 0;

constexpr auto M_CAP = 20.0f;
constexpr auto NORMAL_DEVIATION = 0.4f;
constexpr auto T_DEVIATION = 0.05f;
constexpr auto NEIGHBOUR_K = 8;
constexpr auto NEIGHBOUR_RADIUS = 20; // pixels

constexpr auto ASPECT_RATIO = 1.0f;
constexpr auto LIVE_WIDTH = 400;
constexpr auto RENDER_WIDTH = 1280;
constexpr auto RENDER_FRAME_COUNT = 500;
constexpr auto SAVE_INTERMEDIATE = true;
constexpr auto SAVE_INTERVAL = 1;

constexpr float SPHERE_R = 0.025f;

constexpr int MAX_RAY_DEPTH = 8;

extern bool DISABLE_GI;

//#define INTERPOLATE_NORMALS
#define PL_ATTENUATION
