#pragma once

#include "scene.h"
#include "camera.h"
#include "image.h"

#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>
#include <functional>

std::tuple<u32, u32, u32> hours_minutes_and_seconds(u32 seconds) {
	u32 hrs = seconds / 3600;
	seconds -= hrs * 3600;
	u32 mins = seconds / 60;
	seconds -= mins * 60;
	return { hrs, mins, seconds};
}

std::string time_string(u32 seconds) {
	auto [hrs, mins, sec] = hours_minutes_and_seconds(seconds);
	if(hrs > 0)
		return std::to_string(hrs) + "hr " + std::to_string(mins) + "min " + std::to_string(sec) + "s";
	if (mins > 0)
		return std::to_string(mins) + "min " + std::to_string(sec) + "s";
	return std::to_string(sec) + "s";
}

class ThreadPool {
public:
	using Task = std::function<void()>;

	ThreadPool(u32 n = 7) {
		terminate = false;
		for (u32 i = 0; i < n; i++) {
			threads.push_back(std::thread([this]() {
				bool should_terminate = false;
				do {
					if (terminate) {
						should_terminate = true;
						break;
					}

					if (task_mutex.try_lock()) {
						if (task_queue.size()) {
							Task front_task = task_queue.front();
							task_queue.erase(task_queue.begin());

							task_mutex.unlock();

							front_task();
						}
						else {
							task_mutex.unlock();
						}
					}

				} while (!should_terminate);
			}));
		}
	}
	~ThreadPool() {
		stop();
	}

	void submit_task(Task&& t) {
		std::lock_guard l(task_mutex);
		task_queue.push_back(std::move(t));
	}
	bool is_done() const {
		std::lock_guard l(task_mutex);
		bool is_done = true;
		for (auto& t : threads) {
			is_done &= t.joinable();
		}
		is_done &= (task_queue.size() == 0);
		return is_done;
	}
	void stop() {
		{
			std::lock_guard l(task_mutex);
			terminate = true;
			task_queue.clear();
		}
		{
			for (auto& t : threads) {
				t.join();
			}
		}
		threads.clear();
	}
	u32 task_queue_size() const {
		std::lock_guard l(task_mutex);
		return (u32)task_queue.size();
	}
	u32 thread_count() const {
		std::lock_guard l(task_mutex);
		return threads.size();
	}
private:
	mutable std::mutex task_mutex;
	std::vector<Task> task_queue;
	std::atomic<bool> terminate;
	std::vector<std::thread> threads;
};

class Renderer {
public:
	Renderer() = default;
	~Renderer() = default;

