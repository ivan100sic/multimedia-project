#include "video.h"
#include <algorithm>
using namespace std;
using namespace video;

int main() {
	Video vid("videos/letimo");
	vid.load();
	
	reverse(vid.v_data.begin(), vid.v_data.end());
	for (auto& fr : vid.v_data) {
		for (auto& rw : fr) {
			for (auto& px : rw)
				swap(px.r, px.g);
			rotate(rw.begin(), rw.begin()+30, rw.end());
		}
	}
	reverse(vid.a_data.begin(), vid.a_data.end());
	
	vid.save();
	vid.finish();
}