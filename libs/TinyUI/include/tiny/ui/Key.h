#pragma once

#include <optional>
#include <string>
#include <utility>

namespace tiny {
	class Key {
	public:
		Key() = default;

		explicit Key(std::string value) : keyValue(std::move(value)) { }

		bool hasValue() const {
			return keyValue.has_value();
		}

		const std::string& value() const {
			return keyValue.value();
		}

		bool operator==(const Key& other) const {
			return keyValue == other.keyValue;
		}

		bool operator!=(const Key& other) const {
			return keyValue != other.keyValue;
		}

	private:
		std::optional<std::string> keyValue;
	};
}