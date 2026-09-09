#pragma once
#include "RE/GFxCharEvent.h"

namespace InputMethod
{
	class Manager : public REX::Singleton<Manager>
	{
	public:
		void Initialize(HWND a_hWnd);
		std::uint32_t GetConsoleKeyCode() const;

		bool Enable();
		bool Disable();
		bool IsEnabled() const;
		void ResetState();
		void UpdateImeWindowPosition();

		void RecordComposing(bool a_isComposing);
		bool IsComposing() const;
		void CancelComposition() const;
		void SetCompositionWindowFont(HKL a_keyboardLayout, std::optional<BYTE> a_characterSet) const;
		
		void RecordCandidateWindowOpen(bool a_isOpen);
		bool IsCandidateWindowOpen() const;

		void ClearPendingCharResult();
		void ProcessCharResult(std::uint16_t a_codeUnit, std::uint16_t a_repeatCount);
		std::wstring GetImeResultString() const;
		void ProcessImeResult();

	private:
		void SetImeWindowPosition(bool a_isFollowCaret) const;
		bool NeedSetImeWindowPosition(HIMC a_imeContext, POINT a_position) const;
		void SetCompositionWindowPosition(HIMC a_imeContext, POINT a_position) const;
		void SetCandidateWindowPosition(HIMC a_imeContext, POINT a_position) const;
		void SendCodePoint(std::uint32_t a_codePoint);

		HWND gameWindow{ nullptr };
		std::uint32_t consoleKeyCode{ RE::ControlMap::kInvalid };

		std::atomic<bool> isEnabled{ false };
		std::atomic<bool> isImePositionUpdatePending{ false };

		std::atomic<bool> isComposing{ false };
		bool isCandidateWindowOpen{ false };
		std::optional<std::uint16_t> pendingCharHighSurrogate;

		static inline constexpr std::size_t charEventPoolSize{ 256 };
		std::array<RE::GFxCharEvent, charEventPoolSize> charEventPool{};
		std::size_t charEventIndex{ 0 };
	};
}
