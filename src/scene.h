#pragma once

#include "math.h"
#include "primitives.h"

#include <optional>

class Scene {
public:
	Scene() = default;
	~Scene() = default;

	void add_sphere(Sphere s) {
		spheres.push_back(s);
	}
	struct Result {
		number_t distance;
		Vec3 normal;
		const Material* material;
	};
	number_t distance(Vec3 position) const {
		number_t min_dist = FLT_MAX;
		for (auto& s : spheres) {
			number_t sdist = (number_t)abs(::distance(s, position));
			if (sdist < min_dist) {
				min_dist = sdist;
			}
		}
		return min_dist;
	}
	Result distance_and_normal_and_material(Vec3 position) const {
		number_t min_dist = FLT_MAX;
		Vec3 norm = Vec3{ 0.f, 1.f, 0.f };
		const Material* mat{};
		for (auto& s : spheres) {
			number_t sdist = abs(::distance(s, position));
			if (sdist < min_dist) {
				min_dist = sdist;
				norm = (position - s.pos).normalize();
				mat = &s.material;
			}
		}
		return { min_dist, norm, mat };
	}
	std::optional<Vec3> ray(Ray r, u32 max_steps, number_t EPSILON) const {
		for (u32 i = 0; i < max_steps; i++) {
			auto dist = distance(r.origin());
			if (dist < EPSILON) {
				return r.origin();
			}
			r = r.advance(dist);
		}
		return {};
	}
private:
	std::vector<Sphere> spheres;
};