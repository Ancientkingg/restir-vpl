#include "material.h"
#include "rtweekend.h"
#include "triangular_light.h"
#include "world.hpp"

int main() {
    camera cam;

    cam.aspect_ratio = 16.0 / 9; // Ratio of image width over height
    cam.image_width = 1000;
    cam.spp = 100;

    material m{
        color(1.f, 1.f, 1.f),
        1.f,
        .0f,
        1.0f
    };


    World *world = new World();
    world->add_obj("objects/suzanne.obj", false);
    world->add_obj("objects/bigCubeLight.obj", true);

    tinybvh::bvhvec4 transpose =
            tinybvh::bvhvec4(0, 40, -1, 0);

    // Move the mesh 100 units back in the z direction
    for (int i = 0; i < world->triangle_soup.size(); i++) {
        world->triangle_soup[i] = transpose + world->triangle_soup[i];
    }


    triangular_light simple_light(
        vec3(10, 60, -2),
        vec3(-10, 60, 0),
        vec3(10, 80, 2),
        color(.0f, .0f, 1.0f),
        1.0f
    );

    triangular_light simple_light_2(
        vec3(10, 20, -2),
        vec3(-10, 20, 0),
        vec3(10, 20, 2),
        color(1.0f, 0.f, 0.f),
        1.0f
    );

    triangular_light simple_light_3(
        vec3(10, 30, 0),
        vec3(-10, 50, 0),
        vec3(10, 30, 0),
        color(1.0f, 1.f, 1.f),
        1.0f
    );

    tinybvh::BVH bvh = world->bvh();
    cam.render(bvh, {simple_light, simple_light_2, simple_light_3}, {m}, "image.ppm");
}
