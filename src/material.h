#pragma once

#include "math.h"

enum struct MaterialType {
	Lambertian,
	Metallic,
	Dielectric
};


struct Lambertian {
	number_t reflectance;
	Vec3 albedo;
	Vec3 emissive;
};

struct Metallic {
	number_t shininess;
	Vec3 emissive;
};

struct Dielectric {
	number_t refractive_index;
	Vec3 emissive;
};


struct Material {
	MaterialType type;
	union {
		Lambertian l;
		Metallic m;
		Dielectric d;
	};
};

