#pragma once

#include <string>
#include <cstdint>

// **** Eventually, it would be preferable to use the WorkerStatus in the .proto header for easier development ****
// **** There were some issues (maybe compilation) for the implementation in the header file.                  ****

enum class WorkerStatus {
    IDLE = 0,
    BUSY = 1,
    OFFLINE = 2,
};

struct Worker {
    std::string id;
    std::string ip;
    uint32_t port;
    WorkerStatus status;
};