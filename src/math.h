#pragma once

#include <stdint.h>
#include <cmath>
#include <random>
#include <intrin.h>

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using i32 = int32_t;
using number_t = double;

struct Vec3 {
	number_t x, y, z;

	Vec3 normalize() const {
		number_t len = sqrt(x * x + y * y + z * z);
		return Vec3{ x / len, y / len, z / len };
	}
	Vec3 operator-() {
		return Vec3{-x,-y,-z};
	}
};

Vec3 operator+(const Vec3& lhs, const Vec3& rhs) {
	return Vec3{ lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z };
}

Vec3 operator-(const Vec3& lhs, const Vec3& rhs) {
	return Vec3{ lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z };
}

Vec3 operator*(const Vec3& lhs, number_t rhs) {
	return Vec3{ lhs.x * rhs, lhs.y * rhs, lhs.z * rhs };
}

number_t dot(Vec3 a, Vec3 b) {
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

number_t clamp(number_t val, number_t min, number_t max) {
	if (val < min)
		return min;
	if (val > max)
		return max;
	return val;
}

number_t distance(const Vec3& a, const Vec3& b) {
	number_t abx = (a.x - b.x);
	number_t aby = (a.y - b.y);
	number_t abz = (a.z - b.z);
	return sqrt(abx * abx + aby * aby + abz * abz);
}

number_t max(number_t a, number_t b, number_t c) {
	return std::max(a, std::max(b, c));
}

class Ray {
public:
	Ray(Vec3 origin, Vec3 dir)
		:
		orig(origin), dir(dir.normalize()) {
	}

	Vec3 origin() const {
		return orig;
	}
	Vec3 direction() const {
		return dir;
	}
	Ray advance(number_t distance) const {
		return Ray(orig + dir * distance, dir);
	}
	void set_direction(Vec3 dir) {
		this->dir = dir;
	}
private:
	Vec3 orig, dir;
};

Vec3 sqrt(Vec3 v) {
	return Vec3{ sqrt(v.x), sqrt(v.y), sqrt(v.z) };
}


Vec3 reflect(const Vec3& v, const Vec3& n) {
	return v - n * 2 * dot(v, n);
}

Vec3 refract(const Vec3& v, const Vec3& n, number_t eta) {
	number_t k = 1.0 - eta * eta * (1.0 - dot(n, v) * dot(n, v));
    if (k < 0.0)
		return Vec3{0.f, 0.f, 0.f};
    else
        return v * eta - n * (eta * dot(n, v) + sqrt(k));
}

Vec3 lerp(const Vec3& a, const Vec3& b, number_t t) {
	return a * t + b * (1.f - t);
}

Vec3 operator*(Vec3 a, Vec3 b) {
	return Vec3{ a.x * b.x, a.y * b.y, a.z * b.z };
}

number_t max(number_t a, number_t b) {
	return a > b ? a : b;
}

template<typename T>
T min(T a, T b) {
	return a < b ? a : b;
}

class RandomDevice {
public:
	RandomDevice() 
		:
		mt(std::random_device()())
	{}
	RandomDevice(u32 seed)
		:
		mt(seed)
	{}
	~RandomDevice(){}

	u32 random() {
		return id(mt);
	}
	number_t random_num(float min = 0.f, float max = 1.f) {
		return min + ((number_t)random() / (number_t)id.max()) * (max - min);
	}
private:
	std::mt19937 mt;
	std::uniform_int_distribution<u32> id;
};

number_t random_f32(number_t min = 0.f, number_t max = 1.f) {
	return min + ((number_t)rand() / (number_t)RAND_MAX) * (max - min);
}

Vec3 random_vec3(number_t min, number_t max) {
	return Vec3{ random_f32(min,max),random_f32(min,max),random_f32(min,max) };
}

number_t square_length(const Vec3& v) {
	return v.x * v.x + v.y * v.y + v.z * v.z;
}

Vec3 random_in_unit_sphere() {
	while (true) {
		Vec3 r = random_vec3(-1.f, 1.f);
		if (square_length(r) < 1.f) return r;
	}
}

Vec3 rotate_z(Vec3 p, number_t ang) {
	return Vec3{
		p.x * cos(ang) - p.y * sin(ang),
		p.x * sin(ang) + p.y * cos(ang),
		p.z
	};
}

Vec3 rotate_y(Vec3 p, number_t ang) {
	return Vec3{
		p.x * cos(ang) + p.z * sin(ang),
		p.y,
		-p.x * sin(ang) + p.z * cos(ang)
	};
}

Vec3 rotate_x(Vec3 p, number_t ang) {
	return Vec3{
		p.x,
		p.y * cos(ang) - p.z * sin(ang),
		p.y * sin(ang) + p.z * cos(ang)
	};
}