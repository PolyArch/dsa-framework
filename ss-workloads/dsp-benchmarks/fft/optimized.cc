#include <complex>
#include <cmath>
#include <algorithm>
#include <iostream>

#define complex_mul(a, b) (a).real() * (b).real() - (a).imag() * (b).imag(), \
  (a).real() * (b).imag() + (a).imag() * (b).real()

#define complex_conj_mul(a, b) (a).real() * (b).real() + (a).imag() * (b).imag(), \
  (a).real() * (b).imag() - (a).imag() * (b).real()

#define complex_add(a, b) (a).real() + (b).real(), (a).imag() + (b).imag()

#define complex_sub(a, b) (a).real() - (b).real(), (a).imag() - (b).imag()

#define complex_norm(a) ((a).real() * (a).real() + (a).imag() * (a).imag())

using std::complex;

complex<float> *fft(complex<float> *from, complex<float> *to, complex<float> *w) {
  for (int blocks = _N_ / 2; blocks; blocks >>= 1) {
    int span = _N_ / blocks;
    for (int j = 0; j < span / 2 * blocks; j += blocks) {
      for (int i = 0; i < blocks; ++i) {
        //printf("%d %d %d\n", blocks, j, i);
        complex<float> &L = from[2 * j + i];
        complex<float> &R = from[2 * j + i + blocks];
        complex<float> tmp(complex_mul(w[j], R));
        to[i + j] = complex<float>(complex_add(L, tmp));
        to[i + j + span / 2 * blocks] = complex<float>(complex_sub(L, tmp));
      }
    }
    //for (int j = 0; j < _N_; ++j)
      //std::cout << to[j] << "\n";
    //std::cout << "\n";
    swap(from, to);
  }
  return from;
}
