#include "material.h"
#include "rtweekend.h"
#include "triangular_light.h"

void load_obj_file(std::vector<tinybvh::bvhvec4> &triangle_soup, std::string file_name) {
    // Load the OBJ file using tinyobjloader
    tinyobj::ObjReaderConfig reader_config;
    reader_config.triangulate = true; // Ensure triangles are created
    reader_config.vertex_color = false; // Disable vertex colors
    reader_config.mtl_search_path = ""; // Set the material search path if needed
    tinyobj::ObjReader reader;
    if (!reader.ParseFromFile(file_name, reader_config)) {
        if (!reader.Error().empty()) {
            std::cerr << "TinyObjReader: " << reader.Error() << std::endl;
        }
        exit(1);
    }
    if (!reader.Warning().empty()) {
        std::cerr << "TinyObjReader: " << reader.Warning() << std::endl;
    }
    // Access the loaded shapes and materials
    const auto &shapes = reader.GetShapes();
    const auto &attrib = reader.GetAttrib();
    // const auto &materials = reader.GetMaterials();
    // Iterate through the shapes and extract the triangles
    for (size_t s = 0; s < shapes.size(); s++) {
        const auto &shape = shapes[s];
        size_t index_offset = 0;
        for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
            int fv = shape.mesh.num_face_vertices[f];
            if (fv != 3) {
                std::cerr << "Warning: Non-triangle face found in OBJ file." << std::endl;
                continue;
            }
            tinybvh::bvhvec4 triangle[3];
            for (size_t v = 0; v < fv; v++) {
                tinyobj::index_t idx = shape.mesh.indices[index_offset + v];
                triangle[v] = tinybvh::bvhvec4(
                    attrib.vertices[3 * idx.vertex_index + 0],
                    attrib.vertices[3 * idx.vertex_index + 1],
                    attrib.vertices[3 * idx.vertex_index + 2],
                    1.0f
                );
            }
            triangle_soup.push_back(triangle[0]);
            triangle_soup.push_back(triangle[1]);
            triangle_soup.push_back(triangle[2]);
            index_offset += fv;
        }
    }

    // Print the number of triangles loaded
    std::clog << "Loaded " << triangle_soup.size() / 3 << " triangles from " << file_name << std::endl;
}

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

    std::vector<tinybvh::bvhvec4> triangle_soup;

    tinybvh::BVH bvh;

    load_obj_file(triangle_soup, "suzanne.obj");

    tinybvh::bvhvec4 transpose =
            tinybvh::bvhvec4(0, 40, -1, 0);

    // Move the mesh 100 units back in the z direction
    for (int i = 0; i < triangle_soup.size(); i++) {
        triangle_soup[i] = transpose + triangle_soup[i];
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
        5.0f
    );

    tinybvh::bvhvec4 *triangles = triangle_soup.data();
    size_t num_triangles = triangle_soup.size() / 3;

    bvh.Build(triangles, num_triangles);

    cam.render(bvh, {simple_light, simple_light_2, simple_light_3}, {m}, "image.ppm");
}
