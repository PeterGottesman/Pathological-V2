#include "requestController.hpp"
#include "scheduler.hpp"
#include "scheduler_server.hpp"
#include <CLI/CLI.hpp>
#include <drogon/drogon.h>
#include <thread>
#include <iostream>

int main(int argc, char **argv) {
    CLI::App app{"Pathological Scheduler"};

    uint16_t grpc_port = 50051;
    std::string listen_address = "0.0.0.0";
    uint16_t http_port = 8080;

    app.add_option("-p,--grpc-port", grpc_port, "Port for gRPC server")->default_val(50051);
    app.add_option("-a,--address", listen_address, "Address to listen on")->default_val("0.0.0.0");
    app.add_option("--http-port", http_port, "Port for HTTP server")->default_val(8080);

    CLI11_PARSE(app, argc, argv);

    // gRPC server runs on background thread
    std::thread grpc_thread([&]() {
        RunServer(grpc_port);
    });

    // Scheduler loop runs on background thread
    std::thread scheduler_thread([]() {
        Scheduler::getInstance().run();
    });

    // Drogon HTTP server runs on main thread
    // Blocking call - everything must go above this
    drogon::app()
        .addListener(listen_address, http_port)
        .run();

    // Cleanup when Drogon shuts down
    std::cout << "Stopping scheduler..." << std::endl;
    Scheduler::getInstance().stop();
    std::cout << "Stopping gRPC server..." << std::endl;
    StopServer();
    std::cout << "Joining gRPC thread..." << std::endl;
    grpc_thread.join();
    std::cout << "Joining scheduler thread..." << std::endl;
    scheduler_thread.join();
    std::cout << "Cleanup complete." << std::endl;

    return 0;
}