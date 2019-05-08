#include "svd.h"
#include "matvec.h"
#include "ss_insts.h"
#include "aplygvs.dfg.h"
#include "fused.dfg.h"
#include "hhr.dfg.h"
#include "finalize.dfg.h"
#include "sim_timing.h"

#define FINALIZE   1
#define INITIALIZE 2
#define FURTHER    3
#define ITERATION  4
#define GRABSIN    5
#define GRABCOS    6

complex<float> f[_N_], d[_N_], _s[_N_];
complex<float> hh[_N_], vec[_N_], r[_N_ * _N_], calc[_N_];
complex<float> _one(1, 0), _zero(0, 0);

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
  //for (int i = 0; i < n; ++i) std::cout << d[i] << " "; std::cout << std::endl;
  //for (int i = 0; i < n - 1; ++i) std::cout << f[i] << " "; std::cout << std::endl;

  int N = _N_;

  float mu = complex_norm(d[n - 1]);
  complex<float> a(complex_norm(d[0]) - mu), b(complex_conj_mul(f[0], d[0]));
  SS_CONST(P_aplygvs_mat0, *((uint64_t*) &a), 1);
  SS_CONST(P_aplygvs_mat1, *((uint64_t*) &b), 1);
  SS_CONST(P_aplygvs_mat2, *((uint64_t*) (d + 0)), 1);
  SS_CONST(P_aplygvs_mat3, *((uint64_t*) (f + 0)), 1);
  SS_DMA_READ(f + 1, 8, 8, n - 2, P_aplygvs_F);
  SS_CONST(P_aplygvs_F, 0, 1);
  SS_DMA_READ(d + 1, 8, 8, n - 1, P_aplygvs_D);
  SS_GARBAGE(P_aplygvs_F_, 1);
  SS_DMA_WRITE(P_aplygvs_F_, 8, 8, n - 2, f);
  SS_DMA_WRITE(P_aplygvs_D_, 8, 8, n - 1, d);

  for (int i = 1; i < n - 1; ++i) {
    //complex<float> buffer[4];
    //SS_RECV(P_aplygvs_mat_, buffer[0]);
    //SS_RECV(P_aplygvs_mat_, buffer[1]);
    //SS_RECV(P_aplygvs_mat_, buffer[2]);
    //SS_RECV(P_aplygvs_mat_, buffer[3]);
    //std::cout << buffer[0] << ", " << buffer[1] << ", " << buffer[2] << ", " << buffer[3] << "\n";
    //SS_CONST(P_aplygvs_mat, *((uint64_t*) (buffer + 0)), 1);
    //SS_CONST(P_aplygvs_mat, *((uint64_t*) (buffer + 1)), 1);
    //SS_CONST(P_aplygvs_mat, *((uint64_t*) (buffer + 2)), 1);
    //SS_CONST(P_aplygvs_mat, *((uint64_t*) (buffer + 3)), 1);

    SS_RECURRENCE(P_aplygvs_mat_0, P_aplygvs_mat0, 1);
    SS_RECURRENCE(P_aplygvs_mat_1, P_aplygvs_mat1, 1);
    SS_RECURRENCE(P_aplygvs_mat_2, P_aplygvs_mat2, 1);
    SS_RECURRENCE(P_aplygvs_mat_3, P_aplygvs_mat3, 1);

    SS_REPEAT_PORT(N);
    SS_RECURRENCE(P_aplygvs_rot0, P_aplygvs_C, 1);
    SS_REPEAT_PORT(N);
    SS_RECURRENCE(P_aplygvs_rot1, P_aplygvs_S, 1);
    SS_DMA_READ(v + i * N, 8, 8, N, P_aplygvs_A);
    SS_DMA_READ(v + (i + 1) * N, 8, 8, N, P_aplygvs_B);
    SS_DMA_WRITE(P_aplygvs_O0, 8, 8, N, v + i * N);
    SS_DMA_WRITE(P_aplygvs_O1, 8, 8, N, v + (i + 1) * N);
    

    //complex<float> c, s;
    //SS_RECV(P_aplygvs_rot, c);
    //SS_RECV(P_aplygvs_rot, s);
    //std::cout << c << ", " << s << std::endl;
  }

  //SS_GARBAGE(P_aplygvs_D_, 1);
  //SS_GARBAGE(P_aplygvs_F_, 1);
  SS_GARBAGE(P_aplygvs_rot0, 1);
  SS_GARBAGE(P_aplygvs_rot1, 1);
  SS_DMA_WRITE(P_aplygvs_mat_0, 8, 8, 1, f + n - 2);
  SS_GARBAGE(P_aplygvs_mat_1, 1);
  SS_DMA_WRITE(P_aplygvs_mat_2, 8, 8, 1, d + n - 1);
  SS_GARBAGE(P_aplygvs_mat_3, 1);

  SS_WAIT_ALL();

  //for (int i = 0; i < n; ++i) std::cout << d[i] << " "; std::cout << std::endl;
  //for (int i = 0; i < n - 1; ++i) std::cout << f[i] << " "; std::cout << std::endl;

}

