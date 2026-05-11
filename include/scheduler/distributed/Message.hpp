#pragma once

#include <sstream>
#include <string>

namespace scheduler::distributed {

enum class MessageType {
    Register,
    Heartbeat,
    Result,
    Ack,
    Error,
    Unknown
};

struct Message {
    MessageType type = MessageType::Unknown;
    std::string workerId;
    std::string payload;

    [[nodiscard]] std::string serialize() const
    {
        switch (type) {
        case MessageType::Register:
            return "REGISTER " + workerId;
        case MessageType::Heartbeat:
            return "HEARTBEAT " + workerId;
        case MessageType::Result:
            return "RESULT " + workerId + " " + payload;
        case MessageType::Ack:
            return "ACK " + payload;
        case MessageType::Error:
            return "ERROR " + payload;
        case MessageType::Unknown:
            return "UNKNOWN " + payload;
        }

        return "UNKNOWN";
    }

    [[nodiscard]] static Message parse(const std::string& text)
    {
        std::istringstream stream(text);
        std::string command;
        std::string workerId;
        stream >> command >> workerId;

        if (command == "REGISTER" && !workerId.empty()) {
            return {MessageType::Register, workerId, ""};
        }

        if (command == "HEARTBEAT" && !workerId.empty()) {
            return {MessageType::Heartbeat, workerId, ""};
        }

        if (command == "RESULT" && !workerId.empty()) {
            std::string payload;
            std::getline(stream, payload);
            if (!payload.empty() && payload.front() == ' ') {
                payload.erase(0, 1);
            }
            return {MessageType::Result, workerId, payload};
        }

        if (command == "ACK") {
            std::string payload;
            std::getline(stream, payload);
            if (!payload.empty() && payload.front() == ' ') {
                payload.erase(0, 1);
            }
            return {MessageType::Ack, "", payload};
        }

        if (command == "ERROR") {
            std::string payload;
            std::getline(stream, payload);
            if (!payload.empty() && payload.front() == ' ') {
                payload.erase(0, 1);
            }
            return {MessageType::Error, "", payload};
        }

        return {MessageType::Unknown, "", text};
    }
};

} // namespace scheduler::distributed
