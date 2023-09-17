#include <stdio.h>
#include <stdint.h>
#include <pthread.h>
#include <sys/_stdint.h>
#include <sys/types.h>
#include "msg.hpp"
#include "thread.hpp"
#include "util.hpp"

extern "C" void sceLncUtilGetAppTitleId(uint32_t appId, char *titleId);

class LaunchListeners {

	// I should have made this a set
	// realistically these will be small though
	Vector<uint32_t> appIds{};
	mutable Mutex mtx{};

	public:
		void append(const uint32_t appId) noexcept {
			auto lock = mtx.lock();
			for (const auto id :appIds) {
				if (id == appId) {
					return;
				}
			}
			appIds.push_back(appId);
		}

		void remove(const uint32_t appId) noexcept {
			auto lock = mtx.lock();
			for (const auto &id :appIds) {
				if (id == appId) {
					appIds.erase(&id);
					return;
				}
			}
		}

		void handle(const pid_t pid) noexcept {
			auto lock = mtx.lock();
			size_t length = appIds.size();
			for (int i = 0; i < length; i++) {
				const auto &id = appIds[i];
				if (sceAppMessagingSendMsg(id, BREW_MSG_TYPE_APP_LAUNCHED, &pid, sizeof(pid), 0) != 0) {
					i--;
					length--;
					appIds.erase(&id);
				}
			}
		}
};

struct RegisteredPrefix {
	uint32_t appId;
	uint32_t prefix;
};

class PrefixHandlers {

	Vector<RegisteredPrefix> ids{};
	mutable Mutex mtx{};

	public:
		void append(uint32_t appId, uint32_t prefix) noexcept {
			auto lock = mtx.lock();
			for (const auto &id : ids) {
				constexpr auto TITLEID_SIZE = 9;
				char titleId[TITLEID_SIZE + 1]{};
				if (id.prefix == prefix) {
					sceLncUtilGetAppTitleId(appId, titleId);
					char sPrefix[sizeof(prefix) + 1];
					*reinterpret_cast<uint32_t *>(sPrefix) = prefix;
					sPrefix[sizeof(sPrefix) - 1] = '\0';
					printf("%s already registered to handle prefix %s\n", titleId, sPrefix);
					return;
				}
			}
			ids.emplace_back(appId, prefix);
		}

		bool handle(const uint32_t prefix, const pid_t pid) noexcept {
			auto lock = mtx.lock();
			size_t length = ids.size();
			for (int i = 0; i < length; i++) {
				const auto &id = ids[i];
				if (id.prefix == prefix) {
					if (sceAppMessagingSendMsg(id.appId, BREW_MSG_TYPE_APP_LAUNCHED, &pid, sizeof(pid), 0) != 0) {
						i--;
						length--;
						ids.erase(&id);
					}
					return true;
				}
			}
			return false;
		}

		bool canHandle(const uint32_t prefix) const noexcept {
			auto lock = mtx.lock();
			for (const auto &id : ids) {
				if (id.prefix == prefix) {
					return true;
				}
			}
			return false;
		}
};

static PrefixHandlers prefixHandlers{}; // NOLINT
static LaunchListeners launchListeners{}; // NOLINT

static int messageThread(void *unused) noexcept {
	(void)unused;
	static AppMessage msg{};

	while (true) {
		if (sceAppMessagingReceiveMsg(&msg) < 0) {
			puts("sceAppMessagingReceiveMsg failed");
		}
		printf("received msg from 0x%04x\n", msg.sender);
		printf("msgType: 0x%x\n", msg.msgType);
		printf("payloadSize: 0x%x\n", msg.payloadSize);
		switch (msg.msgType) {
			case BREW_MSG_TYPE_REGISTER_PREFIX_HANDLER: {
				prefixHandlers.append(msg.sender, *reinterpret_cast<uint32_t *>(msg.payload));
				break;
			}
			case BREW_MSG_TYPE_REGISTER_LAUNCH_LISTENER:
				if (*msg.payload) {
					launchListeners.append(msg.sender);
				} else {
					launchListeners.remove(msg.sender);
				}
				break;
			default:
				puts("unexpected msgType");
				break;
		}
	}
	return 0;
}

JThread startMessageReceiver() noexcept {
	return JThread{messageThread};
}

bool notifyHandlers(const uint32_t prefix, const pid_t pid, const bool isHomebrew) noexcept {
	if (isHomebrew) {
		// prefix handlers are notified first since they are responsible for loading the elf if applicable
		// listeners need not be notified
		printf("homebrew launched %d\n", pid);
		return prefixHandlers.handle(prefix, pid);
	}
	launchListeners.handle(pid);
	return false;
}

bool hasPrefixHandler(const uint32_t prefix) noexcept {
	return prefixHandlers.canHandle(prefix);
}
