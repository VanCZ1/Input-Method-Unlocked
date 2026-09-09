#pragma once

namespace Input
{
	class Manager : public REX::Singleton<Manager>
	{
	public:
		RE::InputEvent* ProcessInputEvent(RE::InputEvent* a_eventHead);
		void OnCompositionStart();
		void OnCompositionEnd();
		void ResetKeyState();

	private:
		std::mutex keyStateMutex;

		std::optional<std::uint32_t> lastKeyInComposing;
		std::optional<std::uint32_t> endKeyInComposing;
		std::bitset<256> modifierKeyPassed;
		std::bitset<256> characterKeyWithCtrl;
		bool shouldCaptureLastKeyInComposing{ false };
		bool shouldCaptureEndKeyInComposing{ false };
	};
}
