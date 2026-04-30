#include "render_server.hpp"
#include "scheduler_client.hpp"

#include <condition_variable>
#include <CLI/CLI.hpp>
#include <cstdint>
#include <csignal>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

// Creating reference to client singleton and unique pointer that will store the server
SchedulerClient& client = SchedulerClient::getInstance();
std::unique_ptr<Server> server;

bool shutdownSig = false;
std::mutex mutex;
std::condition_variable shutdownNow;

// Sends to STD out the signal hit and then alerts cv for
// shutdownServer thread
void signalHandler(int signal){
    std::string sig;
    if (signal == 1) {
        sig = "SIGHUP";
    } else if (signal == 2) {
        sig = "SIGINT";
    } else if (signal == 15) {
        sig = "SIGTERM";
    } else {
        sig = "UNDEFINED";
    }
    std::cout << sig << " HIT" << std::endl;
    shutdownSig = true;     // Used to stop spurious wakeup
    shutdownNow.notify_one();
}

void shutdownServer(){
    std::unique_lock<std::mutex> lock(mutex);
    shutdownNow.wait(lock, [] {return shutdownSig;});
    client.Disconnect();
    server->Shutdown();
}

int main(int argc, char** argv) {
    CLI::App app{"Pathological - Vulkan Path Tracer"};

    std::string schedulerAddress;
    std::string renderServerAddress = "127.0.0.1";
    uint32_t renderServerPort = 50051;
    std::string worker_id = "test_client_1";
    boost::uuids::random_generator random_gen;

    app.add_option("schedulerAddress", schedulerAddress, "Address and port to find scheduler at")->required();
    app.add_option("-a,--render-address", renderServerAddress, "Address of current machine running program")->default_val("127.0.0.1");
    app.add_option("-p,--port", renderServerPort, "Port to launch server")->default_val(50051);
    app.add_option("-n, --name", worker_id, "Name that scheduler will refer to this render worker as");
    CLI11_PARSE(app, argc, argv);

    // Sets the members needed for the scheduler client
    client.SetMembers(
        grpc::CreateChannel(schedulerAddress, grpc::InsecureChannelCredentials()),
        worker_id,
        renderServerAddress,
        renderServerPort
    );

    // If cannot establish connection with scheduler exit
    if(client.EstablishConnection() == 1){
        std::cerr << "Could not establish connection with scheduler. Now exiting." << std::endl;
        return 1;
    }

    // Signal handling to alert scheduler that render worker is down
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    signal(SIGHUP, signalHandler);  // Probably should change this to just pause process
                                    // but exits for now

    server = BuildServer(renderServerPort, client, worker_id, random_gen);
    std::thread shutdown_thread(shutdownServer);
    server->Wait();
    shutdown_thread.join();

    return 0;
}
