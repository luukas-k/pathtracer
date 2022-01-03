#pragma once

#include "math.h"

#include <vector>
#include <sstream>
#include <fstream>

class Image {
public:
	Image(u32 w, u32 h)
		:
		w(w), h(h) {
		// Init to black image
		data.resize(w * h, Vec3{ 0.f, 0.f, 0.f });
	}
	Image(u32 w, u32 h, const Image& img) :
		w(w), h(h) {
		data.resize(w * h, Vec3{ 0.f, 0.f, 0.f });

		auto read_color = [img = &img](number_t x, number_t y) {
			// Clamp x and y to image size
			if(x < 0) x = 0.f;
			if(y < 0) y = 0.f;
			if(x > img->width()) x = (number_t)img->width();
			if(y > img->height()) y = (number_t)img->height();
			// Sample from image
			return img->get((u32)x, (u32)y);
		};

		auto sample = [read_color, dest_w = w, dest_h = h, img = &img](u32 x, u32 y) -> Vec3 {
			number_t 
				src_w = (float)img->width(),
				src_h = (float)img->height();

			number_t 
				xfac = (float)x / (float)dest_w, 
				yfac = (float)y / (float)dest_h;

			number_t 
				target_x = xfac * src_w,
				target_y = yfac * src_h;

			// Do the actual image sampling 

			/*
			Sample like below although with half pixel offsets
			| | | | | | |
			| | | | | | |
			| | |x|x| | |
			| | |x|x| | |
			| | | | | | |	
			| | | | | | |		
			*/
			return (
				read_color(target_x - 0.5f, target_y + 0.5f) + 
				read_color(target_x + 0.5f, target_y + 0.5f) +
				read_color(target_x - 0.5f, target_y - 0.5f) + 
				read_color(target_x + 0.5f, target_y + 0.5f)
			) * (1.f / 4.f);
		};

		for (u32 i = 0; i < w; i++) {
			for (u32 k = 0; k < h; k++) {
				// For each pixel calculate the resulting pixel
				get(i, k) = sample(i, k);
			}
		}
	}


	u32 width() const { return w; }
	u32 height() const { return h; }

	Vec3& get(u32 x, u32 y) { return data[x + y * w]; }
	const Vec3& get(u32 x, u32 y) const { return data[x + y * w]; }
private:
	std::vector<Vec3> data;
	u32 w, h;
};

void write_ppm(const std::string& filepath, const Image& img) {
	std::stringstream ss;

	u32
		w = img.width(),
		h = img.height();


	ss << "P3\n";
	ss << w << " " << h << "\n";

	ss << "255\n";

	for (u32 y = 0; y < h; y++) {
		for (u32 x = 0; x < w; x++) {
			ss
				<< (u32)(clamp(img.get(x, y).x, 0.f, 1.f) * 255.f) << " "
				<< (u32)(clamp(img.get(x, y).y, 0.f, 1.f) * 255.f) << " "
				<< (u32)(clamp(img.get(x, y).z, 0.f, 1.f) * 255.f) << "\n";
		}
		ss << "\n";
	}

	std::ofstream fs(filepath);

	if (!fs) {
		return;
	}
	std::string img_str = ss.str();
	fs.write(img_str.c_str(), img_str.size());
	fs.close();

	std::string start_cmd = "start " + std::string(filepath);
	std::system(start_cmd.c_str());
}