	void render(const Scene& scene, const Camera& cam, Image& rt) {
		u32 w = rt.width(), h = rt.height();
		number_t ar = (number_t)w / (number_t)h;
		u32 last_completion = 0;

		auto t1 = std::chrono::high_resolution_clock::now();
		// Iterate over every pixel
		for (u32 y = 0; y < h; y++) {
			u32 cur_completion = (u32)(((number_t)y / (number_t)h) * 100);
			auto t2 = std::chrono::high_resolution_clock::now();
			if (cur_completion != last_completion) {
				u32 elapsed = (u32)std::chrono::duration_cast<std::chrono::seconds>(t2 - t1).count();
				u32 estimated_completion = (u32)((number_t)elapsed / ((number_t)y / (number_t)h)) - elapsed;
				std::cerr << "\r                                                                                                                  ";
				std::cerr << "\rCompletion: " << (u32)(((number_t)y / (number_t)h) * 100) << "% \tTime: " << time_string(elapsed) << "s\tEstimated completion: " << time_string(estimated_completion) << "s";
				last_completion = cur_completion;
			}
			for (u32 x = 0; x < w; x++) {
				Vec3 col{ 0.f, 0.f, 0.f };
				// For each sample generate a ray and add some randomness to it
				for (u32 sample = 0; sample < samples; sample++) {
					number_t u = x + random_f32();
					number_t v = y + random_f32();
					col = col + raycast_scene(scene, cam.get_ray(u, v, (number_t)w, (number_t)h, ar), bounces + 1);
				}
				// Blend the samples together
				rt.get(x, y) = sqrt(col * (1.f / samples));
			}
		}
	}
	void render_mt(const Scene& scene, const Camera& cam, Image& rt) {
		u32 w = rt.width(), h = rt.height();
		number_t ar = (number_t)w / (number_t)h;
		u32 last_completion = 0;

		struct TaskInfo {
			u32 x, y, w, h;
		};

		ThreadPool tp(num_threads);
		u32 total_tasks = 0;
		for (u32 y = 0; y < h; ) {
			u32 cons_h = min<u32>(64, h - y);
			for (u32 x = 0; x < w; ) {
				u32 cons_w = min<u32>(64, w - x);
				TaskInfo ti{
					.x = x,
					.y = y,
					.w = cons_w,
					.h = cons_h
				};
				tp.submit_task([this, ti, w, h, sc = samples, cam = &cam, scene = &scene, rt = &rt, ar]() {
					for (u32 i = 0; i < ti.w; i++) {
						for (u32 k = 0; k < ti.h; k++) {
							Vec3 col{ 0.f, 0.f, 0.f };
							for (u32 s = 0; s < sc; s++) {
								number_t u = ti.x + i + random_f32();
								number_t v = ti.y + k + random_f32();
								col = col + raycast_scene(*scene, cam->get_ray(u, v, (number_t)w, (number_t)h, ar), bounces + 1);
							}
							rt->get(ti.x + i, ti.y + k) = sqrt(col * (1.f / sc));
						}
					}
				});
				total_tasks += 1;
				x += cons_w;
			}
			y += cons_h;
		}

		auto t0 = std::chrono::high_resolution_clock::now();
		while (!tp.is_done()) {
			using namespace std::chrono_literals;
			std::this_thread::sleep_for(2s);
			u32 num_tasks_left = tp.task_queue_size();
			auto t1 = std::chrono::high_resolution_clock::now();
			auto time_since_start = (u32)std::chrono::duration_cast<std::chrono::seconds>(t1 - t0).count();
			float completion_percentage = ((float)(total_tasks - num_tasks_left) / (float)total_tasks) * 100.f;
			float time_estimate = (((float)time_since_start / (float)completion_percentage) * 100.f);
			std::cout << "Completion: " << completion_percentage << " Runtime: " << time_string(time_since_start) << " Total runtime estimate: " << time_string(time_estimate - time_since_start) << "\n";
		}
	}

