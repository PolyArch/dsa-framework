#ifndef __SS_LOC_H__
#define __SS_LOC_H__

//https://softwareengineering.stackexchange.com/questions/194412/using-scoped-enums-for-bit-flags-in-c

#define ENUM_FLAG_OPERATOR(T,X) inline T operator X (T lhs, T rhs) { return (T) (static_cast<std::underlying_type<T>::type>(lhs) X static_cast<std::underlying_type<T>::type>(rhs)); } 
#define ENUM_FLAGS(T) \
  enum class T; \
inline T operator ~ (T t) { return (T) (~static_cast<std::underlying_type<T>::type>(t)); } \
ENUM_FLAG_OPERATOR(T,|) \
ENUM_FLAG_OPERATOR(T,^) \
ENUM_FLAG_OPERATOR(T,&) \
enum class T

ENUM_FLAGS(LOC) {NONE=0x000, 
                      DMA=0x001, 
                      SCR=0x002, 
                     PORT=0x004, 
                    CONST=0x008, 
               REMOTE_SCR=0x010, 
              REMOTE_PORT=0x020, 
                  REC_BUS=0x040,
                  NETWORK=0x080,
                    TOTAL=0x07F}; 

//using T = std::underlying_type<LOC>::type;
//inline LOC operator | (LOC lhs, LOC rhs) {
//  return (LOC)(static_cast<T>(lhs) | static_cast<T>(rhs));}
//
//inline LOC& operator |= (LOC& lhs, LOC rhs) {
//  lhs = (LOC)(static_cast<T>(lhs) | static_cast<T>(rhs));
//  return lhs;
//}
//
//using T = std::underlying_type<LOC>::type;
//inline LOC operator & (LOC lhs, LOC rhs) {
//  return (LOC)(static_cast<T>(lhs) & static_cast<T>(rhs));}
//
//inline LOC& operator &= (LOC& lhs, LOC rhs) {
//  lhs = (LOC)(static_cast<T>(lhs) & static_cast<T>(rhs));
//  return lhs;
//}
//
//
//

//inline T operator ~ (T t) { return (T) (~static_cast<std::underlying_type_t<T>::type>(t)); } 


#endif
