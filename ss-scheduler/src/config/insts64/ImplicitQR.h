//Dummy instruction for Implicit QR factorization on a bidiagonal matrix

#define FINALIZE   1
#define INITIALIZE 2
#define FURTHER    3
#define ITERATION  4
#define GRABSIN    5
#define GRABCOS    6

//ops[0]: D
//ops[1]: F
//ops[2]: Inst

uint64_t inst = ops[2];

//bool same_as_last = reg[5] == inst;
uint64_t last_inst = reg[5];

reg[5] = inst;

if (inst == FINALIZE) {
  if (last_inst != FINALIZE && last_inst != ITERATION && last_inst != INITIALIZE) {
    std::cerr << "LAST INSTRUCTION" << last_inst << "\n";
    assert(false && "NOT VALID STATE FOR FINALIZE");
  }
  if (last_inst != FINALIZE) {
    //std::cout << "[FIN] f[i]" << as_float_complex(reg[2]) << "\n";
    return reg[2];
  } else {
    //std::cout << "[FIN] d[i+1]" << as_float_complex(reg[3]) << "\n\n";
    return reg[3];
  }
}

auto givens = [] (complex<float> a, complex<float> b) -> float {
  return sqrt((std::conj(a) * a).real() + (std::conj(b) * b).real());
};

if (inst == INITIALIZE) {
  if (last_inst == FINALIZE || last_inst == 0) {
    reg[0] = ops[0];
    reg[1] = ops[1];
    reg[4] = 1;
    discard = 1;
    //std::cout << "d[0]: " << as_float_complex(reg[0]) << "\n";
    //std::cout << "f[0]: " << as_float_complex(reg[1]) << "\n\n";
  } else if (last_inst == INITIALIZE) {
    ++reg[4];
    complex<float> d0 = as_float_complex(reg[0]);
    complex<float> f0 = as_float_complex(reg[1]);
    complex<float> d1 = as_float_complex(ops[0]);
    complex<float> dn1= as_float_complex(ops[1]);

    //if (reg[4] == 2) {
    //  std::cout << "d[1]: " << d1 << "\n";
    //  std::cout << "d[n-1]: " << dn1 << "\n";
    //}

    float mu = (dn1 * std::conj(dn1)).real();
    complex<float> a = std::conj(d0) * d0 - mu;
    complex<float> b = std::conj(f0) * d0;
    float r  = givens(a, b);
    complex<float> c = std::conj(a) / r;
    if (reg[4] == 2) {
      //std::cout << "[INIT] cos:" << c << "\n";
      return as_uint64(c);
    }
    complex<float> s = std::conj(b) / r;
    if (reg[4] == 3) {
      //std::cout << "[INIT] sin:" << s << "\n";
      return as_uint64(s);
    }
    a = d0 * c + f0 * std::conj(s);
    f0 = d0 * s - f0 * std::conj(c);
    b = d1 * std::conj(s);
    d1 *= -std::conj(c);
    r = givens(a, b);
    c = std::conj(a) / r;
    s = std::conj(b) / r;
    reg[0] = as_uint64(c);
    reg[1] = as_uint64(s);
    reg[2] = as_uint64(f0 * c + d1 * s);
    reg[3] = as_uint64(f0 * std::conj(s) - d1 * std::conj(c));
    //std::cout << "cos:" << c << "\n";
    //std::cout << "sin:" << s << "\n";
    //std::cout << "f[0]:" << as_float_complex(reg[2]) << "\n";
    //std::cout << "d[1]:" << as_float_complex(reg[3]) << "\n";
    //std::cout << "[INIT] [FIN] d[0]:" << r << "\n\n";
    return as_uint32(r);
  } else {
    std::cerr << "LAST INSTRUCTION" << last_inst << "\n";
    assert(false && "NOT VALID STATE FOR INITALIZATION");
  }
}

if (inst == FURTHER) {
  if (last_inst != INITIALIZE && last_inst != ITERATION) {
    std::cerr << "LAST INSTRUCTION" << last_inst << "\n";
    assert(false && "NOT VALID STATE FOR PREPARING FOR NEXT ITERATION");
  }
  complex<float> c = as_float_complex(reg[0]);
  complex<float> s = as_float_complex(reg[1]);
  complex<float> fi1 = as_float_complex(ops[1]);
  reg[0] = reg[2];
  reg[1] = as_uint64(fi1 * s);
  reg[2] = reg[3];
  reg[3] = as_uint64(-fi1 * std::conj(c));
  //std::cout << "f[i]:" << as_float_complex(reg[0]) << "\n"
  //  << "extra:" << as_float_complex(reg[1]) << "\n"
  //  << "d[i+1]:" << as_float_complex(reg[2]) << "\n"
  //  << "f[i+1]:" << as_float_complex(reg[3]) << "\n\n";
  discard = 1;
}

if (inst == ITERATION) {
  if (last_inst != FURTHER) {
    if (last_inst != ITERATION) {
      std::cerr << "LAST INSTRUCTION" << last_inst << "\n";
      assert(false && "NOT PREPARED FOR ITERATION");
    } else {
      ++reg[4];
    }
  } else {
    reg[4] = 1;
  }
  complex<float> a = as_float_complex(reg[0]);
  complex<float> b = as_float_complex(reg[1]);
  float r = givens(a, b);
  if (last_inst == FURTHER) {
    //Finalize f[i-1]
    //std::cout << "[FIN] f[i-1]" << r << "\n";
    assert(reg[4] == 1);
    return as_uint32(r);
  }
  complex<float> c  = std::conj(a) / r;
  if (reg[4] == 2) {
    return as_uint64(c);
  }
  complex<float> s  = std::conj(b) / r;
  if (reg[4] == 3) {
    return as_uint64(s);
  }
  complex<float> di = as_float_complex(reg[2]);
  complex<float> fi = as_float_complex(reg[3]);
  complex<float> di1 = as_float_complex(ops[0]);
  a   = di * c + fi * s;
  fi  = di * std::conj(s) - fi * std::conj(c);
  b   = di1 * s;
  di1*= -std::conj(c);
  r = givens(a, b);
  if (reg[4] == 4) {
    //std::cout << "[FIN] d[i]:" << r << "\n\n";
    return as_uint32(r);
  }
  c = std::conj(a) / r;
  s = std::conj(b) / r;
  reg[0] = as_uint64(c);
  reg[1] = as_uint64(s);
  reg[2] = as_uint64(c * fi + s * di1);
  reg[3] = as_uint64(std::conj(s) * fi - std::conj(c) * di1);

  //std::cout << "cos:" << c << "\n";
  //std::cout << "sin:" << s << "\n";
  //std::cout << "f[i]:" << as_float_complex(reg[2]) << "\n";
  //std::cout << "d[i+1]:" << as_float_complex(reg[3]) << "\n\n";

  discard = 1;
}

//magic number!
return -1;

#undef FINALIZE
#undef FURTHER
#undef ITERATION
#undef INITIALIZE
