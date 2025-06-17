#pragma once

constexpr auto BASE_LIGHT_INTENSITY = 50.0f;

constexpr auto ENABLE_TEXTURES = false;

constexpr auto N_PHOTONS = 100000;
constexpr auto N_INDIRECT_PHOTONS = 100000;
constexpr auto MAX_BOUNCES = 20;
constexpr auto MIN_BOUNCES = 1;

constexpr auto M_CAP = 20.0f;
constexpr auto NORMAL_DEVIATION = 0.4f;
constexpr auto T_DEVIATION = 0.05f;
constexpr auto NEIGHBOUR_K = 8;
constexpr auto NEIGHBOUR_RADIUS = 20; // pixels

constexpr auto LIVE_WIDTH = 400;
constexpr auto RENDER_WIDTH = 1280;
constexpr auto RENDER_FRAME_COUNT = 5;
constexpr auto SAVE_INTERMEDIATE = false;
constexpr auto SAVE_FORMAT = 1; // 0: png, 1: pfm, 2: both

constexpr float SPHERE_R = 0.025f;

constexpr int MAX_RAY_DEPTH = 5;

extern bool DISABLE_GI;

//#define INTERPOLATE_NORMALS

