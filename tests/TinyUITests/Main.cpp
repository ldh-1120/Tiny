#include <iostream>

namespace {
	int failureCount = 0;

	void expect(bool condition, const char* message) {
		if (condition) {
			std::cout << "[PASS] " << message << '\n';
			return;
		}

		++failureCount;
		std::cerr << "[FAIL] " << message << '\n';
	}
}

int main() {
	expect(1 + 1 == 2, "basic math works");
	expect(2 * 3 == 6, "multiplication works");

	if (failureCount != 0) {
		std::cerr << '\n' << failureCount << " test(s) failed.\n";
		return 1;
	}

	std::cout << "\nAll tests passed.\n";
	return 0;
}