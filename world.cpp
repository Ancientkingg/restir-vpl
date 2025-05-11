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

World load_world() {
	auto loading_start = std::chrono::high_resolution_clock::now();

	World world;
	//world.add_obj("objects/whiteMonkey.obj", false);
	//world.add_obj("objects/blueMonkey_rotated.obj", false);
	//world.add_obj("objects/bigCubeLight.obj", true);
	// world.add_obj("objects/bistro_normal.obj", false);
	// world.add_obj("objects/bistro_lights.obj", true);
	world.place_obj("objects/bigCubeLight.obj", true, glm::vec3(5, 5, 0));
	world.place_obj("objects/modern_living_room.obj", false, glm::vec3(0, 0, 0));
	auto loading_stop = std::chrono::high_resolution_clock::now();

	std::clog << "Loading took ";
	std::clog << std::chrono::duration_cast<std::chrono::milliseconds>(loading_stop - loading_start).count();
	std::clog << " milliseconds" << std::endl;

	return world;
}

void load_one_obj_at(std::vector<Triangle>& triangle_soup, std::vector<tinyobj::material_t>& materials, std::vector<int>& mat_ids, std::string& file_path, glm::vec3 position) {
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

	int num_starting_mats = materials.size();

	// Access the loaded shapes and materials
	const auto& shapes = reader.GetShapes();
	const auto& attrib = reader.GetAttrib();

	const auto& new_mats = reader.GetMaterials();

	for (tinyobj::material_t new_mat : new_mats) {
		materials.push_back(new_mat);
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
			mat_ids.push_back(material_id);
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

				auto normal_base = 3 * idx.normal_index;
				auto x_normals = attrib.normals[normal_base + x];
				auto y_normals = attrib.normals[normal_base + y];
				auto z_normals = attrib.normals[normal_base + z];

				auto texcoord_base = 2 * idx.texcoord_index;
				auto x_texcoords = attrib.texcoords[texcoord_base + x];
				auto y_texcoords = attrib.texcoords[texcoord_base + y];

				triangle_verts[v] = Vertex(
					glm::vec3(x_pos, y_pos, z_pos),
					glm::vec3(x_normals, y_normals, z_normals),
					glm::vec2(x_texcoords, y_texcoords)
				);
			}

			Triangle triangle = Triangle(triangle_verts, material_id);
			triangle_soup.push_back(triangle + position);

			index_offset += fv;
			face_id++;
		}
	}

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
	load_one_obj_at(triangle_soup, all_materials, all_material_ids, file_path, glm::vec3(0.0f));
	if(is_lights) load_one_obj_at(lights, light_materials, light_material_ids , file_path, glm::vec3(0.0f));
}

void World::place_obj(std::string file_path, bool is_lights, glm::vec3 position) {
	load_one_obj_at(triangle_soup, all_materials, all_material_ids, file_path, position);
	if (is_lights) load_one_obj_at(lights, light_materials, light_material_ids, file_path, position);
}

tinybvh::BVH& World::bvh(){
	// Check if this->bvhInstance is already built
	if (bvh_built) {
		return this->bvhInstance;
	}
	bvh_built = true;

	raw_bvh_data = toBVHVec(triangle_soup);

//#if defined(__AVX__) || defined(__AVX2__)
	//bvhInstance.BuildAVX(raw_triangles.data(), triangle_soup.size());
//#else
	bvhInstance.Build(raw_bvh_data.data(), triangle_soup.size());
//#endif

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
	hit.triangle[0] = toGLMVec(bvhInstance.verts[r.hit.prim * 3 + 0]);
	hit.triangle[1] = toGLMVec(bvhInstance.verts[r.hit.prim * 3 + 1]);
	hit.triangle[2] = toGLMVec(bvhInstance.verts[r.hit.prim * 3 + 2]);
	hit.normal = glm::normalize(
		glm::cross(hit.triangle[1] - hit.triangle[0],
			hit.triangle[2] - hit.triangle[0]));

	if (glm::dot(hit.normal, ray.direction()) > 0.0f) {
		hit.normal = -hit.normal;
	}

	int m_id = bvhInstance.verts[r.hit.prim * 3][3];
	if (m_id < 0 || m_id >= all_materials.size()) {
		std::cerr << "Error: Material ID out of range." << std::endl;
		return false;
	}

	hit.mat_ptr = get_materials()[m_id];


	// Calculate UV coordinates using r.hit.u and r.hit.v
	const Triangle& triangle = triangle_soup[r.hit.prim];
	float w = 1.0f - r.hit.u - r.hit.v;
	hit.uv = triangle.v0.texcoord * w +
		triangle.v1.texcoord * r.hit.u +
		triangle.v2.texcoord * r.hit.v;

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
			light_materials[light_material_ids[face_id]].ambient[0],
			light_materials[light_material_ids[face_id]].ambient[1],
			light_materials[light_material_ids[face_id]].ambient[2]
		);
		float intensity = (c[0] + c[1] + c[2]) / 3;
		TriangularLight tl(lights[i], c, intensity);
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