//The most horrible kernel so far
void bidiagonalize(complex<float> *a, complex<float> *f, complex<float> *d, complex<float> *q) {
  int N = _N_;
  const int w_spad = 8;
  const int r_trans_spad = N * 8;
  const int horizon_vec  = 8192;
  const int q_spad = N * 8;

  SS_CONTEXT(2);
  SS_CONFIG(fused_config, fused_size);
  //SS_DMA_SCRATCH_LOAD(&_one, 8, 8, 1, 0);
  //SS_CONST_SCR(0, *((uint64_t*)&_one), 1);

  SS_CONTEXT(1);
  SS_CONFIG(hhr_config, hhr_size);
  //SS_CONST_SCR(0, *((uint64_t*)&_one), 1);
  //SS_DMA_SCRATCH_LOAD(&_one, 8, 8, 1, 0);
  SS_DMA_READ(a + N, 8 * N, 8, N - 1, P_hhr_A);
  SS_DMA_READ(a + N, 8 * N, 8, N - 1, P_hhr_B);
  SS_CONST(P_hhr_Coef, 1065353216, N - 1);
  SS_CONST(P_hhr_reset, 2, N - 2);
  SS_CONST(P_hhr_reset, 1, 1);
  SS_REPEAT_PORT(3);
  SS_RECURRENCE(P_hhr_O, P_hhr_NORM, 1);
  SS_REPEAT_PORT(3);
  SS_DMA_READ(a, 8, 8, 1, P_hhr_HEAD);
  SS_CONST(P_hhr_Inst, 0, 1);
  SS_CONST(P_hhr_Inst, 1, 1);
  SS_CONST(P_hhr_Inst, 2, 1);

  SS_DMA_WRITE(P_hhr_RES, 8, 8, 1, d); //alpha
  SS_REPEAT_PORT(N - 1);
  SS_RECURRENCE(P_hhr_RES, P_hhr_B, 1); //normalize
  SS_CONST(P_hhr_Coef, 1065353216, N - 1);
  SS_REPEAT_PORT(N * (N - 1));
  SS_RECURRENCE(P_hhr_RES, P_hhr_Coef, 1); //tau
  SS_DMA_READ(a + N, 8 * N, 8, N - 1, P_hhr_A);
  SS_CONST(P_hhr_reset, 1, N - 1);

  SS_SCR_WRITE(P_hhr_O, 8 * (N - 1), w_spad);
  SS_WAIT_SCR_WR();

  SS_SCR_PORT_STREAM(w_spad - 8, 0, 8 * N, N - 1, P_hhr_B);
  SS_2D_CONST(P_hhr_reset, 2, N - 1, 1, 1, N - 1);
  //SS_DMA_WRITE(P_hhr_O, 8, 8, N - 1, buffer + N); test pass!
  //SS_GARBAGE(P_hhr_O, N - 1); test pass!
  for (int i = 1; i < N; ++i) {
    SS_DMA_READ(a + i, 8 * N, 8, N, P_hhr_A);
  }

  SS_REPEAT_PORT(N);
  SS_RECURRENCE(P_hhr_O, P_hhr_RA, N - 1);
  SS_SCR_PORT_STREAM(w_spad - 8, 0, 8 * N, N - 1, P_hhr_RB);
  SS_2D_CONST(P_hhr_RSignal, 1, 1, 0, N - 1, N - 1);
  //SS_DMA_WRITE(P_hhr_R_MEM, 8, 8, N - 1, a + 1);
  SS_SCR_WRITE(P_hhr_R_MEM, 8 * (N - 1), horizon_vec);
  SS_SCR_WRITE(P_hhr_R_SPAD, 8 * (N - 1) * (N - 1), r_trans_spad);
  for (int i = 1; i < N; ++i) {
    SS_DMA_READ(a + i, 8 * N, 8, N, P_hhr_RC);
  }

  SS_WAIT_SCR_WR();

  SS_SCRATCH_READ(horizon_vec + 8, 8 * (N - 2), P_hhr_A);
  SS_SCRATCH_READ(horizon_vec + 8, 8 * (N - 2), P_hhr_B);
  SS_CONST(P_hhr_Coef, 1065353216, N - 2);
  SS_CONST(P_hhr_reset, 2, N - 3);
  SS_CONST(P_hhr_reset, 1, 1);
  SS_REPEAT_PORT(3);
  SS_RECURRENCE(P_hhr_O, P_hhr_NORM, 1);
  SS_REPEAT_PORT(3);
  SS_SCRATCH_READ(horizon_vec, 8, P_hhr_HEAD);
  SS_CONST(P_hhr_Inst, 0, 1);
  SS_CONST(P_hhr_Inst, 1, 1);
  SS_CONST(P_hhr_Inst, 2, 1);
  SS_DMA_WRITE(P_hhr_RES, 8, 8, 1, f);
  SS_REPEAT_PORT(N - 2);
  SS_RECURRENCE(P_hhr_RES, P_hhr_B, 1);
  SS_SCRATCH_READ(horizon_vec + 8, 8 * (N - 2), P_hhr_A);
  SS_CONST(P_hhr_Coef, 1065353216, N - 2);
  SS_CONST(P_hhr_reset, 1, N - 2);
  SS_RECURRENCE(P_hhr_O, P_hhr_IN, N - 2);
  SS_RECURRENCE(P_hhr_RES, P_hhr_IN, 1);
  SS_SCR_WRITE(P_hhr_OUTlocal, 8 * (N - 2), w_spad);
  //SS_DMA_WRITE(P_hhr_OUTremote, 0, 8 * (N - 2), 1, buffer); //DEBUG PASS!
  //SS_GARBAGE(P_hhr_OUTremote, N - 2); //TODO: xfer it to Q kernel later
  SS_XFER_RIGHT(P_hhr_OUTremote, P_fused_IN, N - 2);
  SS_CONTEXT(2);
  SS_SCR_WRITE(P_fused_OUT, 8 * (N - 2), w_spad);
  //SS_DMA_SCRATCH_LOAD(&_one, 0, 8, 1, 0);
  //SS_CONST_SCR(0, *((uint64_t*)&_one), 1);
  SS_CONTEXT(1);
  SS_REPEAT_PORT((N - 1) * (N - 1));
  SS_RECURRENCE(P_hhr_OUTlocal, P_hhr_Coef_, 1);
  SS_REPEAT_PORT(N - 1);
  SS_XFER_RIGHT(P_hhr_OUTremote, P_fused_QTAU, 1);
  //SS_GARBAGE(P_hhr_OUTremote, 1); //TODO: xfer it to Q kernel later
  SS_WAIT_SCR_WR();
  SS_REPEAT_PORT(N - 1);
  SS_SCRATCH_READ(0, 8 * (N - 1), P_hhr_B_);
  SS_SCRATCH_READ(r_trans_spad, 8 * (N - 1) * (N - 1), P_hhr_A_);
  SS_CONST(P_hhr_ACC_, 0, N - 1);
  SS_RECURRENCE(P_hhr_O_, P_hhr_ACC_, (N - 1) * (N - 2));
  SS_SCR_WRITE(P_hhr_O_, 8 * (N - 1), horizon_vec);
  SS_WAIT_SCR_WR();
  //DEBUG INFO
  //SS_SCRATCH_DMA_STORE(0, 0, 8 * (N - 1), 1, hh);
  //SS_SCRATCH_DMA_STORE(horizon_vec, 0, 8 * (N - 1), 1, vec);
  //SS_SCRATCH_DMA_STORE(r_trans_spad, 0, 8 * (N - 1) * (N - 1), 1, r);
  SS_SCRATCH_READ(r_trans_spad, 8 * (N - 1) * (N - 1), P_hhr_RC);
  SS_SCR_PORT_STREAM(horizon_vec, 0, 8 * (N - 1), (N - 1), P_hhr_RB);
  SS_REPEAT_PORT(N - 1);
  SS_SCRATCH_READ(0, 8 * (N - 1), P_hhr_RA);
  SS_CONST(P_hhr_RSignal, 0, (N - 1) * (N - 1));
  SS_SCR_WRITE(P_hhr_R_SPAD, 8 * (N - 1) * (N - 1), r_trans_spad);
  //SS_WAIT_SCR_WR();
  //DEBUG INFO
  //SS_SCRATCH_DMA_STORE(r_trans_spad, 0, 8 * (N - 1) * (N - 1), 1, r);

  SS_CONTEXT(2);
  SS_WAIT_SCR_WR();
  SS_SCR_PORT_STREAM(0, 8, 8, N - 1, P_fused_QW);
  SS_CONST(P_fused_QM, 1065353216, N - 1);
  SS_CONST(P_fused_Qreset, 1, N - 1);
  SS_REPEAT_PORT(N - 1);
  SS_RECURRENCE(P_fused_QV, P_fused_QA, N - 1);
  //SS_SCR_PORT_STREAM(0, 8, 8, N, P_fused_QA);
  SS_SCR_PORT_STREAM(0, 0, 8 * (N - 1), N - 1, P_fused_QB);
  SS_2D_CONST(P_fused_QC, 1065353216, 1, 0, N - 1, N - 2);
  SS_CONST(P_fused_QC, 1065353216, 1);
  SS_2D_CONST(P_fused_QSignal, 1, 1, 0, N - 2, N - 1);
  SS_DMA_WRITE(P_fused_Q_MEM, 0, 8 * (N - 1), 1, q + N + 1);
  SS_SCR_WRITE(P_fused_Q_SPAD, 8 * (N - 1) * (N - 2), q_spad);

  for (int i = 1; i < N - 1; ++i) {
    int n = N - i;
    SS_CONTEXT(1);
    SS_WAIT_SCR_WR();

    SS_SCRATCH_READ(r_trans_spad + 8, 8 * (n - 1), P_hhr_A);
    SS_SCRATCH_READ(r_trans_spad + 8, 8 * (n - 1), P_hhr_B);
    SS_CONST(P_hhr_Coef, 1065353216, n - 1);
    SS_CONST(P_hhr_reset, 2, n - 2);
    SS_CONST(P_hhr_reset, 1, 1);

    SS_REPEAT_PORT(3);
    SS_RECURRENCE(P_hhr_O, P_hhr_NORM, 1);
    SS_REPEAT_PORT(3);
    SS_SCRATCH_READ(r_trans_spad, 8, P_hhr_HEAD);
    SS_CONST(P_hhr_Inst, 0, 1);
    SS_CONST(P_hhr_Inst, 1, 1);
    SS_CONST(P_hhr_Inst, 2, 1);
    
    SS_SCRATCH_READ(r_trans_spad + 8, 8 * (n - 1), P_hhr_A);
    SS_DMA_WRITE(P_hhr_RES, 8, 8, 1, d + i); //alpha
    SS_REPEAT_PORT(n - 1);
    SS_RECURRENCE(P_hhr_RES, P_hhr_B, 1); //normalize
    SS_CONST(P_hhr_Coef, 1065353216, n - 1);
    SS_REPEAT_PORT(n * (n - 1));
    SS_RECURRENCE(P_hhr_RES, P_hhr_Coef, 1); //tau
    SS_CONST(P_hhr_reset, 1, n - 1);
    
    SS_SCR_WRITE(P_hhr_O, 8 * (n - 1), w_spad);
    SS_WAIT_SCR_WR();

    SS_SCR_PORT_STREAM(w_spad - 8, 0, 8 * n, n - 1, P_hhr_B);
    SS_2D_CONST(P_hhr_reset, 2, n - 1, 1, 1, n - 1);
    SS_SCRATCH_READ(r_trans_spad + 8 * n, 8 * (n - 1) * n, P_hhr_A);

    SS_REPEAT_PORT(n);
    SS_RECURRENCE(P_hhr_O, P_hhr_RA, n - 1);
    SS_SCR_PORT_STREAM(w_spad - 8, 0, 8 * n, n - 1, P_hhr_RB);
    SS_2D_CONST(P_hhr_RSignal, 1, 1, 0, n - 1, n - 1);
    SS_SCRATCH_READ(r_trans_spad + 8 * n, 8 * (n - 1) * n, P_hhr_RC);
    if (n - 1 != 1) {
      SS_SCR_WRITE(P_hhr_R_MEM, 8 * (n - 1), horizon_vec);
      SS_SCR_WRITE(P_hhr_R_SPAD, 8 * (n - 1) * (n - 1), r_trans_spad);
    } else {
      SS_DMA_WRITE(P_hhr_R_MEM, 0, 8, 1, f + i);
      SS_DMA_WRITE(P_hhr_R_SPAD, 0, 8, 1, d + i + 1);
    }

    SS_WAIT_SCR_WR();

    //SS_SCRATCH_DMA_STORE(0, 0, 8 * n, 1, hh);
    //SS_WAIT_ALL();
    //for (int j = 0; j < n; ++j) std::cout << hh[j] << " "; std::cout << "\n";

    if (n > 2) {
      SS_SCRATCH_READ(horizon_vec + 8, 8 * (n - 2), P_hhr_A);
      SS_SCRATCH_READ(horizon_vec + 8, 8 * (n - 2), P_hhr_B);
      SS_CONST(P_hhr_Coef, 1065353216, n - 2);
      SS_CONST(P_hhr_reset, 2, n - 3);
      SS_CONST(P_hhr_reset, 1, 1);
      SS_REPEAT_PORT(3);
      SS_RECURRENCE(P_hhr_O, P_hhr_NORM, 1);
      SS_REPEAT_PORT(3);
      SS_SCRATCH_READ(horizon_vec, 8, P_hhr_HEAD);
      SS_CONST(P_hhr_Inst, 0, 1);
      SS_CONST(P_hhr_Inst, 1, 1);
      SS_CONST(P_hhr_Inst, 2, 1);
      SS_DMA_WRITE(P_hhr_RES, 8, 8, 1, f + i);
      SS_REPEAT_PORT(n - 2);
      SS_RECURRENCE(P_hhr_RES, P_hhr_B, 1);
      SS_SCRATCH_READ(horizon_vec + 8, 8 * (n - 2), P_hhr_A);
      SS_CONST(P_hhr_Coef, 1065353216, n - 2);
      SS_CONST(P_hhr_reset, 1, n - 2);
      SS_RECURRENCE(P_hhr_O, P_hhr_IN, n - 2);
      SS_RECURRENCE(P_hhr_RES, P_hhr_IN, 1);
      SS_SCR_WRITE(P_hhr_OUTlocal, 8 * (n - 2), w_spad);
      //SS_DMA_WRITE(P_hhr_OUTremote, 0, 8 * (n - 2), 1, hh); //DEBUG PASS!
      //SS_GARBAGE(P_hhr_OUTremote, n - 2); //TODO: xfer it to Q kernel later
      SS_XFER_RIGHT(P_hhr_OUTremote, P_fused_IN, n - 2);
      SS_CONTEXT(2);
      SS_WAIT_SCR_WR();
      SS_SCR_WRITE(P_fused_OUT, 8 * (n - 2), w_spad);
      SS_CONTEXT(1);
      SS_REPEAT_PORT((n - 1) * (n - 1));
      SS_RECURRENCE(P_hhr_OUTlocal, P_hhr_Coef_, 1);
      SS_REPEAT_PORT((n - 1) * (N - 1));
      SS_XFER_RIGHT(P_hhr_OUTremote, P_fused_QTAU, 1);
      //SS_GARBAGE(P_hhr_OUTremote, 1); //TODO: xfer it to Q kernel later
      SS_WAIT_SCR_WR();
      SS_REPEAT_PORT(n - 1);
      SS_SCRATCH_READ(0, 8 * (n - 1), P_hhr_B_);
      SS_SCRATCH_READ(r_trans_spad, 8 * (n - 1) * (n - 1), P_hhr_A_);
      SS_CONST(P_hhr_ACC_, 0, n - 1);
      SS_RECURRENCE(P_hhr_O_, P_hhr_ACC_, (n - 1) * (n - 2));
      SS_SCR_WRITE(P_hhr_O_, 8 * (n - 1), horizon_vec);
      SS_WAIT_SCR_WR();
      //DEBUG INFO
      //SS_SCRATCH_DMA_STORE(0, 0, 8 * (n - 1), 1, hh);
      //SS_SCRATCH_DMA_STORE(horizon_vec, 0, 8 * (n - 1), 1, vec);
      //SS_SCRATCH_DMA_STORE(r_trans_spad, 0, 8 * (n - 1) * (n - 1), 1, r);
      SS_SCRATCH_READ(r_trans_spad, 8 * (n - 1) * (n - 1), P_hhr_RC);
      SS_SCR_PORT_STREAM(horizon_vec, 0, 8 * (n - 1), (n - 1), P_hhr_RB);
      SS_REPEAT_PORT(n - 1);
      SS_SCRATCH_READ(0, 8 * (n - 1), P_hhr_RA);
      SS_CONST(P_hhr_RSignal, 0, (n - 1) * (n - 1));
      SS_SCR_WRITE(P_hhr_R_SPAD, 8 * (n - 1) * (n - 1), r_trans_spad);
      //DEBUG INFO
      SS_WAIT_SCR_WR();

      /* It fixes the timing */
      //SS_SCRATCH_DMA_STORE(r_trans_spad, 0, 8 * (n - 1) * (n - 1), 1, r);
      SS_WAIT_ALL();
      //for (int j = 0; j < n - 1; ++j) std::cout << hh[j] << " "; std::cout << "\n";
      //std::cout << i << "\n";
      //std::cout << "r:\n";
      //for (int j = 0; j < n - 1; ++j) {
      //  for (int k = 0; k < n - 1; ++k)
      //    std::cout << r[k * (n - 1) + j] << " ";
      //  std::cout << "\n";
      //}
      /* It fixes the timing */

      SS_CONTEXT(2);
      SS_WAIT_SCR_WR();
      SS_SCR_PORT_STREAM(0, 0, 8 * (n - 1), N - 1, P_fused_QW);
      SS_SCRATCH_READ(q_spad, 8 * (N - 1) * (n - 1), P_fused_QM);
      SS_2D_CONST(P_fused_Qreset, 2, n - 2, 1, 1, N - 1);
      SS_REPEAT_PORT(n - 1);
      SS_RECURRENCE(P_fused_QV, P_fused_QA, N - 1);
      SS_SCR_PORT_STREAM(0, 0, 8 * (n - 1), N - 1, P_fused_QB);
      SS_SCRATCH_READ(q_spad, 8 * (N - 1) * (n - 1), P_fused_QC);
      SS_2D_CONST(P_fused_QSignal, 1, 1, 0, n - 2, N - 1);
      SS_DMA_WRITE(P_fused_Q_MEM, 0, 8 * (N - 1), 1, q + (i + 1) * N + 1);
      if (n - 2 != 1) {
        SS_SCR_WRITE(P_fused_Q_SPAD, 8 * (N - 1) * (n - 2), q_spad);
      } else {
        SS_DMA_WRITE(P_fused_Q_SPAD, 0, 8 * (N - 1), 1, q + (i + 2) * N + 1);
      }
    }
  }
  SS_CONTEXT(3);
  SS_WAIT_ALL();
  //for (int i = 0; i < N - 1; ++i) std::cout << f[i] << " "; std::cout << "\n";
  //for (int i = 0; i < N; ++i) std::cout << d[i] << " "; std::cout << "\n";
  //for (int i = 0; i < N; ++i) {
  //  for (int j = 0; j < N; ++j)
  //    std::cout << q[i * N + j] << " ";
  //  std::cout << "\n";
  //}
}

