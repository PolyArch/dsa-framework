#include "testing.h"

int main(int argc, char* argv[]) {
  begin_roi();
  SS_WAIT_SCR_RD();
  end_roi();
}
