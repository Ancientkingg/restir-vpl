#pragma once  

#include "camera.hpp"  
#include "world.hpp"
#include "light_sampler.hpp"

void render(Camera& cam, World& world, int framecount, bool accumulate = false, SamplingMode sampling_mode = SamplingMode::ReSTIR);

void render_live(Camera& cam, World& world, bool progressive = true);