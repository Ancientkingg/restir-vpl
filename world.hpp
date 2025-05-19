#pragma once

#ifndef TINY_BVH_H_
#include "lib/tiny_bvh.h"
#endif
#ifndef TINY_OBJ_LOADER_H_
#include "lib/tiny_obj_loader.h"
#endif

#include <string>
#include <vector>
#include <memory>

#include "light.hpp"
#include "material.hpp"

class World
{
	public:
	std::vector<Triangle> triangle_soup;
	std::vector<tinyobj::material_t> all_materials;
	std::vector<Triangle> lights;
	std::vector<tinyobj::material_t> light_materials;
	std::vector<std::shared_ptr<PointLight>> point_lights;
	
	
	World(); // constructor makes an empty world

	void add_obj(std::string file, bool is_lights); // Add an obj, indicate if it is all lights
	void place_obj(std::string file, bool is_lights, glm::vec3 position);

	void spawn_point_light(glm::vec3 position, glm::vec3 normal, glm::vec3 color, float intensity);
	inline void remove_last_point_light() {
		point_lights.pop_back();
	}

	bool intersect(Ray& ray, HitInfo& hit);
	bool is_occluded(const Ray &ray, float dist);

	tinybvh::BVH& bvh(); // Build the bvh

	std::vector<std::weak_ptr<Light>> get_lights();
	std::vector<std::weak_ptr<Material>> get_materials(bool ignore_textures = true);


	private:
	std::vector<std::shared_ptr<Light>> scene_lights;
	std::vector<int> all_material_ids;
	std::vector<int> light_material_ids;
	std::vector<std::shared_ptr<Material>> mats_small;
	tinybvh::BVH bvhInstance;
	bool bvh_built = false;
	std::vector<tinybvh::bvhvec4> raw_bvh_data;

	void load_obj_at(std::string& file_path, glm::vec3 position, bool force_light = false);
};

World load_world();