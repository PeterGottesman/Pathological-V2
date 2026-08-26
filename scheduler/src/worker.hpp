#pragma once

#include <cstdint>
#include <string>

// **** Eventually, it would be preferable to use the WorkerStatus in the .proto header for easier development ****
// **** There were some issues (maybe compilation) for the implementation in the header file.                  ****

enum class WorkerStatus : std::uint8_t {
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