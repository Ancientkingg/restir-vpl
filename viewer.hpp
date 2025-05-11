#pragma once  

#include "camera.hpp"  
#include "world.hpp"

void render(Camera& cam, World& world, int framecount, bool accumulate = false);

void render_live(Camera& cam, World& world, bool progressive = true);