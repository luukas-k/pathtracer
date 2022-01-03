#pragma once

#include "math.h"
#include "material.h"

struct Sphere {
	Vec3 pos;
	number_t radius;
	Material material;
};


number_t distance(const Sphere& sphere, Vec3 p) {
	return ::distance(p, sphere.pos) - sphere.radius;
}