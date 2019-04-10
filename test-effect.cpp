#include "video.h"
using namespace std;
using namespace video;

int main(int argc, char** argv) {
	if (argc != 2)
		return 1;
	Video vid(argv[1]);
	vid.load();

	// reverse video
	
	for (int i=0, j=(int)vid.v_data.frames-1; i<j; i++,j--) {
		// cerr << "swapping frames " << i << ' ' << j << '\n';
		for (int x=0; x<(int)vid.v_data.width; x++) {
			for (int y=0; y<(int)vid.v_data.height; y++) {
				swap(vid.v_data(i, x, y), vid.v_data(j, x, y));
			}
		}
	}

	// reverse audio
	for (int i=0, j=(int)vid.a_data.samples-1; i<j; i++,j--) {
		for (int x=0; x<(int)vid.a_data.channels; x++) {
			swap(vid.a_data(i, x), vid.a_data(j, x));
		}
	}
	// mute the left channel
	for (int i=0; i<(int)vid.a_data.samples; i++) {
		vid.a_data(i, 0) = 0;
	}

	// swap red and green, remove blue
	
	for (int i=0; i<(int)vid.v_data.frames; i++) {
		// cerr << "recoloring frame " << i << '\n';
		for (int x=0; x<(int)vid.v_data.width; x++) {
			for (int y=0; y<(int)vid.v_data.height; y++) {
				swap(vid.v_data(i, x, y).r, vid.v_data(i, x, y).g);
				vid.v_data(i, x, y).b = 0;
			}
		}
	}
	
	vid.save();
	vid.finish();
}