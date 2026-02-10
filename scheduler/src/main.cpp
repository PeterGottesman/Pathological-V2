#include "scheduler.hpp"
#include <CLI/CLI.hpp>

int main(int argc, char** argv) {
    CLI::App app{"Pathological Scheduler"};

	bool some_option;

	// Likely options:
	// - ips/ports to listen on
	// - database connections
	// - any config files
	// - ...
	app.add_option("someoption", some_option, "Some fake option")->required();
	CLI11_PARSE(app, argc, argv);

	Scheduler sched{some_option};
}
