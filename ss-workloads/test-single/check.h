#include <iostream>

template <class T>
void compare(char* test, T* array1, T* array2, int size) {
  bool error=false;
  for(int i = 0; i < size; ++i) {
    if(array1[i] != array2[i]) {
      error = true; 
      break;
    }
  }
  if(error) {
    std::cerr << "ERROR ON: " << test << ", Printing Outputs\n";
    for(int i = 0; i < size; ++i) {
      std::cout << std::dec << i << std::hex << " " << array1[i] << ":" << array2[i];;
      if(array1[i] != array2[i]) {
        std::cerr << " \t\tERROR";
      }
      std::cerr << "\n";
    }
    exit(1);
  } 
}