	void set_samples(u32 n) { samples = n; }
	u32 get_samples() const { return samples; }
	void set_bounces(u32 n) { bounces = n; }
	u32 get_bounces() const { return bounces; }
	void set_max_path_steps(u32 n) { path_step_max = n; }
	u32 get_max_path_steps() const { return path_step_max; }
	void set_epsilon(number_t e) { EPSILON = e; }
	number_t get_epsilon() const { return EPSILON; }
	void set_thread_count(u32 n) { num_threads = n; }
	u32 get_thread_count() const { return num_threads; }
private:
	/*
	Vec3 raycast_scene_recurse(const Scene& scene, Ray ray, i32 depth) {
		// If maximum depth is reached no light happens
		if (depth <= 0)
			return Vec3{ 0.f, 0.f, 0.f };

		// Ray march into the scene to find the ray scene intersection
		auto pos_or = scene.ray(ray, path_step_max, EPSILON);

		// If there is a collision do shading computations
		if (auto pos = pos_or.value_or(Vec3{}); pos_or.has_value()) {
			auto [distance_to_scene, normal, mat] = scene.distance_and_normal_and_material(pos);

			// Advance the reflection ray a bit to reduce self intersection of the ray
			// Different scattering if the material is a metal, lambertian or dielectric
			number_t REFLECTION_ADVANCE = EPSILON * 1.2f;
			if (mat->type == MaterialType::Metallic) {
				Vec3 scatter_dir = reflect(ray.direction(), normal) + random_in_unit_sphere() * (1.f - mat->m.shininess);
				return mat->m.emissive + raycast_scene(scene, Ray(pos, scatter_dir).advance(REFLECTION_ADVANCE), depth - 1);
			}
			else if (mat->type == MaterialType::Lambertian) {
				Vec3 target = pos + normal + random_in_unit_sphere();
				Vec3 scatter_dir = target - pos;
				return mat->l.emissive + mat->l.albedo * raycast_scene(scene, Ray(pos, scatter_dir).advance(REFLECTION_ADVANCE), depth - 1) * mat->l.reflectance;
			}
			else if (mat->type == MaterialType::Dielectric) {
				bool is_front = dot(ray.direction(), -normal.normalize()) < 0.f;
				number_t refraction_ratio = is_front ? mat->d.refractive_index : 1.f / mat->d.refractive_index;
				// Check if internal or external refraction

				double cos_theta = fmin(dot(-ray.direction(), normal), 1.0);
				double sin_theta = sqrt(1.0 - cos_theta * cos_theta);

				bool cannot_refract = refraction_ratio * sin_theta > 1.0;

				Vec3 scatter_dir = reflect(ray.direction().normalize(), normal.normalize());
				if (!cannot_refract)
					scatter_dir = refract(ray.direction().normalize(), normal.normalize() * (is_front ? -1.f : 1.f), refraction_ratio);
				// else
				// 	scatter_dir = scatter_dir + random_in_unit_sphere() * 0.1f;

				return mat->d.emissive + raycast_scene(scene, Ray(pos, scatter_dir).advance(REFLECTION_ADVANCE), depth - 1);
			}
		}
		// Nothing was hit so no light is accumulated
		return Vec3{ 0.f, 0.f, 0.f };
	}
	*/
	Vec3 raycast_scene(const Scene& scene, Ray ray, i32 max_depth) {
		Vec3 color{0.f, 0.f, 0.f};
		Vec3 factor{1.f, 1.f, 1.f};

		for (i32 depth = 0; depth < max_depth; depth++) {
			// Ray march into the scene to find the ray scene intersection
			auto pos_or = scene.ray(ray, path_step_max, EPSILON);

			// If there is a collision do shading computations
			if (auto pos = pos_or.value_or(Vec3{}); pos_or.has_value()) {
				auto [distance_to_scene, normal, mat] = scene.distance_and_normal_and_material(pos);

				// Advance the reflection ray a bit to reduce self intersection of the ray
				// Different scattering if the material is a metal, lambertian or dielectric
				number_t REFLECTION_ADVANCE = EPSILON * 1.2f;
				if (mat->type == MaterialType::Metallic) {
					Vec3 scatter_dir = reflect(ray.direction(), normal) + random_in_unit_sphere() * (1.f - mat->m.shininess);
					color = color + factor * mat->m.emissive;

					factor = {1.f, 1.f, 1.f};
					ray = Ray(pos, scatter_dir).advance(REFLECTION_ADVANCE);
				}
				else if (mat->type == MaterialType::Lambertian) {
					Vec3 target = pos + normal + random_in_unit_sphere();
					Vec3 scatter_dir = target - pos;

					color = color + factor * mat->l.emissive;

					factor = mat->l.albedo * mat->l.reflectance;
					ray = Ray(pos, scatter_dir).advance(REFLECTION_ADVANCE);
				}
				else if (mat->type == MaterialType::Dielectric) {
					bool is_front = dot(ray.direction(), -normal.normalize()) < 0.f;
					number_t refraction_ratio = is_front ? mat->d.refractive_index : 1.f / mat->d.refractive_index;
					// Check if internal or external refraction

					double cos_theta = fmin(dot(-ray.direction(), normal), 1.0);
					double sin_theta = sqrt(1.0 - cos_theta * cos_theta);

					bool cannot_refract = refraction_ratio * sin_theta > 1.0;

					Vec3 scatter_dir = reflect(ray.direction().normalize(), normal.normalize());
					if (!cannot_refract)
						scatter_dir = refract(ray.direction().normalize(), normal.normalize() * (is_front ? -1.f : 1.f), refraction_ratio);

					scatter_dir = scatter_dir + random_in_unit_sphere() * 0.1f;

					color = color + factor * mat->d.emissive;

					factor = {1.f, 1.f, 1.f},
					ray = Ray(pos, scatter_dir).advance(REFLECTION_ADVANCE);
				}
				else {
					// Unkown mat
				}
			}
			else {
				break;
			}
		}
		return color;
	}

	u32 samples = 4;
	u32 bounces = 4;
	u32 path_step_max = 100;
	number_t EPSILON = 0.0001f;
	u32 num_threads = 1;
};