void implicit_iteration(complex<float> *d, complex<float> *f, complex<float> *v) {
  SS_CONFIG(aplygvs_config, aplygvs_size);

  int left = 0, right = _N_ - 1;
  while (left < right) {
    //printf("%d %d\n", left, right);
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

}

void svd(complex<float> *a, complex<float> *u, float *s, complex<float> *v) {
  bidiagonalize(a, f, d, v);

  int N = _N_;
  SS_CONTEXT(1);

  implicit_iteration(d, f, v);

  //SS_CONFIG(aplygvs_config, aplygvs_size);
  //implicit_kernel(d, f, v, N);
  //return;

  //for (int i = 0; i < N - 1; ++i)
  //  std::cout << f[i] << "\n";
  //for (int i = 0; i < N; ++i)
  //  std::cout << d[i] << "\n";
  /*for (int i = 0; i < N; ++i) {
    for (int j = 0; j < N; ++j)
      std::cout << v[i * N + j] << " ";
    std::cout << "\n";
  }*/
 
  d[N - 1] = sqrt(complex_norm(d[N - 1]));
  SS_CONFIG(finalize_config, finalize_size);
  SS_DMA_READ(d, 0, 8 * _N_, 1, P_finalize_D);
  SS_SCR_WRITE(P_finalize_SINV, 8 * _N_, 0);
  SS_WAIT_SCR_WR();

  SS_DMA_READ(v, 0, 8 * _N_ * _N_, _N_, P_finalize_A);
  SS_2D_CONST(P_finalize_reset, 2, _N_ / 4 - 1, 1, 1, _N_ * _N_);
  for (int i = 0; i < _N_; ++i) {
    SS_DMA_READ(a + i * _N_, 0, 8 * _N_, _N_, P_finalize_B);
    s[i] = d[i].real();
  }
  //SS_GARBAGE(P_finalize_O, _N_ * _N_);
  SS_RECURRENCE(P_finalize_O, P_finalize_U, _N_ * _N_);
  SS_REPEAT_PORT(_N_);
  SS_SCRATCH_READ(0, 8 * _N_, P_finalize_INV);
  SS_DMA_WRITE(P_finalize_RES, 0, 8 * _N_ * _N_, 1, u);
  SS_WAIT_ALL();

  //for (int i = 0; i < _N_; ++i) {
  //  s[i] = sqrt(complex_norm(d[i]));
  //}
  //for (int i = 0; i < _N_; ++i) {
  //  for (int j = 0; j < _N_; ++j) {
  //    complex<float> sum(0, 0);
  //    for (int k = 0; k < _N_; ++k)
  //      sum += complex<float>(complex_conj_mul(v[j * _N_ + k], a[i * _N_ + k]));
  //    u[i * _N_ + j] = sum;
  //  }
  //}
  //for (int i = 0; i < _N_; ++i) {
  //  for (int j = 0; j < _N_; ++j) {
  //    u[i * _N_ + j] /= s[j];
  //  }
  //}
}
