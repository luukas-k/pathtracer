#pragma once

class Camera {
public:
	Camera()
		:
		pos({ .x = 0.f, .y = 0.f, .z = -40.f }),
		focal_distance(30.f),
		pitch(0.f), yaw(0.f), roll(0.f) {
	}
	Ray get_ray(number_t xf, number_t yf, number_t wf, number_t hf, number_t ar) const {
		number_t nxf = xf / wf;
		number_t nyf = yf / hf;

		number_t xdir = (nxf - 0.5f) * ar;
		number_t ydir = -1.f * (nyf - 0.5f);

		Vec3 look_dir = Vec3{ xdir, ydir, focal_distance }.normalize();

		look_dir = rotate_x(look_dir, pitch);
		look_dir = rotate_y(look_dir, yaw);
		look_dir = rotate_z(look_dir, roll);

		return Ray(pos, look_dir);
	}
	void set_position(Vec3 p) {
		pos = p;
	}
	void set_focal_distance(number_t d) {
		focal_distance = d;
	}
	void set_rotation(number_t pitch, number_t yaw, number_t roll) {
		this->pitch = pitch;
		this->yaw = yaw;
		this->roll = roll;
	}
private:
	Vec3 pos;
	number_t focal_distance;
	number_t pitch, yaw, roll;
};