#ifndef WORLD_HPP
#define WORLD_HPP

#ifndef TINY_BVH_H_
#include "lib/tiny_bvh.h"
#endif
#ifndef TINY_OBJ_LOADER_H_
#include "lib/tiny_obj_loader.h"
#endif
#include <sys/stat.h>
#include <string>
#include <iostream>
#include <vector>
#include <algorithm>

#include "triangular_light.h"
#include "material.h"

class World
{
	public:
	std::vector<tinybvh::bvhvec4> triangle_soup;
	std::vector<tinyobj::material_t> all_materials;
	std::vector<tinybvh::bvhvec4> lights;
	std::vector<tinyobj::material_t> light_materials;
	
	
	World(); // constructor makes an empty world

	void add_obj(std::string file, bool is_lights); // Add an obj, indicate if it is all lights

	bool intersect(Ray& ray, hit_info& hit);
	bool is_occluded(const Ray &ray, float dist);

	tinybvh::BVH& bvh(); // Build the bvh

	std::vector<triangular_light> get_triangular_lights();
	std::vector<Material*> get_materials();


	private:
	std::vector<int> all_material_ids;
	std::vector<int> light_material_ids;
	std::vector<Material*> mats_small;
	tinybvh::BVH bvhInstance;
	bool bvh_built = false;
};

#endif