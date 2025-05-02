cmake_minimum_required(VERSION 3.28)
project(RayTracerInOneWeekend)

set(CMAKE_OSX_ARCHITECTURES "arm64")
set(CMAKE_CXX_STANDARD 20)

set(GLM ${CMAKE_CURRENT_SOURCE_DIR}/lib/glm)

include_directories(${GLM})
link_directories(${GLM})

add_executable(RayTracerInOneWeekend 
        
        "main.cpp"
        "color.h"
        "ray.h"
        "rtweekend.h"
        "camera.h"
        "camera2.cpp"
        "lib/tiny_obj_loader.h"
        "triangular_light.h"
        "material.h"
        "light_sampler.h"
        "world.cpp"
        "texture.h" 
        "hit_info.h" "tiny_bvh_types.h" "util.cpp" "util.h"
        image_writer.h)

file(COPY ${CMAKE_SOURCE_DIR}/objects DESTINATION ${CMAKE_BINARY_DIR})

# Add the include directory for GLM
target_include_directories(RayTracerInOneWeekend PUBLIC ${GLM})
