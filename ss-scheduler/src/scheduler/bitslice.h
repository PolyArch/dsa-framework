#ifndef __BITSLICE_H__
#define __BITSLICE_H__

#include <utility>
#include <vector>
#include <iostream>

template <class T> 
class bitslices {
public:
  void write(unsigned slice, unsigned p1, unsigned p2, T val, bool check0=true) {
//    std::cout << slice << " " << p1 << " " << p2 << "\n"; 
    check_size(slice);

    T shift_val = val << p1;
    T mask = gen_mask(p1,p2);

    assert(((shift_val & mask) == shift_val) && "too big of val");
    
    if(check0) {
      assert(((_slices[slice]&mask)==0) && "overwrote bits unintenionally?");
    } else {
     _slices[slice]^=(_slices[slice]&mask);
    }

    _slices[slice] |= shift_val;
  }

  void write(unsigned slice, T val) {
      check_size(slice); 
      _slices[slice]=val;
  }


  void clear_slice(unsigned slice) {
    check_size(slice);

    _slices[slice]=0;
  }

  T read_slice(unsigned slice, unsigned p1, unsigned p2) {
    check_size(slice);

     T mask = gen_mask(p1,p2);
     T masked_val = _slices[slice] & mask;
     return masked_val >> p1;
  }

  T read_slice(unsigned slice) {
    check_size(slice);

    return _slices[slice];
  }

  unsigned size() {return _slices.size();}

private:
  std::vector<T> _slices;

  T gen_mask(unsigned p1, unsigned p2) {
    assert(p1 >= 0);
    assert( (p1 <= p2 ) && "first bit pos less than second");
    assert( (p2 < sizeof(T)*8) && "don't go past end of slice");
    
    T mask=0;
    //I realize this isn't the fastest method, I also don't care. : )
    for(unsigned i = p1; i <= p2; ++i) {
      mask |= ((T)1) << i;
    }
    return mask;
  }

  void check_size(unsigned slice) {
    if(slice >= _slices.size()) {
      _slices.resize(slice+1,0);
    }
  }

};


#endif
