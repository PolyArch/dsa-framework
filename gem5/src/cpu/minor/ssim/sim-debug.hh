#ifndef __SIM_SS_DEBUG_H__
#define __SIM_SS_DEBUG_H__
#include <string>
#include <cstring>
#include <iostream>

//Header-specific debugs
class SS_DEBUG {
public:
  #undef DO_DBG
  #define DO_DBG(x) static bool x;
  #include "dbg.h"
  static std::string verif_name;
  static bool pred;

  #undef DO_DBG
  #define DO_DBG(x) if(getenv(#x)) \
  {x=std::string(getenv(#x))!="0" && pred; \
    if(x && strcmp(#x,"SUPRESS_SS_STATS")!=0) \
    {std::cout << "SS_DEBUG VAR: " << #x << " IS ON\n";}}

 static void check_env(bool flag) {
   pred = flag;
   #include "dbg.h"
   if(getenv("SS_VERIF_NAME")) {
     verif_name = std::string(getenv("SS_VERIF_NAME"));
   }
 } 
};

#endif
