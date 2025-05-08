#pragma once  

#include "camera.hpp"  
#include "world.hpp"

void render(Camera& cam, World& world, int framecount);

void render_live(Camera& cam, World& world, bool progressive = true);