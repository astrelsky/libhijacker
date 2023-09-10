#pragma once

#include <stdint.h>
#include <stdlib.h>
#include "thread.hpp"

struct AppMessage;

extern "C" uint32_t sceAppMessagingSendMsg(uint32_t appId, uint32_t msgType, const void *msg, size_t msgLength, uint32_t flags);

extern "C" int sceAppMessagingReceiveMsg(const AppMessage *msg);

struct AppMessage {
	static constexpr size_t PAYLOAD_SIZE = 8192;
	uint32_t sender;
	uint32_t msgType;
	uint8_t payload[PAYLOAD_SIZE];
	uint32_t payloadSize;
	uint64_t timestamp;
};

enum HomebrewDaemonMessageType : uint32_t {
	BREW_MSG_TYPE_REGISTER_PREFIX_HANDLER = 0x1000000,
	BREW_MSG_TYPE_REGISTER_LAUNCH_LISTENER,
	BREW_MSG_TYPE_APP_LAUNCHED
};

JThread startMessageReceiver() noexcept;
bool notifyHandlers(const uint32_t prefix, const pid_t pid, const bool isHomebrew) noexcept;
bool hasPrefixHandler(const uint32_t prefix) noexcept;
