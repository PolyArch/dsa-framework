#include "testing.h"

int main(int argc, char* argv[]) {
  begin_roi();
  SS_CONFIG(none_config, none_size);
  SS_SCR_PORT_STREAM(0, 0, 0, 1, P_none_in);
  SS_WAIT_ALL();
  end_roi();
  return 0;
}
