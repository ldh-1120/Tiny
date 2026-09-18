#include <tiny/platform/SystemClipboard.h>

#include <cstring>
#include <string>

#include <Windows.h>

#include <tiny/core/text/Unicode.h>

namespace tiny {
	namespace {
		class ClipboardSession {
		public:
			ClipboardSession() : opened(OpenClipboard(nullptr) != FALSE) { }
			~ClipboardSession() {
				if (opened)
					CloseClipboard();
			}

			ClipboardSession(const ClipboardSession&) = delete;
			ClipboardSession& operator=(const ClipboardSession&) = delete;

			bool isOpen() const {
				return opened;
			}

		private:
			bool opened = false;
		};
	}

	bool SystemClipboard::writeText(std::u32string_view text) {
		std::wstring wideText = utf32ToWide(text);

		SIZE_T byteSize = (wideText.size() + 1) * sizeof(wchar_t);

		HGLOBAL memory = GlobalAlloc(GMEM_MOVEABLE, byteSize);
		if (!memory)
			return false;

		void* destination = GlobalLock(memory);
		if (!destination) {
			GlobalFree(memory);
			return false;
		}

		std::memcpy(destination, wideText.c_str(), byteSize);

		GlobalUnlock(memory);

		ClipboardSession session;
		if (!session.isOpen()) {
			GlobalFree(memory);
			return false;
		}

		if (EmptyClipboard() == FALSE) {
			GlobalFree(memory);
			return false;
		}

		HANDLE result = SetClipboardData(CF_UNICODETEXT, memory);
		if (!result) {
			GlobalFree(memory);
			return false;
		}

		return true;
	}

	std::optional<std::u32string> SystemClipboard::readText() {
		if (IsClipboardFormatAvailable(CF_UNICODETEXT) == FALSE)
			return std::nullopt;

		ClipboardSession session;
		if (!session.isOpen())
			return std::nullopt;

		HANDLE data = GetClipboardData(CF_UNICODETEXT);
		if (!data)
			return std::nullopt;

		const wchar_t* text = static_cast<const wchar_t*>(GlobalLock(data));
		if (!text)
			return std::nullopt;

		std::wstring wideText(text);

		GlobalUnlock(data);

		return wideToUtf32(wideText);
	}
}