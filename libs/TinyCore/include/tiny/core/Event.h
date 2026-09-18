#pragma once

#include <algorithm>
#include <cstddef>
#include <functional>
#include <memory>
#include <utility>
#include <vector>

#include <tiny/core/Subscription.h>

namespace tiny {
	template<typename... Args>
	class Event {
	private:
		struct Slot {
			std::function<void(Args...)> callback;
			bool active = true;
		};

	public:
		using Callback = std::function<void(Args...)>;

		Event() = default;

		Event(const Event&) = delete;
		Event& operator=(const Event&) = delete;

		Subscription subscribe(Callback callback) {
			std::shared_ptr<Slot> slot = std::make_shared<Slot>();
			slot->callback = std::move(callback);

			slots.push_back(slot);

			std::weak_ptr<Slot> weakSlot = slot;

			return Subscription([weakSlot]() {
				std::shared_ptr<Slot> slot = weakSlot.lock();
				if (!slot)
					return;

				slot->active = false;
			});
		}

		void emit(Args... args) {
			++emitDepth;

			std::size_t count = slots.size();
			for (std::size_t index = 0; index < count; ++index) {
				std::shared_ptr<Slot> slot = slots[index];
				if (!slot->active)
					continue;

				slot->callback(args...);
			}

			--emitDepth;
			if (emitDepth == 0)
				removeInactiveSlots();
		}

		void clear() {
			for (const std::shared_ptr<Slot>& slot : slots)
				slot->active = false;

			if (emitDepth == 0)
				slots.clear();
		}

		std::size_t listenerCount() const {
			std::size_t count = 0;
			for (const std::shared_ptr<Slot>& slot : slots) {
				if (slot->active)
					++count;
			}

			return count;
		}

		bool empty() const {
			return listenerCount() == 0;
		}

	private:
		void removeInactiveSlots() {
			std::erase_if(slots, [](const std::shared_ptr<Slot>& slot) {
				return !slot->active;
			});
		}

	private:
		std::vector<std::shared_ptr<Slot>> slots;
		std::size_t emitDepth = 0;
	};
}