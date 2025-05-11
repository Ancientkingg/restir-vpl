#include "world.hpp"

#include <filesystem>
#include <chrono>
#include <glm/glm.hpp>
#ifndef TINY_BVH_H_
#include "lib/tiny_bvh.h"
#endif

#include "camera.hpp"
#include "texture.hpp"
#include "tiny_bvh_types.hpp"
#include "geometry.hpp"
#include "constants.hpp"



World load_world() {
	auto loading_start = std::chrono::high_resolution_clock::now();

	World world;
	//world.add_obj("objects/whiteMonkey.obj", false);
	//world.add_obj("objects/blueMonkey_rotated.obj", false);
	//world.add_obj("objects/bigCubeLight.obj", true);
	//world.add_obj("objects/bistro_normal.obj", false);
	//world.add_obj("objects/bistro_lights.obj", true);
	 //world.place_obj("objects/bigCubeLight.obj", true, glm::vec3(5, 5, 0));
	 //world.place_obj("objects/modern_living_room.obj", false, glm::vec3(0, 0, 0));
	//world.add_obj("objects/monkeyLightInOne.obj", false);
	 //world.place_obj("objects/ceiling_light.obj", true, glm::vec3(0, 10, 0));
	// world.place_obj("objects/Gauntlet.obj", false, glm::vec3(0, 0, 0));
	auto loading_stop = std::chrono::high_resolution_clock::now();

	std::clog << "Loading took ";
	std::clog << std::chrono::duration_cast<std::chrono::milliseconds>(loading_stop - loading_start).count();
	std::clog << " milliseconds" << std::endl;

	return world;
}


void World::load_obj_at(std::string& file_path, glm::vec3 position, bool force_light) {
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

	int num_starting_mats = all_materials.size();

	// Access the loaded shapes and materials
	const auto& shapes = reader.GetShapes();
	const auto& attrib = reader.GetAttrib();

	const auto& new_mats = reader.GetMaterials();

	for (tinyobj::material_t new_mat : new_mats) {
		all_materials.push_back(new_mat);
	}

	bool normals_excluded = attrib.normals.empty();
	if (normals_excluded) {
		std::cerr << "Normals are not included in " << file_path << std::endl;
	}

	bool texcoords_excluded = attrib.texcoords.empty();
	if (texcoords_excluded) {
		std::cerr << "Texcoords are not included in " << file_path << std::endl;
	}

	// Iterate through the shapes and extract the triangles
	for (tinyobj::shape_t shape : shapes) {
		int face_id = 0;
		size_t index_offset = 0;
		for (int fv : shape.mesh.num_face_vertices) {
			if (fv != 3) {
				std::cerr << "Warning: Non-triangle face found in OBJ file." << std::endl;
				continue;
			}
			int material_id = -1;
			if (face_id < shape.mesh.material_ids.size()) {
				material_id = shape.mesh.material_ids[face_id] + num_starting_mats;
			}
			all_material_ids.push_back(material_id);
			Vertex triangle_verts[3];
			for (size_t v = 0; v < fv; v++) {
				tinyobj::index_t idx = shape.mesh.indices[index_offset + v];

				const int x = 0;
				const int y = 1;
				const int z = 2;

				auto pos_base = 3 * idx.vertex_index;
				auto x_pos = attrib.vertices[pos_base + x];
				auto y_pos = attrib.vertices[pos_base + y];
				auto z_pos = attrib.vertices[pos_base + z];

				glm::vec3 normals = glm::vec3(0.0f);
				if (!normals_excluded) {
					auto normal_base = 3 * idx.normal_index;
					auto x_normals = attrib.normals[normal_base + x];
					auto y_normals = attrib.normals[normal_base + y];
					auto z_normals = attrib.normals[normal_base + z];
					normals = glm::vec3(x_normals, y_normals, z_normals);
				}

				glm::vec2 texcoords = glm::vec2(0.0f);
				if (!texcoords_excluded) {
					auto texcoord_base = 2 * idx.texcoord_index;
					auto x_texcoords = attrib.texcoords[texcoord_base + x];
					auto y_texcoords = attrib.texcoords[texcoord_base + y];
					texcoords = glm::vec2(x_texcoords, y_texcoords);
				}

				triangle_verts[v] = Vertex(
					glm::vec3(x_pos, y_pos, z_pos),
					normals,
					texcoords
				);
			}

			Triangle triangle = Triangle(triangle_verts, material_id);

			// Check if the material is emissive and add to light triangles
			if (material_id >= 0 && material_id < all_materials.size()) {
				const tinyobj::material_t& mat = all_materials[material_id];

				bool check_emission = mat.emission[0] > 0 || mat.emission[1] > 0 || mat.emission[2] > 0;
				bool check_emissive_texmap = mat.emissive_texname != "";

				if (check_emission || check_emissive_texmap || force_light) {
					lights.push_back(triangle + position);
					light_material_ids.push_back(material_id);
					light_materials.push_back(mat);
				}
			}

			triangle_soup.push_back(triangle + position);
			all_material_ids.push_back(material_id);
			

			index_offset += fv;
			face_id++;
		}
	}
	std::clog << "Loaded " << triangle_soup.size() << " triangles from " << file_path << std::endl;
	std::clog << "Loaded " << all_materials.size() << " materials from " << file_path << std::endl;
	std::clog << "Loaded " << lights.size() << " lights from " << file_path << std::endl;
}

