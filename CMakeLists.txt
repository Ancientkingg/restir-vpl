cmake_minimum_required(VERSION 3.28)
project(RayTracerInOneWeekend)

set(CMAKE_OSX_ARCHITECTURES "arm64")
set(CMAKE_CXX_STANDARD 20)

find_package(OpenMP REQUIRED)

find_package(SDL2 REQUIRED)
include_directories(SDL2Test ${SDL2_INCLUDE_DIRS})

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
        image_writer.h
        viewer.h
)

file(COPY ${CMAKE_SOURCE_DIR}/objects DESTINATION ${CMAKE_BINARY_DIR})

# Add the include directory for GLM
target_include_directories(RayTracerInOneWeekend PUBLIC ${GLM})

target_link_libraries(RayTracerInOneWeekend PUBLIC OpenMP::OpenMP_CXX ${SDL2_LIBRARIES})

message(STATUS "Host processor: ${CMAKE_HOST_SYSTEM_PROCESSOR}")
# Determine if we're building for ARM
if(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "aarch64" OR CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "arm64")
    include(CheckCXXCompilerFlag)

    # Check for the ARMv8 SIMD flag
    check_cxx_compiler_flag("-march=armv8-a+simd" COMPILER_SUPPORTS_ARMV8_SIMD)
    if(COMPILER_SUPPORTS_ARMV8_SIMD)
        target_compile_options(RayTracerInOneWeekend
                PRIVATE
                $<$<CXX_COMPILER_ID:GNU,Clang>:-march=armv8-a+simd>
        )
        message(STATUS "Enabling -march=armv8-a+simd for NEON support")
    else()
        message(WARNING "ARM host, but compiler does not support -march=armv8-a+simd; building without NEON")
    endif()
else()
    message(STATUS "Non-ARM host (${CMAKE_HOST_SYSTEM_PROCESSOR}); skipping NEON flags")
endif()

target_compile_definitions(RayTracerInOneWeekend
        PRIVATE
        __ARM_NEON__=1            # pretend the compiler defined this
)