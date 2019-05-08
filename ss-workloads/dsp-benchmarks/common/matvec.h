#ifndef MATVEC_H
#define MATVEC_H

#include <complex>
#include <algorithm>
#include "ss_insts.h"
//#include "mmv.dfg.h"
//#include "mmvc.dfg.h"
//#include "vmm.dfg.h"
//#include "vcmm.dfg.h"
//#include "subouterx2.dfg.h"
//#include "suboutercx2.dfg.h"

#define complex_mul(a, b) (a).real() * (b).real() - (a).imag() * (b).imag(), \
  (a).real() * (b).imag() + (a).imag() * (b).real()

#define complex_mul_cons(a, b) (a).real() * (b), (a).imag() * (b)

#define complex_conj_mul(a, b) (a).real() * (b).real() + (a).imag() * (b).imag(), \
  (a).real() * (b).imag() - (a).imag() * (b).real()

#define complex_add(a, b) (a).real() + (b).real(), (a).imag() + (b).imag()

#define complex_sub(a, b) (a).real() - (b).real(), (a).imag() - (b).imag()

#define complex_norm(a) ((a).real() * (a).real() + (a).imag() * (a).imag())

#define get_pad(n, vec_width) ((n) % (vec_width) ? (vec_width) - ((n) % vec_width) : 0)

#define CPUvec_norm(from, ext, vec, res) \
  do { \
    res = 0; \
    for (int _i_(from); _i_ < (from) + (ext); ++_i_) \
      res += complex_norm((vec)[_i_]); \
  } while(false) 

#define CPUvec_norm_and_copy(ext, vec, copy, res) \
  do { \
    res = 0; \
    for (int _i_ = 0; _i_ < (ext); ++_i_) { \
      res += complex_norm((vec)[_i_]); \
      (copy)[_i_] = (vec)[_i_]; \
    } \
  } while(false) 

#define CPUmat_mul_vec(mat, mat_height, mat_width, mat_stride, vec, is_conj, res) \
  do { \
    for (int _i_(0); _i_ < (mat_height); ++_i_) { \
      complex<float> sum(0, 0); \
      for (int _j_(0); _j_ < (mat_width); ++_j_) \
        sum += (mat)[_i_ * (mat_stride) + _j_] * ((is_conj) ? std::conj((vec)[_j_]) : (vec)[_j_]); \
      (res)[_i_] = sum; \
    } \
  } while(false)

#define SBmat_mul_vec(mat, mat_height, mat_width, mat_stride, vec, is_conj, res) \
  do { \
    int pad(get_pad(mat_width, 4)); \
    int A(!is_conj ? P_mmv_A : P_mmvc_A); \
    int B(!is_conj ? P_mmv_B : P_mmvc_B); \
    int O(!is_conj ? P_mmv_O : P_mmvc_O); \
    int reset(!is_conj ? P_mmv_reset : P_mmvc_reset); \
    if (is_conj) {\
      SS_CONFIG(mmvc_config, mmvc_size); \
    } else { \
      SS_CONFIG(mmv_config, mmv_size); \
    } \
    for (int _i_(0); _i_ < (mat_height); ++_i_) { \
      SS_DMA_READ((mat) + _i_ * (mat_stride), 8, 8, mat_width, A); \
      SS_CONST(A, 0, pad); \
      SS_DMA_READ((vec), 8, 8, mat_width, B); \
      SS_CONST(B, 0, pad); \
      SS_CONST(reset, 0, ((mat_width) + pad) / 4 - 1); \
      SS_CONST(reset, 1, 1); \
      SS_GARBAGE(O, ((mat_width) + pad) / 4 - 1); \
      SS_DMA_WRITE(O, 8, 8, 1, (res) + _i_); \
    } \
    SS_WAIT_ALL(); \
  } while(false)

#define REVELmat_mul_vec(mat, mat_height, mat_width, mat_stride, vec, is_conj, res) \
  do { \
    int pad(get_pad(mat_width, 4)); \
    int A(is_conj ? P_mmvc_A : P_mmv_A); \
    int B(is_conj ? P_mmvc_B : P_mmv_B); \
    int O(is_conj ? P_mmvc_O : P_mmv_O); \
    int reset(is_conj ? P_mmvc_reset : P_mmv_reset); \
    if (is_conj) {\
      SS_CONFIG(mmvc_config, mmvc_size); \
    } else { \
      SS_CONFIG(mmv_config, mmv_size); \
    } \
    SS_FILL_MODE(STRIDE_ZERO_FILL); \
    SS_DMA_READ(mat, 8 * (mat_stride), 8 * (mat_width), mat_height, A); \
    SS_DMA_READ((vec), 0, 8 * (mat_width), mat_height, B); \
    SS_2D_CONST(reset, 2, ((mat_width) + pad) / 4 - 1, 1, 1, mat_height); \
    SS_DMA_WRITE(O, 8, 8, mat_height, res); \
    SS_WAIT_ALL(); \
  } while(false)

#define CPUvec_mul_mat(mat, mat_height, mat_width, mat_stride, vec, is_conj, res) \
  do { \
    for (int _i_(0); _i_ < (mat_width); ++_i_) \
      (res)[_i_] = complex<float>(0, 0); \
    for (int _j_(0); _j_ < (mat_height); ++_j_) \
      for (int _i_(0); _i_ < (mat_width); ++_i_) \
        (res)[_i_] += (mat)[_j_ * (mat_stride) + _i_] * \
            ((is_conj) ? std::conj((vec)[_j_]) : (vec)[_j_]); \
  } while (false)