World::World() {
	triangle_soup = {};
	all_materials = {};
	lights = {};
	light_materials = {};

	all_material_ids = {};
	light_material_ids = {};
	mats_small = {};
}

void World::add_obj(std::string file_path, bool is_lights){
	load_obj_at(file_path, glm::vec3(0.0f), is_lights);
}

void World::place_obj(std::string file_path, bool is_lights, glm::vec3 position) {
	load_obj_at(file_path, position, is_lights);
}

tinybvh::BVH& World::bvh(){
	// Check if this->bvhInstance is already built
	if (bvh_built) {
		return this->bvhInstance;
	}
	bvh_built = true;

	raw_bvh_data = toBVHVec(triangle_soup);

#if defined(__AVX__) || defined(__AVX2__)
	bvhInstance.BuildAVX(raw_bvh_data.data(), triangle_soup.size());
#else
	bvhInstance.Build(raw_bvh_data.data(), triangle_soup.size());
#endif

	return bvhInstance;
}

bool World::intersect(Ray& ray, HitInfo& hit) {
	tinybvh::Ray r = toBVHRay(ray);
	if (!bvh_built) {
		std::cerr << "Error: BVH not built. Call bvh() before intersect()." << std::endl;
		return false;
	}
	bvhInstance.Intersect(r);
	if (r.hit.t == 1E30f) {
		return false; // No intersection
	}

	hit.t = r.hit.t;
	hit.r = ray;
	const Triangle& triangle = triangle_soup[r.hit.prim];

	hit.triangle = triangle;

	int m_id = bvhInstance.verts[r.hit.prim * 3][3];
	if (m_id < 0 || m_id >= all_materials.size()) {
		std::cerr << "Error: Material ID out of range." << std::endl;
		return false;
	}

	hit.mat_ptr = get_materials()[m_id];

	hit.uv = glm::vec2(r.hit.u, r.hit.v);

	return true;
}

bool World::is_occluded(const Ray &ray, float dist) {
	tinybvh::Ray r = toBVHRay(ray, dist);
	return this->bvhInstance.IsOccluded(r);
}

std::vector<TriangularLight> World::get_triangular_lights(){
	std::vector<TriangularLight> out;
	float  max_pdf = -1;

	int face_id = 0;
	for(int i = 0; i < lights.size(); i++){
		glm::vec3 c = glm::vec3(
			all_materials[light_material_ids[face_id]].ambient[0],
			all_materials[light_material_ids[face_id]].ambient[1],
			all_materials[light_material_ids[face_id]].ambient[2]
		);
		float intensity = (c[0] + c[1] + c[2]) / 3;
		TriangularLight tl(lights[i], c, intensity * BASE_LIGHT_INTENSITY);
		out.push_back(tl);
		face_id++;
	}

	return out;
}

std::vector<Material*> World::get_materials(bool ignore_textures){
	if (mats_small.size() > 0) return mats_small;

	std::vector<Material*> out;
	for(tinyobj::material_t mat : all_materials){
		bool is_light = std::any_of(light_materials.begin(), light_materials.end(), [&](const tinyobj::material_t& m) {
			return m.name == mat.name;
		});

		if (mat.diffuse_texname != "" && !ignore_textures) {
			Texture* texture = new ImageTexture(mat.diffuse_texname.c_str());
			if (is_light) {
				Emissive* end_mat = new Emissive(texture);
				out.push_back(end_mat);
			}
			else {
				Lambertian* end_mat = new Lambertian(texture);
				out.push_back(end_mat);
			}
		}
		else {
			if (is_light) {
				Emissive* end_mat = new Emissive({ mat.ambient[0], mat.ambient[1], mat.ambient[2] });
				out.push_back(end_mat);
			}
			else {
				Lambertian* end_mat = new Lambertian({ mat.diffuse[0], mat.diffuse[1], mat.diffuse[2] });
				out.push_back(end_mat);
			}
		}
	}

	mats_small = out;
	return out;
}
