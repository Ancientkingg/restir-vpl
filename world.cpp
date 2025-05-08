#include "world.hpp"

#include <glm/glm.hpp>
#include "camera2.hpp"

void load_one_obj(std::vector<tinybvh::bvhvec4> &triangle_soup, std::vector<tinyobj::material_t> &materials, std::vector<int> &mat_ids, std::string &file_path) {
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
	const auto &shapes = reader.GetShapes();
	const auto &attrib = reader.GetAttrib();

	const auto &new_mats = reader.GetMaterials();

	for(tinyobj::material_t new_mat : new_mats){
		materials.push_back(new_mat);
	}
	// Iterate through the shapes and extract the triangles
	for (tinyobj::shape_t shape : shapes){
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
			tinybvh::bvhvec4 triangle[3];
			for (size_t v = 0; v < fv; v++) {
				tinyobj::index_t idx = shape.mesh.indices[index_offset + v];
				triangle[v] = tinybvh::bvhvec4(
					attrib.vertices[3 * idx.vertex_index + 0],
					attrib.vertices[3 * idx.vertex_index + 1],
					attrib.vertices[3 * idx.vertex_index + 2],
					material_id
				);
			}

			triangle_soup.push_back(triangle[0]);
			triangle_soup.push_back(triangle[1]);
			triangle_soup.push_back(triangle[2]);
			index_offset += fv;
			face_id++;
		}
	}

}

void load_one_obj_at(std::vector<tinybvh::bvhvec4>& triangle_soup, std::vector<tinyobj::material_t>& materials, std::vector<int>& mat_ids, std::string& file_path, glm::vec3 position) {
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
			tinybvh::bvhvec4 triangle[3];
			for (size_t v = 0; v < fv; v++) {
				tinyobj::index_t idx = shape.mesh.indices[index_offset + v];
				triangle[v] = tinybvh::bvhvec4(
					attrib.vertices[3 * idx.vertex_index + 0],
					attrib.vertices[3 * idx.vertex_index + 1],
					attrib.vertices[3 * idx.vertex_index + 2],
					material_id
				);
			}

			tinybvh::bvhvec4 pos = tinybvh::bvhvec4(position[0], position[1], position[2], 0.0f);

			triangle_soup.push_back(triangle[0] + pos);
			triangle_soup.push_back(triangle[1] + pos);
			triangle_soup.push_back(triangle[2] + pos);
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
	load_one_obj(triangle_soup, all_materials, all_material_ids, file_path);
	if(is_lights) load_one_obj(lights, light_materials, light_material_ids , file_path);
}

void World::place_obj(std::string file_path, bool is_lights, glm::vec3 position) {
	load_one_obj_at(triangle_soup, all_materials, all_material_ids, file_path, position);
	if (is_lights) load_one_obj_at(lights, light_materials, light_material_ids, file_path, position);
}

tinybvh::BVH& World::bvh(){
	// Check if this->bvhInstance is already built
	if (bvh_built) {
		std::clog << "BVH built, not building" << std::endl;
		return this->bvhInstance;
	}
	std::clog << "BVH not built, building" << std::endl;
	bvh_built = true;
	bvhInstance.Build(triangle_soup.data(), triangle_soup.size()/3);

	return bvhInstance;
}

bool World::intersect(Ray& ray, hit_info& hit) {
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

	hit.mat_ptr = mats_small[m_id];

	return true;
}

bool World::is_occluded(const Ray &ray, float dist) {
	tinybvh::Ray r = toBVHRay(ray, dist);
	return this->bvhInstance.IsOccluded(toBVHRay(ray));
}


std::vector<triangular_light> World::get_triangular_lights(){
	std::vector<triangular_light> out;
	float  max_pdf = -1;

	int face_id = 0;
	for(int i = 0; i < lights.size(); i += 3){
		glm::vec3 c = glm::vec3(
			light_materials[light_material_ids[face_id]].ambient[0],
			light_materials[light_material_ids[face_id]].ambient[1],
			light_materials[light_material_ids[face_id]].ambient[2]
		);
		triangular_light tl{
			glm::vec3(lights[i+0][0],lights[i+0][1],lights[i+0][2]),
			glm::vec3(lights[i+1][0],lights[i+1][1],lights[i+1][2]),
			glm::vec3(lights[i+2][0],lights[i+2][1],lights[i+2][2]),
			c,
			(c[0] + c[1] + c[2]) / 3
		};
		tl.pdf = 1.0 / tl.area();
		out.push_back(tl);

		max_pdf = tl.pdf > max_pdf ? tl.pdf : max_pdf;
		face_id++;
	}

	for(triangular_light &tl : out) tl.pdf = tl.pdf / max_pdf;	
	return out;
}

std::vector<Material*> World::get_materials(){
	std::vector<Material*> out;
	for(tinyobj::material_t mat : all_materials){
		bool is_light = std::any_of(light_materials.begin(), light_materials.end(), [&](const tinyobj::material_t& m) {
			return m.name == mat.name;
		});
		if(is_light){
			Emissive * end_mat = new Emissive({mat.ambient[0], mat.ambient[1], mat.ambient[2]});
			out.push_back(end_mat);
		}
		else{
			Lambertian * end_mat = new Lambertian({mat.diffuse[0], mat.diffuse[1], mat.diffuse[2]});
			out.push_back(end_mat);
		}
	}
	mats_small = out;
	return out;
}
