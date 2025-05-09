#define TINYBVH_IMPLEMENTATION
#include "lib/tiny_bvh.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "lib/tiny_obj_loader.h"

#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

#include "camera.hpp"
#include "world.hpp"
#include "viewer.hpp"


int main(int argc, char* argv[]) {
    World world = load_world();

    Camera cam;
    // render(cam, world, 20);
    render_live(cam, world);

    return 0;
}