#define SBvec_mul_mat(mat, mat_height, mat_width, mat_stride, vec, is_conj, res) \
  do { \
    int pad(get_pad((mat_width), 4)); \
    int A(is_conj ? P_vcmm_A : P_vmm_A); \
    int B(is_conj ? P_vcmm_B : P_vmm_B); \
    int C(is_conj ? P_vcmm_C : P_vmm_C); \
    int O(is_conj ? P_vcmm_O : P_vmm_O); \
    if (is_conj) { \
      SS_CONFIG(vcmm_config, vcmm_size); \
    } else { \
      SS_CONFIG(vmm_config, vmm_size); \
    } \
    SS_CONST(C, 0, (mat_width) + pad); \
    for (int _i_(0); _i_ < (0) + (mat_height); ++_i_) { \
      SS_DMA_READ((mat) + _i_ * (mat_stride), 8, 8, (mat_width), A); \
      SS_CONST(A, 0, pad); \
      SS_CONST(B, *((uint64_t*)(vec) + _i_), ((mat_width) + pad) / 4); \
      if (_i_ != (mat_height) - 1) { \
        SS_RECURRENCE(O, C, (mat_width) + pad); \
      } else { \
        SS_DMA_WRITE(O, 8, 8, (mat_width), (res)); \
        SS_GARBAGE(O, pad); \
      } \
    } \
    SS_WAIT_ALL(); \
  } while (false)


#define REVELvec_mul_mat(mat, mat_height, mat_width, mat_stride, vec, is_conj, res) \
  do { \
    int pad(get_pad(mat_width, 4)); \
    int A(is_conj ? P_vcmm_A : P_vmm_A); \
    int B(is_conj ? P_vcmm_B : P_vmm_B); \
    int C(is_conj ? P_vcmm_C : P_vmm_C); \
    int O(is_conj ? P_vcmm_O : P_vmm_O); \
    if (is_conj) { \
      SS_CONFIG(vcmm_config, vcmm_size); \
    } else { \
      SS_CONFIG(vmm_config, vmm_size); \
    } \
    SS_FILL_MODE(STRIDE_ZERO_FILL); \
    SS_DMA_READ(mat, 8 * (mat_stride), 8 * (mat_width), (mat_height), A); \
    SS_CONST(C, 0, (mat_width) + pad); \
    SS_RECURRENCE(O, C, ((mat_width) + pad) * ((mat_height) - 1)); \
    SS_REPEAT_PORT(((mat_width) + pad) / 4); \
    SS_DMA_READ((vec), 8, 8, (mat_height), B); \
    SS_DMA_WRITE(O, 8, 8, (mat_width), (res)); \
    SS_GARBAGE(O, pad); \
    SS_WAIT_ALL(); \
  } while (false)

#define CPUsub_outerx2(m, m_height, m_width, m_stride, a, b, conj_b, res, res_stride) \
  do { \
    complex<float> *_m = (complex<float> *) m; \
    for (int _i_(0); _i_ < (m_height); ++_i_) { \
      for (int _j_(0); _j_ < (m_width); ++_j_) { \
        complex<float> _a_ = a[_i_]; \
        complex<float> _b_ = b[_j_]; \
        if (conj_b) _b_ = std::conj(_b_); \
        complex<float> _m_ = _m == NULL ? (_i_ == _j_ ? 1.0f : 0.0f) : _m[_i_ * (m_stride) + _j_]; \
        (res)[_i_ * (res_stride) + _j_] = _m_ - _a_ * _b_ * 2.0f; \
      } \
    } \
  } while (false)

#define REVELsub_outerx2(m, m_height, m_width, m_stride, a, b, conj_b, res, res_stride) \
  do { \
      int A, B, C, O; \
      complex<float> *_m = (complex<float> *) m; \
      if (!(conj_b)) { \
        SS_CONFIG(subouterx2_config, subouterx2_size); \
        A      = P_subouterx2_A; \
        B      = P_subouterx2_B; \
        C      = P_subouterx2_C; \
        O      = P_subouterx2_O; \
      } else if (conj_b) { \
        SS_CONFIG(suboutercx2_config, suboutercx2_size); \
        A      = P_suboutercx2_A; \
        B      = P_suboutercx2_B; \
        C      = P_suboutercx2_C; \
        O      = P_suboutercx2_O; \
      } \
      int pad = get_pad((m_width), 2); \
      if (_m != NULL) { \
        SS_FILL_MODE(STRIDE_DISCARD_FILL); \
        SS_DMA_READ(m, 8 * (m_stride), 8 * (m_width), (m_height), A); \
      } else { \
        SS_FILL_MODE(NO_FILL); \
        SS_2D_CONST(A, 1065353216, 1, 0, m_width, (m_height) - 1); \
        SS_CONST(A, 1065353216, 1); \
        SS_FILL_MODE(STRIDE_DISCARD_FILL); \
      } \
      SS_REPEAT_PORT(((m_width) + pad) / 2); \
      SS_DMA_READ(a, 8, 8, (m_height), B); \
      SS_DMA_READ(b, 0, 8 * (m_width), m_height, C); \
      SS_DMA_WRITE(O, 8 * (res_stride), 8 * (m_width), m_height, res); \
      SS_WAIT_ALL(); \
  } while (false)

#endif
