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

class World
{
	public:
	std::vector<tinybvh::bvhvec4> triangle_soup;
	std::vector<tinyobj::material_t> all_materials;
	std::vector<tinybvh::bvhvec4> lights;
	std::vector<tinyobj::material_t> light_materials;
	
	
	World(); // constructor makes an empty world
	
	void add_obj(std::string file, bool is_lights); // Add an obj, indicate if it is all lights

	tinybvh::BVH bvh(); // Build the bvh
};

#endif