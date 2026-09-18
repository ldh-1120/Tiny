#pragma once

namespace tiny {
	void initializePlatform();
	void shutdownPlatform();

	int runMessageLoop();
}