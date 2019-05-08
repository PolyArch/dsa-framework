#include "svd.h"
#include "ss_insts.h"
#include "vmc.dfg.h"
#include "vv.dfg.h"
#include "mvc.dfg.h"
#include "vvc.dfg.h"
#include "vm.dfg.h"
#include "aplygvs.dfg.h"
#include "finalize.dfg.h"

#define complex_mul(a, b) (a).real() * (b).real() - (a).imag() * (b).imag(), \
  (a).real() * (b).imag() + (a).imag() * (b).real()

#define complex_mul_cons(a, b) (a).real() * (b), (a).imag() * (b)

#define complex_conj_mul(a, b) (a).real() * (b).real() + (a).imag() * (b).imag(), \
  (a).real() * (b).imag() - (a).imag() * (b).real()

#define complex_add(a, b) (a).real() + (b).real(), (a).imag() + (b).imag()

#define complex_sub(a, b) (a).real() - (b).real(), (a).imag() - (b).imag()

#define complex_norm(a) ((a).real() * (a).real() + (a).imag() * (a).imag())

complex<float> f[_N_], d[_N_], r[_N_ * _N_], temp[_N_];
complex<float> _one(1, 0), _zero(0, 0);

void household(complex<float> *v, int n, complex<float> &alpha) {
  float norm0 = complex_norm(v[0]), norm1 = 0;
  for (int j = 1; j < n; ++j) {
    norm1 += complex_norm(v[j]);
  }
  float _alpha = sqrt(1 + norm1 / norm0);
  alpha = complex<float>(-v->real() * _alpha, -v->imag() * _alpha);
  float rate = 1 + _alpha;
  v[0] = complex<float>(v->real() * rate, v->imag() * rate);
  norm1 += complex_norm(*v);
  norm1 = 1. / sqrt(norm1);
  for (int j = 0; j < n; ++j) {
    v[j] = complex<float>(v[j].real() * norm1, v[j].imag() * norm1);
  }
}

void givens(complex<float> &a, complex<float> &b, complex<float> &alpha) {
  float norm0 = complex_norm(a), norm1 = complex_norm(b);
  float r = sqrt(norm1 + norm0);
  complex<float> c(std::conj(a) / r);
  complex<float> s(std::conj(b) / r);
  alpha = r;
  a = c;
  b = s;
}

/* L Givens Rotation
 * (c        s     ) (a)
 * (s.conj  -c.conj) (b)
 * */
#define apply_givens(c, s, a, b) \
  do { \
    complex<float> \
      resa((c) * (a) + (s) * (b)), \
      resb(std::conj(s) * (a) - std::conj(c) * (b)); \
    a = resa; \
    b = resb; \
  } while(false)

