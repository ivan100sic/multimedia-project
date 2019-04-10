#pragma once
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <vector>
#include <string>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <ios>

namespace video {

typedef uint8_t pix_component;
typedef int16_t sample_component;
typedef std::vector<sample_component> sample;

struct pix {
	pix_component r, g, b;
};

struct video_metadata {
	int width;
	int height;
	int sample_rate;
	int channels;
	int audio_bitrate;
	int video_bitrate;
	std::string framerate;
	std::string pix_fmt;
};

int run(std::string cmd) {
	return system(cmd.c_str());
}

std::ifstream::pos_type filesize(const char* filename) {
    std::ifstream in(filename, std::ifstream::ate | std::ifstream::binary);
    return in.tellg(); 
}

video_metadata prepare(std::string f, std::string fnum) {
	using std::string;
	using std::ifstream;
	using std::vector;
	// extract raw video
	run(string("rm -f ") + "/dev/shm/"  + fnum + ".rawvideo");
	run(string("ffmpeg -i ") + f + ".in -f rawvideo -pix_fmt rgb24 "
		+ "/dev/shm/" + fnum + ".rawvideo");

	// extract raw audio
	run(string("rm -f ") + "/dev/shm/"  + fnum + ".rawaudio");
	run(string("ffmpeg -i ") + f + ".in -vn -f s16le -acodec pcm_s16le "
		+ "/dev/shm/" + fnum + ".rawaudio");

	// extract metadata
	// ffprobe -show_streams -show_format test1.mp4
	run(string("ffprobe -show_streams -show_format ") + f + ".in > "
		+ "/dev/shm/" + fnum + ".meta");

	video_metadata metadata;
	ifstream metafile("/dev/shm/" + fnum + ".meta");
	string ln;
	vector<string> codec_types;
	vector<int> bitrates;
	bool rotated = false;
	while (getline(metafile, ln)) {
		auto k = ln.find('=');
		if (k != string::npos) {
			string key, val;
			key = ln.substr(0, k);
			val = ln.substr(k+1);
			if (key == "width") {
				metadata.width = stoi(val);
			} else if (key == "height") {
				metadata.height = stoi(val);
			} else if (key == "pix_fmt") {
				metadata.pix_fmt = val;
			} else if (key == "r_frame_rate") {
				if (val[0] != '0')
					metadata.framerate = val;
			} else if (key == "channels") {
				metadata.channels = stoi(val);
			} else if (key == "sample_rate") {
				metadata.sample_rate = stoi(val);
			} else if (key == "codec_type") {
				codec_types.push_back(val);
			} else if (key == "bit_rate") {
				bitrates.push_back(stoi(val));
			} else if (key == "TAG:rotate") {
				if (val == "90" || val == "270")
					rotated = true;
			}
		}
	}

	if (rotated)
		std::swap(metadata.width, metadata.height);

	for (size_t i=0; i<std::min(codec_types.size(), bitrates.size()); i++) {
		if (codec_types[i] == "video") {
			metadata.video_bitrate = bitrates[i];
		} else if (codec_types[i] == "audio") {
			metadata.audio_bitrate = bitrates[i];
		}
	}

	return metadata;
}

struct video_view {
	pix* ptr;
	size_t frames, width, height;
	pix& operator() (size_t f, size_t x, size_t y) const {
		return ptr[f*width*height + y*width + x];
	}
};

struct audio_view {
	sample_component* ptr;
	size_t samples, channels;
	sample_component& operator() (size_t s, size_t ch) const {
		return ptr[s*channels + ch];
	}
};

struct Video {
	video_metadata metadata;
	video_view v_data;
	audio_view a_data;
	std::string f;
	std::string fnum;
	int fdv, fda;

	Video(const std::string& f) : f(f) {
		for (char x : f)
			fnum += std::to_string((int)x);
	}

	void load() {
		metadata = prepare(f, fnum);
		using std::string;
		string fnv = string("/dev/shm/") + fnum + ".rawvideo";
		string fna = string("/dev/shm/") + fnum + ".rawaudio";

		fdv = open(fnv.c_str(), O_RDWR);
		fda = open(fna.c_str(), O_RDWR);

		struct stat sb;

		fstat(fdv, &sb);
		v_data.ptr = (pix*)mmap(0, sb.st_size, PROT_READ | PROT_WRITE,
			MAP_SHARED, fdv, 0);
		v_data.width = metadata.width;
		v_data.height = metadata.height;
		v_data.frames = sb.st_size / v_data.width / v_data.height / sizeof(pix);

		fstat(fda, &sb);
		a_data.ptr = (sample_component*)mmap(0, sb.st_size, PROT_READ | PROT_WRITE,
			MAP_SHARED, fda, 0);
		a_data.channels = metadata.channels;
		a_data.samples = sb.st_size / a_data.channels / sizeof(sample_component);
	}

	void save() {
		// nothing to save!
		munmap(v_data.ptr, v_data.frames*v_data.width*v_data.height*sizeof(pix));
		munmap(a_data.ptr, a_data.samples*a_data.channels*sizeof(sample_component));
	}

	void finish() {
		using std::to_string;
		using std::string;
		/*
		ffmpeg -f rawvideo -pix_fmt rgb24 -s 320x240 -framerate 27.77 -i test1.rawvideo\
		-f s16le -sample_rate 48000 -channels 2 -i test1.rawaudio\
		-pix_fmt yuv420p -strict -2 -shortest test2.mp4
		*/
		run(string("rm -f ") + f + ".mp4");

		string cmd = "ffmpeg -f rawvideo -pix_fmt rgb24 -s ";
		cmd += to_string(metadata.width) + "x" + to_string(metadata.height);
		cmd += string(" -framerate ") + metadata.framerate;
		cmd += string(" -i /dev/shm/") + fnum + ".rawvideo ";
		cmd += string(" -f s16le -sample_rate ") + to_string(metadata.sample_rate);
		cmd += string(" -channels ") + to_string(metadata.channels);
		cmd += string(" -i /dev/shm/") + fnum + ".rawaudio ";
		cmd += string(" -pix_fmt ") + metadata.pix_fmt;
		cmd += string(" -b:v ") + to_string(metadata.video_bitrate);
		cmd += string(" -b:a ") + to_string(metadata.audio_bitrate);
		cmd += string(" -shortest -strict -2 ");
		cmd += string(" ") + f + ".mp4";

		// std::cerr << "cmd: " << cmd << '\n';
		run(cmd);
		run(string("mv ") + f + ".mp4 " + f + ".out");
		run(string("rm /dev/shm/") + fnum + ".rawvideo");
		run(string("rm /dev/shm/") + fnum + ".rawaudio");
		run(string("rm /dev/shm/") + fnum + ".meta");
	}
};

} // end namespace video
