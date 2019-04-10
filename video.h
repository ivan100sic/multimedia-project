#pragma once
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

video_metadata prepare(std::string f) {
	using std::string;
	using std::ifstream;
	using std::vector;
	// extract raw video
	run(string("rm -f ") + f + ".rawvideo");
	run(string("ffmpeg -i ") + f + ".in -f rawvideo -pix_fmt rgb24 "
		+ f + ".rawvideo");

	// extract raw audio
	run(string("rm -f ") + f + ".rawaudio");
	run(string("ffmpeg -i ") + f + ".in -vn -f s16le -acodec pcm_s16le "
		+ f + ".rawaudio");

	// extract metadata
	// ffprobe -show_streams -show_format test1.mp4
	run(string("ffprobe -show_streams -show_format ") + f + ".in > "
		+ f + ".meta");

	video_metadata metadata;
	ifstream metafile(f + ".meta");
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

struct Video {
	video_metadata metadata;
	std::vector<std::vector<std::vector<pix>>> v_data;
	std::vector<sample> a_data;
	std::string f;

	Video(const std::string& f) : f(f) {}

	void load() {
		using std::vector;
		using std::ifstream;
		using std::ios;
		using std::copy;
		// convert to raw video/audio/meta and then load it
		metadata = prepare(f);
		int w = metadata.width;
		int h = metadata.height;
		// read video
		ifstream vf(f + ".rawvideo", ios::binary);
		pix* fb = new pix[w*h];
		while (vf.read((char*)fb, w*h*sizeof(pix))) {
			vector<vector<pix>> frame(h, vector<pix>(w));
			for (int i=0; i<h; i++) {
				copy(fb+i*w, fb+(i+1)*w, frame[i].begin());
			}
			v_data.emplace_back(move(frame));
		}
		delete[] fb;
		// read audio
		// TODO
		int ch = metadata.channels;
		sample_component* sb = new sample_component[ch];
		ifstream af(f + ".rawaudio", ios::binary);
		while (af.read((char*)sb, ch*sizeof(sample_component))) {
			sample smpl(ch);
			copy(sb, sb+ch, smpl.begin());
			a_data.emplace_back(move(smpl));
		}
		delete[] sb;
	}

	void save() {
		using std::vector;
		using std::ofstream;
		using std::ios;
		// convert to raw video/audio/meta and then load it
		// write video
		ofstream vf(f + ".rawvideo", ios::binary);
		for (const auto& frame : v_data) {
			for (const auto& row : frame) {
				vf.write((char*)row.data(), row.size() * sizeof(pix));
			}
		}
		// write audio
		// TODO
		ofstream af(f + ".rawaudio", ios::binary);
		for (const auto& smpl : a_data)
			af.write((char*)smpl.data(), smpl.size() * sizeof(sample_component));
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
		cmd += string(" -i ") + f + ".rawvideo ";
		cmd += string(" -f s16le -sample_rate ") + to_string(metadata.sample_rate);
		cmd += string(" -channels ") + to_string(metadata.channels);
		cmd += string(" -i ") + f + ".rawaudio ";
		cmd += string(" -pix_fmt ") + metadata.pix_fmt;
		cmd += string(" -b:v ") + to_string(metadata.video_bitrate);
		cmd += string(" -b:a ") + to_string(metadata.audio_bitrate);
		cmd += string(" -shortest -strict -2 ");
		cmd += string(" ") + f + ".mp4";

		std::cerr << "cmd: " << cmd << '\n';
		int status = run(cmd);
		std::cerr << "ffmpeg command status: " << status << '\n';
		run(string("mv ") + f + ".mp4 " + f + ".out");
	}
};

} // end namespace video