void implicit_kernel(complex<float> *d, complex<float> *f, complex<float> *v, int n) {
  float mu = complex_norm(d[n - 1]);
  complex<float> a(complex_norm(d[0]) - mu), b(complex_conj_mul(f[0], d[0])), alpha;
  complex<float> c, s, extra;
  givens(a, b, alpha);

  c = a; s = b;
  SS_CONFIG(aplygvs_config, aplygvs_size);
  SS_CONST(P_aplygvs_C, *((uint64_t*)&c), _N_);
  SS_CONST(P_aplygvs_S, *((uint64_t*)&s), _N_);
  SS_DMA_READ(v, 8, 8, _N_, P_aplygvs_A);
  SS_DMA_READ(v + _N_, 8, 8, _N_, P_aplygvs_B);
  SS_DMA_WRITE(P_aplygvs_O0, 8, 8, _N_, v);
  SS_DMA_WRITE(P_aplygvs_O1, 8, 8, _N_, v + _N_);
  //SS_WAIT_ALL();
  //for (int i = 0; i < _N_; ++i) {
  //  lmm2x2(m0, m1, v[i], v[i + _N_]);
  //}

  a = d[0] * c + f[0] * std::conj(s);
  f[0] = d[0] * s - f[0] * std::conj(c);
  b = d[1] * std::conj(s);
  d[1] *= -std::conj(c);

  givens(a, b, d[0]);
  apply_givens(a, b, f[0], d[1]);
  if (n != 2) {
    b *= f[1];
    f[1] *= -std::conj(a);
  }

  for (int i = 1; i < n - 1; ++i) {
    a = f[i - 1];
    givens(a, b, f[i - 1]);
    c = a; s = b;
    a = d[i];

    a = d[i] * c + f[i] * s;
    f[i] = d[i] * std::conj(s) - f[i] * std::conj(c);
    b = d[i + 1] * s;
    d[i + 1] *= -std::conj(c);

    SS_CONST(P_aplygvs_C, *((uint64_t*)&c), _N_);
    SS_CONST(P_aplygvs_S, *((uint64_t*)&s), _N_);
    SS_DMA_READ(v + i * _N_, 8, 8, _N_, P_aplygvs_A);
    SS_DMA_READ(v + i * _N_ + _N_, 8, 8, _N_, P_aplygvs_B);
    SS_DMA_WRITE(P_aplygvs_O0, 8, 8, _N_, v + i * _N_);
    SS_DMA_WRITE(P_aplygvs_O1, 8, 8, _N_, v + i * _N_ + _N_);
    //for (int j = 0; j < _N_; ++j) {
    //  lmm2x2(m0, m1, v[i * _N_ + j], v[(i + 1) * _N_ + j]);
    //}

    givens(a, b, d[i]);
    c = a; s = b;
    apply_givens(c, s, f[i], d[i + 1]);

    if (i != n - 2) {
      b = s * f[i + 1];
      f[i + 1] *= -std::conj(c);
    }
  }
  SS_WAIT_ALL();
}

