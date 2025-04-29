#include "material.h"
#include "rtweekend.h"
#include "world.hpp"

int main() {
    camera cam;

    cam.aspect_ratio = 16.0 / 9; // Ratio of image width over height
    cam.image_width = 500;
    cam.spp = 100;

    World *world = new World();
    world->add_obj("objects/whiteMonkey.obj", false);
    world->add_obj("objects/whiteMonkey_rotated.obj", false);

    world->add_obj("objects/twoBigCubeLights.obj", true);

    tinybvh::bvhvec4 transpose =
            tinybvh::bvhvec4(0, 40, -1, 0);

    // Move the mesh 100 units back in the z direction
    for (int i = 0; i < world->triangle_soup.size(); i++) {
        world->triangle_soup[i] = transpose + world->triangle_soup[i];
    }


    std::vector<triangular_light> lights = world->get_triangular_lights();
    tinybvh::BVH bvh = world->bvh();
    std::vector<material> mats = world->get_materials();

    cam.render(bvh, lights, mats, "image.ppm");
}
