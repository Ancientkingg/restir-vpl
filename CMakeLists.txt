cmake_minimum_required(VERSION 3.30)
project(RayTracerInOneWeekend)

set(CMAKE_OSX_ARCHITECTURES "arm64")
set(CMAKE_CXX_STANDARD 20)

add_executable(RayTracerInOneWeekend main.cpp
        vec3.h
        color.h
        ray.h
        rtweekend.h
        camera.h
        lib/tiny_bvh.h
        lib/tiny_obj_loader.h
        triangular_light.h
        material.h
)