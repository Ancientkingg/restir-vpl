cmake_minimum_required(VERSION 3.28)
project(RayTracerInOneWeekend)

set(CMAKE_OSX_ARCHITECTURES "arm64")
set(CMAKE_CXX_STANDARD 20)

set(GLM ${CMAKE_CURRENT_SOURCE_DIR}/lib/glm)

add_executable(RayTracerInOneWeekend main.cpp
        vec3.h
        color.h
        ray.h
        rtweekend.h
        camera.h
        camera2.h
        lib/tiny_bvh.h
        lib/tiny_obj_loader.h
        triangular_light.h
        material.h
        light_sampler.h
        world.cpp
)

# Add the include directory for GLM
target_include_directories(RayTracerInOneWeekend PUBLIC ${GLM})
