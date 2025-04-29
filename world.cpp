#include "world.hpp"

void load_one_obj(std::vector<tinybvh::bvhvec4> &triangle_soup, std::vector<tinyobj::material_t> &materials, std::string &file_path) {
	// this function is copied from Rafayels original implementation with slight changes

	// Load the OBJ file using tinyobjloader
	tinyobj::ObjReaderConfig reader_config;
	reader_config.triangulate = true; // Ensure triangles are created
	reader_config.vertex_color = false; // Disable vertex colors
	reader_config.mtl_search_path = ""; // Set the material search path if needed
	tinyobj::ObjReader reader;
	if (!reader.ParseFromFile(file_path, reader_config)) {
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
	materials = reader.GetMaterials();
	// Iterate through the shapes and extract the triangles
	for (auto shape : shapes){
		size_t index_offset = 0;
		for (int fv : shape.mesh.num_face_vertices) {
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

}

World::World(){
	triangle_soup   = {};
	all_materials   = {};
	lights          = {};
	light_materials = {};
}

void World::add_obj(std::string file_path, bool is_lights){
	load_one_obj(triangle_soup, all_materials, file_path);
	if(is_lights) load_one_obj(lights, light_materials, file_path);
}

tinybvh::BVH World::bvh(){
	tinybvh::BVH out;
	out.Build(triangle_soup.data(), triangle_soup.size()/3);
	return out;
}
