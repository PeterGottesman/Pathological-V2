#include "rpcserver.hpp"
#include "webserver.hpp"

class Scheduler {
public:
	Scheduler([[maybe_unused]] bool some_option) : rpc_server{}, web_server{} {
		// Constructor
	}

private:
	GRPCServer rpc_server;
	WebServer web_server;
};
