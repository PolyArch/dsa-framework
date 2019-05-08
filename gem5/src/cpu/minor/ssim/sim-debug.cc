#include "sim-debug.hh"


#undef DO_DBG
#define DO_DBG(x) bool SS_DEBUG::x = false;
#include "dbg.h"

std::string SS_DEBUG::verif_name = std::string("");
bool SS_DEBUG::pred = true;
