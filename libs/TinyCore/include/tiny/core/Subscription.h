#pragma once

#include <functional>
#include <utility>

namespace tiny {
	class Subscription {
	public:
		Subscription() = default;

		explicit Subscription(std::function<void()> unsubscribe) : unsubscribeFunction(std::move(unsubscribe)) { }

		~Subscription() {
			unsubscribe();
		}

		Subscription(const Subscription&) = delete;
		Subscription& operator=(const Subscription&) = delete;

		Subscription(Subscription&& other) noexcept : unsubscribeFunction(std::move(other.unsubscribeFunction)) {
			other.unsubscribeFunction = nullptr;
		}

		Subscription& operator=(Subscription&& other) noexcept {
			if (this == &other)
				return *this;

			unsubscribe();

			unsubscribeFunction = std::move(other.unsubscribeFunction);
			other.unsubscribeFunction = nullptr;

			return *this;
		}

		void unsubscribe() {
			if (!unsubscribeFunction)
				return;

			unsubscribeFunction();
			unsubscribeFunction = nullptr;
		}

		bool isSubscribed() const {
			return unsubscribeFunction != nullptr;
		}

	private:
		std::function<void()> unsubscribeFunction;
	};
}