void svd(complex<float> *a, complex<float> *u, float *s, complex<float> *v) {
  for (int i = 0; i < _N_ - 1; ++i) {
    int len = _N_ - i;
    complex<float> hv[len], alpha;
    for (int j = 0; j < len; ++j)
      hv[j] = (i ? r : a)[j * len];
    household(hv, len, d[i]);

    SS_CONFIG(vmc_config, vmc_size);
    SS_CONST(P_vmc_C, *((uint64_t*)&_zero), len - 1);
    SS_DMA_READ((i ? r : a) + 1, 8 * len, 8 * (len - 1), len, P_vmc_B);
    SS_RECURRENCE(P_vmc_O, P_vmc_C, (len - 1) * (len - 1));
    SS_REPEAT_PORT(len - 1);
    SS_DMA_READ(hv, 8, 8, len, P_vmc_A);
    //for (int k = 0; k < len; ++k) {
    //  SS_CONST(P_vmc_A, *((uint64_t*)(hv + k)), len - 1);
    //}
    SS_DMA_WRITE(P_vmc_O, 8, 8, len - 1, temp + 1);
    SS_WAIT_ALL();

    //for (int j = 1; j < len; ++j) std::cout << temp[j] << " "; std::cout << "\n";

    //for (int j = 1; j < len; ++j)
    //  temp[j] = 0;
    //for (int k = 0; k < len; ++k) {
    //  for (int j = 1; j < len; ++j) {
    //    temp[j] += complex<float>(complex_conj_mul(hv[k], (i ? r : a)[k * len + j]));
    //  }
    //}

    SS_CONFIG(vv_config, vv_size);
    SS_DMA_READ((i ? r : a) + 1, 8 * len, 8 * (len - 1), len, P_vv_C);
    SS_DMA_READ(temp + 1, 0, 8 * (len - 1), len, P_vv_A);
    SS_DMA_WRITE(P_vv_O, 8, 8, (len - 1) * len, r);
    for (int j = 0; j < len; ++j) {
      SS_CONST(P_vv_B, *((uint64_t*)(hv + j)), len - 1);
    }
    SS_WAIT_ALL();
    
    //for (int j = 1; j < len; ++j)
    //  temp[j] = complex<float>(temp[j].real() * 2, temp[j].imag() * 2);
    //for (int j = 0; j < len; ++j) {
    //  for (int k = 1; k < len; ++k) {
    //    complex<float> delta(complex_mul(temp[k], hv[j]));
    //    r[j * (len - 1) + (k - 1)] = complex<float>(complex_sub((i ? r : a)[j * len + k], delta));
    //  }
    //}
    //for (int j = 0; j < len; ++j) { for (int k = 0; k < len - 1; ++k) std::cout << r[j * (len - 1) + k] << " "; std::cout << "\n"; }

    if (i != _N_ - 2) {
      --len;
      for (int j = 0; j < len; ++j)
        hv[j] = r[j];
      household(hv, len, f[i]);

      SS_CONFIG(mvc_config, mvc_size);
      SS_DMA_READ(hv, 0, 8 * len, len, P_mvc_A);
      SS_DMA_READ(r + len, 8, 8, len * len, P_mvc_B);
      for (int j = 0; j < len; ++j) {
        SS_CONST(P_mvc_reset, 0, len - 1);
        SS_CONST(P_mvc_reset, 1, 1);
        SS_GARBAGE(P_mvc_O, len - 1);
        SS_DMA_WRITE(P_mvc_O, 8, 8, 1, temp + j);
      }
      SS_WAIT_ALL();
      //for (int j = 0; j < len; ++j) {
      //  temp[j] = 0;
      //  for (int k = 0; k < len; ++k) {
      //    complex<float> delta(complex_conj_mul(hv[k], r[(j + 1) * len + k]));
      //    temp[j] = complex<float>(complex_add(temp[j], delta));
      //  }
      //}

      SS_CONFIG(vv_config, vv_size);
      SS_DMA_READ(r + len, 8, 8, len * len, P_vv_C);
      SS_DMA_READ(hv, 0, 8 * len, len, P_vv_A);
      SS_DMA_WRITE(P_vv_O, 8, 8, len * len, r);
      for (int j = 0; j < len; ++j) {
        SS_CONST(P_vv_B, *((uint64_t*)(temp + j)), len);
      }
      SS_WAIT_ALL();
      //for (int j = 0; j < len; ++j) {
      //  for (int k = 0; k < len; ++k) {
      //    complex<float> delta(complex_mul(temp[j], hv[k]));
      //    delta = complex<float>(complex_mul_cons(delta, 2));
      //    r[j * len + k] = complex<float>(complex_sub(r[(j + 1) * len + k], delta));
      //  }
      //}
      
      if (!i) {
        v[0] = complex<float>(1, 0);
        SS_CONFIG(vvc_config, vvc_size);
        SS_DMA_READ(hv, 0, 8 * (_N_ - 1), _N_ - 1, P_vvc_B);
        SS_DMA_WRITE(P_vvc_O, 8 * _N_, 8 * (_N_ - 1), _N_ - 1, v + _N_ + 1);
        for (int j = 1; j < _N_; ++j) {
          v[j] = v[j * _N_] = complex<float>(0, 0);
          SS_CONST(P_vvc_A, *((uint64_t*)(hv + j - 1)), _N_ - 1);
          SS_CONST(P_vvc_C, *((uint64_t*)&_one), 1);
          if (j != _N_ - 1) {
            SS_CONST(P_vvc_C, *((uint64_t*)&_zero), _N_ - 1);
          }
        }
        SS_WAIT_ALL();
        //for (int j = 1; j < _N_; ++j) {
        //  v[j] = v[j * _N_] = complex<float>(0, 0);
        //  for (int k = 1; k < _N_; ++k) {
        //    complex<float> delta(complex_conj_mul(hv[j - 1], hv[k - 1]));
        //    complex<float> diag(j == k, 0);
        //    complex<float> val(delta.real() * 2, delta.imag() * 2);
        //    v[j * _N_ + k] = complex<float>(complex_sub(diag, val));
        //  }
        //}
      } else {
        SS_CONFIG(vm_config, vm_size);
        SS_CONST(P_vm_C, *((uint64_t*)&_zero), _N_ - 1);
        SS_DMA_READ(v + _N_ * (i + 1) + 1, 8 * _N_, 8 * (_N_ - 1), len, P_vm_B);
        SS_RECURRENCE(P_vm_O, P_vm_C, (_N_ - 1) * (len - 1));
        SS_DMA_WRITE(P_vm_O, 8, 8, _N_ - 1, temp + 1);
        for (int j = i + 1; j < _N_; ++j) {
          SS_CONST(P_vm_A, hv + j - i - 1, _N_ - 1);
        }
        SS_WAIT_ALL();
        //for (int k = 1; k < _N_; ++k)
        //  temp[k] = 0;
        //for (int j = i + 1; j < _N_; ++j) {
        //  for (int k = 1; k < _N_; ++k) {
        //    complex<float> delta(complex_mul(hv[j - i - 1], v[j * _N_ + k]));
        //    temp[k] = complex<float>(complex_add(temp[k], delta));
        //  }
        //}
        SS_CONFIG(vvc_config, vvc_size);
        SS_DMA_READ(temp + 1, 0, 8 * (_N_ - 1), len, P_vvc_B);
        SS_DMA_READ(v + _N_ * (i + 1) + 1, 8 * _N_, 8 * (_N_ - 1), len, P_vvc_C);
        SS_DMA_WRITE(P_vvc_O, 8 * _N_, 8 * (_N_ - 1), len, v + _N_ * (i + 1) + 1);
        for (int k = 0; k < len; ++k) {
          SS_CONST(P_vvc_A, *((uint64_t*)(hv + k)), _N_ - 1);
        }
        SS_WAIT_ALL();
        //for (int k = 1; k < _N_; ++k)
        //  temp[k] = complex<float>(temp[k].real() * 2, temp[k].imag() * 2);
        //for (int k = 1; k < _N_; ++k) {
        //  for (int j = i + 1; j < _N_; ++j) {
        //    complex<float> delta(complex_conj_mul(hv[j - i - 1], temp[k]));
        //    v[j * _N_ + k] -= delta;
        //  }
        //}
      }
    }
  }
  f[_N_ - 2] = r[0];
  d[_N_ - 1] = r[1];

  int left = 0, right = _N_ - 1;
  while (left < right) {
    while (left < _N_ - 1 && fabs(f[left].real()) < eps && fabs(f[left].imag()) < eps)
      ++left;
    while (right >= 1 && fabs(f[right - 1].real()) < eps && fabs(f[right - 1].imag()) < eps)
      --right;
    if (right - left >= 1) {
      //std::cout << left << " " << right << "\n";
      implicit_kernel(d + left, f + left, v + left * _N_, right - left + 1);
      //for (int i = left; i < right; ++i) std::cout << f[i] << " "; std::cout << "\n";
      //for (int i = left; i < right + 1; ++i) std::cout << d[i] << " "; std::cout << "\n";
    }
  }

  int N = _N_;

  for (int i = 0; i < _N_; ++i) {
    s[i] = sqrt(complex_norm(d[i]));
  }

  //d[N - 1] = sqrt(complex_norm(d[N - 1]));

  SS_CONFIG(finalize_config, finalize_size);
  for (int i = 0; i < _N_; ++i) {
    for (int j = 0; j < _N_; ++j) {
      SS_DMA_READ(v + j * _N_, 8, 8, _N_, P_finalize_A);
      SS_DMA_READ(a + i * _N_, 8, 8, _N_, P_finalize_B);
      SS_CONST(P_finalize_reset, 0, _N_ / 4 - 1)
      SS_CONST(P_finalize_reset, 1, 1)
      SS_GARBAGE(P_finalize_O, _N_ / 4 - 1);
      SS_DMA_WRITE(P_finalize_O, 0, 8, 1, u + i * _N_ + j);
    }
  }
  SS_WAIT_ALL();

  //for (int i = 0; i < _N_; ++i) {
  //  for (int j = 0; j < _N_; ++j) {
  //    complex<float> sum(0, 0);
  //    for (int k = 0; k < _N_; ++k)
  //      sum += complex<float>(complex_conj_mul(v[j * _N_ + k], a[i * _N_ + k]));
  //    u[i * _N_ + j] = sum;
  //  }
  //}
  for (int i = 0; i < _N_; ++i) {
    for (int j = 0; j < _N_; ++j) {
      u[i * _N_ + j] /= s[j];
    }
  }

}
