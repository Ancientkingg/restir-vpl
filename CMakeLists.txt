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
"camera.cpp"
"hit_info.hpp"
"image_writer.cpp"
"light_sampler.cpp"
"main.cpp"
"material.cpp"
"ray.cpp"
"render.cpp"
"shading.cpp"
"texture.cpp"
"tiny_bvh_types.cpp"
"triangular_light.cpp"
"util.cpp"
"viewer.cpp"
"world.cpp"
"lib/stb_image.h" 
"interval.cpp" 
"geometry.cpp")

file(COPY ${CMAKE_SOURCE_DIR}/objects DESTINATION ${CMAKE_BINARY_DIR})
add_custom_command(TARGET RayTracerInOneWeekend POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_SOURCE_DIR}/objects"
    "$<TARGET_FILE_DIR:RayTracerInOneWeekend>/objects"
)

# Add the include directory for GLM
target_include_directories(RayTracerInOneWeekend PUBLIC ${GLM})

target_link_libraries(RayTracerInOneWeekend PUBLIC OpenMP::OpenMP_CXX ${SDL2_LIBRARIES})
target_link_libraries(RayTracerInOneWeekend PRIVATE SDL2::SDL2 SDL2::SDL2main)

message(STATUS "Host processor: ${CMAKE_HOST_SYSTEM_PROCESSOR}")

include(CheckCXXCompilerFlag)

check_cxx_compiler_flag("/arch:AVX2" COMPILER_SUPPORTS_AVX2)
check_cxx_compiler_flag("-mavx2" COMPILER_SUPPORTS_MAVX2)

if (MSVC AND COMPILER_SUPPORTS_AVX2)
    message(STATUS "Enabling AVX2 for MSVC")
    target_compile_options(RayTracerInOneWeekend PRIVATE /arch:AVX2)
elseif ((CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU") AND COMPILER_SUPPORTS_MAVX2)
    message(STATUS "Enabling AVX2 for GCC/Clang")
    target_compile_options(RayTracerInOneWeekend PRIVATE -mavx2)
else()
    message(WARNING "AVX2 not supported or enabled. Check your compiler/target.")
endif()

# Determine if we're building for ARM
if(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "aarch64" OR CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "arm64")

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