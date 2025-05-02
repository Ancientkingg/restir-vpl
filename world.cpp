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

World::World(){
	triangle_soup   = {};
	all_materials   = {};
	lights          = {};
	light_materials = {};

	all_material_ids   = {};
	light_material_ids = {};
}

void World::add_obj(std::string file_path, bool is_lights){
	load_one_obj(triangle_soup, all_materials, all_material_ids, file_path);
	if(is_lights) load_one_obj(lights, light_materials, light_material_ids , file_path);
}

tinybvh::BVH World::bvh(){
	tinybvh::BVH out;
	out.Build(triangle_soup.data(), triangle_soup.size()/3);
	return out;
}

bool World::intersect(Ray& ray, hit_info& hit) {
	tinybvh::Ray r = toBVHRay(ray);
	tinybvh::BVH bvhInstance = this->bvh();
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

	hit.mat = &get_materials()[m_id];

	return true;
}

std::vector<triangular_light> World::get_triangular_lights(){
	std::vector<triangular_light> out;

	int face_id = 0;
	for(int i = 0; i < lights.size(); i += 3){
		glm::vec3 c = glm::vec3(
			light_materials[light_material_ids[face_id]].emission[0],
			light_materials[light_material_ids[face_id]].emission[1],
			light_materials[light_material_ids[face_id]].emission[2]
		);
		triangular_light tl{
			glm::vec3(lights[i][0],lights[i][1],lights[i][2]),
			glm::vec3(lights[i+1][0],lights[i+1][1],lights[i+1][2]),
			glm::vec3(lights[i+2][0],lights[i+2][1],lights[i+2][2]),
			c,
			(c[0] + c[1] + c[2]) / 3
		};
		out.push_back(tl);
		face_id++;
	}
	return out;
}

std::vector<material> World::get_materials(){
	std::vector<material> out;
	for(int id = 0; id < *std::max_element(all_material_ids.begin(), all_material_ids.end()); id++){
		tinyobj::material_t mat = all_materials[id];
		glm::vec3 c = glm::vec3(
			mat.diffuse[0],
			mat.diffuse[1],
			mat.diffuse[2]
		);
		float k_d = (mat.diffuse[0] + mat.diffuse[1] + mat.diffuse[2]) / 3;
		float k_s = *mat.specular;
		float p = mat.shininess;

		out.push_back({c, k_d, k_s, p});

	}
	return out;
}