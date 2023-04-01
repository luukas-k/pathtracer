#include "math.h"
#include "image.h"
#include "renderer.h"
#include "material.h"
#include "primitives.h"


void generate_scene_1(Scene& scene, u32 ball_count) {
	Material red_mat{
		.type = MaterialType::Lambertian,
		.l = {
			.reflectance = .3f,
			.albedo = {.7f, .4f, .4f}
		}
	};

	Material reflective_mat{
		.type = MaterialType::Metallic,
		.m = {
			.shininess = 1.f
		}
	};

	Material glass_mat{
		.type = MaterialType::Dielectric,
		.d = {
			.refractive_index = 1.3f
		}
	};

	Material blue_mat{
		.type = MaterialType::Lambertian,
		.l = {
			.reflectance = .5f,
			.albedo = {0.f, 0.f, 1.f}
		}
	};
	// Ground sphere
	scene.add_sphere(Sphere{
		.pos = {0.f, -100.f, 0.f},
		.radius = 100.f,
		.material = red_mat
					 });
	/*scene.add_sphere(Sphere{
		.pos = {-.5f, 0.5f, 0.f},
		.radius = .5f,
		.material = reflective_mat
					 });
	scene.add_sphere(Sphere{
		.pos = {.75f, 0.75f, 0.f},
		.radius = .75f,
		.material = blue_mat
					 });
	scene.add_sphere(Sphere{
		.pos = {0.f, 0.7f, -3.f},
		.radius = 0.7f,
		.material = glass_mat
	});*/
	// Sky sphere
	scene.add_sphere(Sphere{
		.pos = {0.f, 5000.f, 0.f},
		.radius = 4000.f,
		.material = {
			.type = MaterialType::Lambertian,
			.l = {
				.reflectance = 0.f,
				.albedo = {1.f, 1.f, 1.f},
				.emissive = {.7f, .7f, .7f},
			}
		}
	});

	RandomDevice rd(5000);

	auto random_ball = [rd = &rd]() -> Sphere {
		auto random_mat = [rd = rd]() -> Material {
			MaterialType mats[]{MaterialType::Lambertian, MaterialType::Dielectric, MaterialType::Metallic};
			MaterialType selected_mat = (MaterialType)((u32)(rd->random_num() * 2.999));
			if (selected_mat == MaterialType::Lambertian) {
				return Material{
					.type = MaterialType::Lambertian,
					.l = {
						.reflectance = rd->random_num(),
						.albedo = {rd->random_num(), rd->random_num(), rd->random_num()},
						.emissive = {rd->random_num() * 0.2f, rd->random_num() * 0.2f, rd->random_num() * 0.2f}
					}
				};
			}
			else if (selected_mat == MaterialType::Metallic) {
				return Material{
					.type = MaterialType::Metallic,
					.m = {
						.shininess = 0.5f + 0.5f * rd->random_num(),
						.emissive = {0.f, 0.f, 0.f}
					}
				};
			}
			else if(selected_mat == MaterialType::Dielectric) {
				return Material{
					.type = MaterialType::Dielectric,
					.d = {
						.refractive_index = 1.3f,
						.emissive = {0.f, 0.f, 0.f}
					}
				};
			}
			else {
				throw;
			}
		};
		number_t rad = .1f + rd->random_num() * .5f;

		return Sphere{
			.pos = Vec3{rd->random_num(-.1f, .1f), 1.f, rd->random_num(-.1f, .1f)}.normalize() * (100.f + rad) + Vec3{0.f, -100.f, 0.f},
			.radius = rad,
			.material = random_mat()
		};
	};
	number_t cur_dist = 0.f;
	for (u32 i = 0; i < ball_count; i++) {
		auto ball = random_ball();
		number_t ang = rd.random_num() * 3.14145f * 2.f;
		cur_dist += rd.random_num() * 0.01;
		ball.pos = Vec3{sin(ang) * cur_dist, 1.f, cos(ang) * cur_dist}.normalize() * (100.f + ball.radius) + Vec3{0.f, -100.f, 0.f};
		scene.add_sphere(ball);
	}

}

class Args {
public:
	Args(int argc, const char* argv[]) {
		for (int i = 0; i < argc; i++) {
			args.push_back(std::string(argv[i]));
		}
	}

	bool has_arg(const std::string& argname) {
		for (auto& a : args) {
			if (a == argname)
				return true;
		}
		return false;
	}
	std::vector<std::string>::const_iterator begin() const { return args.begin(); }
	std::vector<std::string>::const_iterator end() const { return args.end(); }
private:
	std::vector<std::string> args;
};

bool update_renderer_settings(Renderer& renderer, const Args& args) {
	for (const auto& arg : args) {
		if (arg.substr(0, 2) == "-j") {
			std::string nthreads = arg.substr(2);
			renderer.set_thread_count(::atoi(nthreads.c_str()));
		}
		else if (arg.substr(0, 2) == "-s") {
			std::string samplec = arg.substr(2);
			renderer.set_samples(::atoi(samplec.c_str()));
		}
		else if (arg.substr(0, 2) == "-b") {
			std::string bouncec = arg.substr(2);
			renderer.set_bounces(::atoi(bouncec.c_str()));
		}
		else if (arg == "-h") {
			std::cout << "-jN [N threads]" << std::endl;
			std::cout << "-sN [N samples]" << std::endl;
			std::cout << "-bN [N bounces]" << std::endl;
			return false;
		}
	}
	return true;
}

int main(int argc, const char* argv[]) {
	Args args(argc, argv);

	Scene scene;
	generate_scene_1(scene, 4);
	
	Renderer renderer;
	renderer.set_thread_count(std::thread::hardware_concurrency() - 1);
	renderer.set_samples(10);
	renderer.set_bounces(26);
	renderer.set_max_path_steps(300);
	renderer.set_epsilon(0.000001);

	u32 w = 1024, h = 1024;

	Image render_target(w, h);

	Camera cam;
	cam.set_focal_distance(50.f);
	cam.set_position({ 0.f, 50.f, -150.f });
	cam.set_rotation(3.1415 / 10, 0.f, 0.f);

	if (!update_renderer_settings(renderer, args))
		return -1;

	std::cerr << "total min: " << renderer.get_samples() * render_target.width() * render_target.height() << std::endl;
	std::cerr << "total max: " << renderer.get_bounces() * renderer.get_samples() * render_target.width() * render_target.height() << std::endl;

	renderer.render_mt(scene, cam, render_target);

	write_ppm("render/rt.ppm", render_target);
	
	return 0;
}
