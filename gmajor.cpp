#include "video.h"
#include <random>
#include <complex>
#include <chrono>
#include <valarray>
using namespace std;
using namespace video;

inline pix scary_color(pix b) {
	int mxdif = max({abs(127 - b.r), abs(127 - b.g), abs(127 - b.b)}) + 1;
	double scale = -0.98 * 127 / mxdif;
	b.r = 127 + (b.r - 127) * scale;
	b.g = 127 + (b.g - 127) * scale;
	b.b = 127 + (b.b - 127) * scale;
	return b;
}

// http://cp-algorithms.com/algebra/fft.html#toc-tgt-9
using cd = complex<double>;
const double PI = acos(-1);

inline int bit_reverse(int num, int lg_n) {
    int res = 0;
    for (int i = 0; i < lg_n; i++) {
        if (num & (1 << i))
            res |= 1 << (lg_n - 1 - i);
    }
    return res;
}

void fft(valarray<cd>& a, bool invert) {
    int n = a.size();
    int lg_n = 0;
    while ((1 << lg_n) < n)
        lg_n++;

    for (int i = 0; i < n; i++) {
        if (i < bit_reverse(i, lg_n))
            swap(a[i], a[bit_reverse(i, lg_n)]);
    }

    for (int len = 2; len <= n; len <<= 1) {
        double ang = 2 * PI / len * (invert ? -1 : 1);
        cd wlen(cos(ang), sin(ang));
        for (int i = 0; i < n; i += len) {
            cd w(1);
            for (int j = 0; j < len / 2; j++) {
                cd u = a[i+j], v = a[i+j+len/2] * w;
                a[i+j] = u + v;
                a[i+j+len/2] = u - v;
                w *= wlen;
            }
        }
    }

    if (invert) {
        for (cd & x : a)
            x /= n;
    }
}

mt19937 eng(chrono::high_resolution_clock::now().time_since_epoch().count());

void apply_window(valarray<cd>& a) {
	int N = a.size();
	for (int n=0; n<N; n++) {
		double w = 0.5 * (1 - cos(2*PI*n / N));
		a[n] *= w;
	}
}

void randomize_phases(valarray<cd>& a) {
	uniform_real_distribution<double> d(0, 2*PI);
	for (cd& z : a)
		z *= exp(cd(0, d(eng)));
}

void pitch_shift_attempt(valarray<cd>& a, double scale) {
	int L = a.size();
	const int N = 2048;
	valarray<cd> b(L);

	auto extract = [&](int c) {
		int l = c - N/2;
		int r = c + N/2;
		valarray<cd> t(N);
		for (int i=l; i<r; i++) {
			t[i-l] = i >= 0 && i < L ? a[i] : 0;
		}
		return t;
	};

	auto imprint = [&](int c, const valarray<cd>& d) {
		int l = c - N/2;
		int r = c + N/2;
		for (int i=l; i<r; i++) {
			if (i >= 0 && i < L)
				b[i] += d[i-l];
		}
	};

	for (int c=0; c<L; c+=N/4) {
		auto e = extract(c);
		apply_window(e);
		fft(e, false);
		valarray<cd> f(N);
		for (int i=0; i<N/2; i++) {
			double lo = i*scale;
			double hi = (i+1)*scale;
			for (int j=lo-2; j<=hi+2; j++) {
				double overlap = max(j+.0, lo) - min(j+1., hi);
				if (j >= 0 && j < N/2)
					f[j] += e[i] * overlap / scale;
			}
		}
		randomize_phases(f);
		fft(f, true);
		apply_window(f);
		imprint(c, f);
	}

	swap(a, b);
}

void pitch_shift(audio_view& a, double scale) {

	auto clip = [](cd z) -> sample_component {
		if (z.real() < -32767)
			return -32768;
		if (z.real() > 32766)
			return 32767;
		return z.real();
	};

	for (int i=0; i<(int)a.channels; i++) {
		valarray<cd> b(a.samples);
		for (int j=0; j<(int)a.samples; j++) {
			b[j] = a(j, i);
		}
		valarray<cd> bplus3 = b;
		valarray<cd> bminus3 = b;
		pitch_shift_attempt(bplus3, pow(2, 0.3333));
		pitch_shift_attempt(bminus3, pow(0.5, 0.3333));
		for (int j=0; j<(int)a.samples; j++) {
			a(j, i) = clip(b[j] + bplus3[j] + bminus3[j] * 1.5);
		}
	}
}

int main(int argc, char** argv) {
	if (argc != 2)
		return 1;
	Video vid(argv[1]);
	vid.load();

	// make the video scary
	auto& v = vid.v_data;
	for (int f=0; f<(int)v.frames; f++) {
		for (int x=0; x<(int)v.width; x++) {
			for (int y=0; y<(int)v.height; y++) {
				v(f, x, y) = scary_color(v(f, x, y));
			}
		}
	}

	pitch_shift(vid.a_data, 1);

	vid.save();
	vid.finish();
}