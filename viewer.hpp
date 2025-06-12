#pragma once  

#include "camera.hpp"  
#include "world.hpp"
#include "restir.hpp"
#include "shading.hpp"

void render(Camera& cam, World& world, int framecount, bool accumulate = false, SamplingMode sampling_mode = SamplingMode::Uniform, ShadingMode shading_mode = ShadingMode::RENDER_SHADING);

void render_live(Camera& cam, World& world, bool progressive = true);