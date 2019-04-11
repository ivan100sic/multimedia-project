#include "video.h"
#include <cmath>
using namespace std;
using namespace video;

template<class T>
void mix(T& a, T b) {
	long r = (a+4l*b) / 5;
	if (r > numeric_limits<T>::max())
		r = numeric_limits<T>::max();
	if (r < numeric_limits<T>::min())
		r = numeric_limits<T>::min();
	a = r;
}

template<class T>
void amplify(T& a) {
	long r = a * sqrt(5);
	if (r > numeric_limits<T>::max())
		r = numeric_limits<T>::max();
	if (r < numeric_limits<T>::min())
		r = numeric_limits<T>::min();
	a = r;
}

void mix(pix& a, pix b) {
	mix(a.r, b.r);
	mix(a.g, b.g);
	mix(a.b, b.b);
}


int main(int argc, char** argv) {
	if (argc != 2)
		return 1;
	Video vid(argv[1]);
	vid.load();

	auto& v = vid.v_data;
	auto& a = vid.a_data;

	for (int f=0; f<(int)v.frames; f++) {
		int g = f - 1;
		if (g < 0)
			continue;

		for (int x=0; x<(int)v.width; x++) {
			for (int y=0; y<(int)v.height; y++) {
				mix(v(f, x, y), v(g, x, y));
			}
		}
	}

	for (int f=0; f<(int)a.samples; f++) {
		int g = f - 4800;
		if (g < 0)
			continue;
		for (int x=0; x<(int)a.channels; x++) {
			mix(a(f, x), a(g, x));
		}
	}

	for (int f=0; f<(int)a.samples; f++) {
		for (int x=0; x<(int)a.channels; x++) {
			amplify(a(f, x));
		}
	}
	
	vid.save();
	vid.finish();
}