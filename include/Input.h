#pragma once

namespace Input
{
	class Manager : public REX::Singleton<Manager>
	{
	public:
		bool ShouldBlockButtonEvent(RE::INPUT_DEVICE a_device, std::uint32_t a_keyCode, float a_value, float a_heldDuration);
		void OnCompositionStart();
		void OnCompositionEnd();
		void ResetKeyState();

	private:
		std::mutex keyStateMutex;
		static inline constexpr std::size_t directInputKeyCount{ 256 };

		std::optional<std::uint32_t> lastKeyInComposing;
		std::optional<std::uint32_t> endKeyInComposing;
		std::bitset<directInputKeyCount> modifierKeyPassed;
		std::bitset<directInputKeyCount> characterKeyWithCtrl;
		bool shouldCaptureLastKeyInComposing{ false };
		bool shouldCaptureEndKeyInComposing{ false };
	};
}
