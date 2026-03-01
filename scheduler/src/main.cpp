#include "requestController.hpp"
#include "scheduler.hpp"
#include <CLI/CLI.hpp>
#include <drogon/drogon.h>

int main(int argc, char **argv) {
  CLI::App app{"Pathological Scheduler"};

  bool some_option;
  drogon::app().addListener("0.0.0.0", 8080);

  // Likely options:
  // - ips/ports to listen on
  // - database connections
  // - any config files
  // - ...
  // app.add_option("someoption", some_option, "Some fake option")->required();
  CLI11_PARSE(app, argc, argv);

  Scheduler sched{some_option};

  // Blocking call. Everything must go above this.
  drogon::app().run();
  return 0;
}
