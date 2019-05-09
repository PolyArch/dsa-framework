#include "testing.h"

int main(int argc, char* argv[]) {

  init();

  begin_roi();
  SS_CONFIG(none_config,none_size);
  SS_WAIT_ALL();
  end_roi();
}
