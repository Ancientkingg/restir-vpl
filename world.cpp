#include "world.hpp"

#include <filesystem>
#include <chrono>
#include <glm/glm.hpp>
#ifndef TINY_BVH_H_
#include "lib/tiny_bvh.h"
#endif
#include <memory>
#include <random>

#include "camera.hpp"
#include "texture.hpp"
#include "tiny_bvh_types.hpp"
#include "geometry.hpp"
#include "constants.hpp"
#include "photon.hpp"
#include "spheres.hpp"

bool DISABLE_GI = true;


World load_world() {
	auto loading_start = std::chrono::high_resolution_clock::now();

	World world;
	// Scene 1
	//world.place_obj("objects/sahur.obj", false, glm::vec3(0, 0, 0));
	
	// Scene 2
	//world.place_obj("objects/cornell-box.obj", false, glm::vec3(0, 0, 0));

	// Scene 3
	world.place_obj("objects/bigCubeLight.obj", true, glm::vec3(5, 5, 0));
	world.place_obj("objects/modern_living_room.obj", false, glm::vec3(0, 0, 0));

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

void World::spawn_vpl(glm::vec3 position, glm::vec3 normal, glm::vec3 color, float intensity) {
	vpls.push_back(std::make_shared<PointLight>(PointLight(color, intensity, position, normal)));
}

void World::spawn_point_light(glm::vec3 position, glm::vec3 normal, glm::vec3 color, float intensity) {
	point_lights.push_back(std::make_shared<PointLight>(PointLight(color, intensity, position, normal)));
}

tinybvh::BVH& World::bvh(){
	// Check if this->bvhInstance is already built
	if (bvh_built) {
		return this->bvhInstance;
	}
	bvh_built = true;

	raw_bvh_data = toBVHVec(triangle_soup);

#if defined(__AVX__) || defined(__AVX2__)
	bvhInstance.BuildHQ(raw_bvh_data.data(), triangle_soup.size());
#else
	bvhInstance.BuildHQ(raw_bvh_data.data(), triangle_soup.size());
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

std::vector <std::weak_ptr<PointLight>> World::get_lights() {
	if (point_lights.empty()) {
		point_lights = generate_point_lights();

		// Convert point lights to glm::vec3
		auto point_light_positions = std::make_unique<std::vector<glm::vec3>>();
		point_light_positions->reserve(point_lights.size());
		for (const auto& pl : point_lights) {
			glm::vec3 pos = pl->position;
			if (std::isnan(pos.x) || std::isnan(pos.y) || std::isnan(pos.z)) {
				std::cerr << "Error: Point light position is NaN!" << std::endl;
			}
			else {
				point_light_positions->push_back(pos);
			}
		}

		point_light_cloud.points = std::move(point_light_positions);
		point_light_cloud.build_index();

		// Add all the vpls
		for (const auto& vpl : vpls) {
			point_lights.push_back(vpl);
		}

		// Convert vpls to points glm::vec3
		auto vpl_positions = std::make_unique<std::vector<glm::vec3>>();
		vpl_positions->reserve(vpls.size());
		for (const auto& vpl : vpls) {
			glm::vec3 pos = vpl->position;
			if (std::isnan(pos.x) || std::isnan(pos.y) || std::isnan(pos.z)) {
				std::cerr << "Error: VPL position is NaN!" << std::endl;
			}
			else {
				vpl_positions->push_back(pos);
			}
		}
		vpl_cloud.points = std::move(vpl_positions);
		vpl_cloud.build_index();
	}

	const int num_lights = point_lights.size();
	std::vector<std::weak_ptr<PointLight>> weak_lights(num_lights);
	for (size_t i = 0; i < num_lights; i++) {
		std::shared_ptr<PointLight> light = point_lights[i];
		weak_lights[i] = std::weak_ptr<PointLight>(light);
	}
	return weak_lights;
}

std::vector<std::shared_ptr<TriangularLight>> World::get_triangular_lights() {
	std::vector<std::shared_ptr<TriangularLight>> scene_lights;
	int face_id = 0;

	for (int i = 0; i < lights.size(); i++) {
		glm::vec3 c = glm::vec3(
			all_materials[light_material_ids[face_id]].ambient[0],
			all_materials[light_material_ids[face_id]].ambient[1],
			all_materials[light_material_ids[face_id]].ambient[2]
		);
		float intensity = (c[0] + c[1] + c[2]) / 3;
		std::shared_ptr<TriangularLight> tl = std::make_shared<TriangularLight>(TriangularLight(lights[i], c, intensity * BASE_LIGHT_INTENSITY));
		scene_lights.push_back(tl);
		face_id++;
	}

	return scene_lights;
}

thread_local std::mt19937 rng2(std::random_device{}());
std::uniform_real_distribution<float> dist2(0.0f, 1.0f);

static int sample_light_index(const int num_lights) {
	// Thread-local pair to track last used upper bound and distribution
	thread_local int cached_num_lights = -1;


	if (cached_num_lights != num_lights) {
		cached_num_lights = num_lights;
	}

	const int out = static_cast<int>(dist2(rng2) * cached_num_lights);

	return out;
}

static std::shared_ptr<TriangularLight> pick_triangular_light(std::vector<std::shared_ptr<TriangularLight>>& lights) {
	// Pick a random light source uniformly (regardless of its area)
	// FUTURE: Implement importance sampling based on the area and intensity of the light source
	const int index = sample_light_index(lights.size());
	return lights[index];
}

std::vector<std::shared_ptr<PointLight>> World::generate_point_lights() {
	constexpr size_t num_photons = N_PHOTONS;
	std::vector<std::shared_ptr<PointLight>> out;
	out.reserve(num_photons);

	// 1) For every triangular light in the scene, randomly generate point lights on it, similarly to how random light samples were generated.
	auto scene_lights = get_triangular_lights();

	// Compute total weighted area (intensity * area)
	float total_weight = 0.0f;
	for (auto& light : scene_lights) {
		total_weight += light->intensity * light->area();
	}

	// Per light, sample a number of photons proportional to its area and intensity
	size_t j = 0;
	for (auto& light : scene_lights) {
		float weight = light->intensity * light->area() / total_weight;
		int num_dl = static_cast<int>(weight * num_photons);

		for (int i = 0; i < num_dl; ++i) {
			float pdf_pt;
			glm::vec3 pos = light->sample_on_light(pdf_pt);
			glm::vec3 norm = light->normal(pos);
			float area = light->area();

			// Direct light emitted flux per photon = (intensity * area) / total_photons
			float per_photon_flux = (light->intensity * area) / float(num_photons);

			auto pl = std::make_shared<PointLight>(light->c, per_photon_flux, pos, norm);
			out.push_back(pl);
		}
	}

	if constexpr (N_INDIRECT_PHOTONS == 0) {
		// If we are not doing any bounces, we can return the point lights immediately
		return out;
	}

	if (DISABLE_GI) {
		// If GI is disabled, we can return the point lights immediately
		return out;
	}

	// 2) Generate indirect VPLs for GI via photon tracing
	size_t generated = 0;
	while (generated < N_INDIRECT_PHOTONS) {
		// Generate photon from existing point light in the scene
		const int idx = dist2(rng2) * out.size();
		const auto& base = out[idx];
		//if (!base) {
		//	// Error
		//	std::cerr << "Error: Light is null at index " << idx << std::endl;
		//}

		float pdf_pt = 1.0f;
		//const glm::vec3 emit_pos = base->sample_on_light(pdf_pt);
		const glm::vec3 emit_pos = base->position; // Use the position of the point light directly (more optimized)

		// Offset the random point slightly in the direction of the normal to avoid self-occlusion
		const glm::vec3 normal = base->normal(emit_pos);
		const glm::vec3 offset_pt = emit_pos + 1e-4f * normal;

		// Sample a random direction from the hemisphere above the light source
		float pdf_dir;
		const glm::vec3 random_dir = base->sample_direction(emit_pos, pdf_dir);

		if (pdf_dir <= 0.0f) {
			// If the PDF is zero or negative, skip this photon
			continue;
		}

		const float cos_theta = glm::dot(normal, random_dir); // Cosine of the angle between the light normal and the random direction
		if (cos_theta <= 0.0f) {
			// If the cosine is zero or negative, skip this photon
			continue;
		}

		const float raw_flux = base->intensity * cos_theta / (pdf_pt * pdf_dir);
		const float per_photon_flux = raw_flux;

		// check if the intensity is valid
		if (!std::isfinite(per_photon_flux)) {
			std::cerr << "Error: per_photon_flux is NaN or Inf!" << std::endl;
			// print base->intensity, cos_theta, pdf_pt and pdf_dir
			continue; // Skip this photon if the intensity is invalid
		}

		// Create a photon with the random point and direction
		Photon photon(offset_pt, random_dir, base->c, per_photon_flux);

		// Shoot the photon into the scene
		photon.shoot(*this, MAX_BOUNCES, generated, N_INDIRECT_PHOTONS);
	}

	// 3) Return the generated point lights
	return out;
}


std::vector<std::weak_ptr<Material>> World::get_materials(bool ignore_textures){
	if (!weak_mats.empty()) return weak_mats;

	if (mats_small.empty()) {

		std::vector <std::shared_ptr<Material>> out;
		for (tinyobj::material_t mat : all_materials) {
			bool is_light = std::any_of(light_materials.begin(), light_materials.end(), [&](const tinyobj::material_t& m) {
				return m.name == mat.name;
				});

			if (mat.diffuse_texname != "" && !ignore_textures) {
				auto texture = std::make_shared<ImageTexture>(mat.diffuse_texname.c_str());
				if (is_light) {
					auto end_mat = std::make_shared<Emissive>(texture);
					out.push_back(end_mat);
				}
				else {
					auto end_mat = std::make_shared<Lambertian>(texture);
					out.push_back(end_mat);
				}
			}
			else {
				if (is_light) {
					auto end_mat = std::make_shared<Emissive>(glm::vec3(mat.emission[0], mat.emission[1], mat.emission[2]));
					out.push_back(end_mat);
				}
				else {
					auto end_mat = std::make_shared<Lambertian>(glm::vec3(mat.diffuse[0], mat.diffuse[1], mat.diffuse[2]));
					out.push_back(end_mat);
				}
			}
		}

		mats_small = out;
	}

	// convert mats_small to weak_ptr<Material>
	const int num_mats = mats_small.size();
	weak_mats = std::vector<std::weak_ptr<Material>>(num_mats);
	for (size_t i = 0; i < num_mats; i++) {
		std::shared_ptr<Material> mat = mats_small[i];
		weak_mats[i] = std::weak_ptr<Material>(mat);
	}
	return weak_mats;
}
