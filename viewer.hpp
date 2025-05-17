#pragma once  

#include "camera.hpp"  
#include "world.hpp"
#include "restir.hpp"

void render(Camera& cam, World& world, int framecount, bool accumulate = false, SamplingMode sampling_mode = SamplingMode::Uniform);

void render_live(Camera& cam, World& world, bool progressive = true);