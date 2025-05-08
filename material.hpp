#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include "ray.hpp"
#include "texture.hpp"
#include "hit_info.hpp"
#include "util.hpp"

class Material {
public:
	virtual bool scatter(const Ray& r_In, const HitInfo& hit, glm::vec3& attenuation, Ray& scattered) const = 0;
	virtual glm::vec3 evaluate(const HitInfo& hit, const glm::vec3& wi) const = 0;
	virtual bool emits_light() const;

	virtual glm::vec3 albedo(const HitInfo& hit) const;
};

class Lambertian : public Material {
	Texture* _albedo;
public:
	Lambertian(const glm::vec3& a);
	Lambertian(Texture* a);
	virtual bool scatter(const Ray& r_in, const HitInfo& hit, glm::vec3& attenuation, Ray& scattered) const;

	virtual glm::vec3 evaluate(const HitInfo& hit, const glm::vec3& wi) const;

	virtual glm::vec3 albedo(const HitInfo& hit) const;
};

class Emissive : public Material {
	Texture* emit;
public:
	Emissive(const glm::vec3& a);
	Emissive(Texture* a);

	virtual bool scatter(const Ray& r_in, const HitInfo& hit, glm::vec3& attenuation, Ray& scattered) const;
	virtual glm::vec3 evaluate(const HitInfo& hit, const glm::vec3& wi) const;
	virtual bool emits_light() const;
	virtual glm::vec3 albedo(const HitInfo& hit) const;
};