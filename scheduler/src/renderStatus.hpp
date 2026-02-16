#pragma once

// Could maybe add 'created' state upon creation of the request
enum class RenderStatus
{
    COMPLETED,
    IN_PROGRESS,
    IN_QUEUE,
    ERROR,
    UNKNOWN
};

inline std::string renderStatusToString(RenderStatus status)
{
    switch (status)
    {
    case RenderStatus::COMPLETED:
        return "Completed";
    case RenderStatus::IN_PROGRESS:
        return "In Progress";
    case RenderStatus::IN_QUEUE:
        return "In Queue";
    case RenderStatus::ERROR:
        return "Error";
    case RenderStatus::UNKNOWN:
        return "Unknown";
    default:
        return "Unknown";
    }
}