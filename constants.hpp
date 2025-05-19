#pragma once

constexpr auto BASE_LIGHT_INTENSITY = 15.0f;

constexpr auto ENABLE_TEXTURES = false;

constexpr auto M_CAP = 20.0f;
constexpr auto NORMAL_DEVIATION = 0.4f;
constexpr auto T_DEVIATION = 0.05f;
constexpr auto NEIGHBOUR_K = 8;
constexpr auto NEIGHBOUR_RADIUS = 10; // pixels

constexpr auto LIVE_WIDTH = 400;
constexpr auto RENDER_WIDTH = 1920;
constexpr auto RENDER_FRAME_COUNT = 1;
constexpr auto SAVE_INTERMEDIATE = false;
constexpr auto SAVE_FORMAT = 1; // 0: png, 1: pfm, 2: both

//#define INTERPOLATE_NORMALS

