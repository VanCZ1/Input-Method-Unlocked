#include "Input.h"
#include "InputMethod.h"
#include "Utils.h"

namespace Input
{
	namespace
	{
		bool IsPassableModifierKey(std::uint32_t a_keyCode)
		{
			switch (a_keyCode) {
			case DIK_LCONTROL:
			case DIK_RCONTROL:
			case DIK_LSHIFT:
			case DIK_RSHIFT:
				return true;
			default:
				return false;
			}
		}
	}

	bool Manager::ShouldBlockButtonEvent(RE::INPUT_DEVICE a_device, std::uint32_t a_keyCode, float a_value, float a_heldDuration)
	{
		if (a_device != RE::INPUT_DEVICE::kKeyboard || a_keyCode >= directInputKeyCount) {
			return false;
		}

		std::scoped_lock lock(keyStateMutex);

		auto inputMethodManager = InputMethod::Manager::GetSingleton();
		if (!inputMethodManager->IsEnabled()) {
			return false;
		}

		bool shouldBlockEvent = false;
		const auto keyIndex = static_cast<std::size_t>(a_keyCode);
		const bool isPressed = a_value > 0.0f;
		const bool isDown = isPressed && a_heldDuration == 0.0f;
		const bool isComposing = inputMethodManager->IsComposing();
		const auto consoleKeyCode = inputMethodManager->GetConsoleKeyCode();

		if (isComposing) {
			shouldBlockEvent = true;

			if (IsPassableModifierKey(a_keyCode)) {
				shouldBlockEvent = !modifierKeyPassed.test(keyIndex);
				if (!isPressed) {
					modifierKeyPassed.reset(keyIndex);
				}
			}

			if (isDown) {
				shouldCaptureLastKeyInComposing = true;
				lastKeyInComposing = a_keyCode;
			} else if (!isPressed && a_keyCode == lastKeyInComposing) {
				lastKeyInComposing.reset();
			}
		} else {
			if (shouldCaptureEndKeyInComposing && isDown) {
				shouldCaptureEndKeyInComposing = false;
				endKeyInComposing = a_keyCode;
			}

			if (a_keyCode == endKeyInComposing) {
				shouldBlockEvent = true;

				if (!isPressed) {
					endKeyInComposing.reset();
				}
			} else if (a_keyCode == consoleKeyCode) {
				shouldBlockEvent = false;
			} else if (Utils::DirectInput::IsTextInputModifierKey(a_keyCode)) {
				shouldBlockEvent = true;

				if (IsPassableModifierKey(a_keyCode)) {
					shouldBlockEvent = false;
					modifierKeyPassed.set(keyIndex, isPressed);
				}
			} else if (Utils::DirectInput::IsTextInputCharacterKey(a_keyCode)) {
				shouldBlockEvent = true;

				if (isDown) {
					const bool isLeftCtrlPressed = Utils::DirectInput::IsKeyPressed(DIK_LCONTROL);
					const bool isRightCtrlPressed = Utils::DirectInput::IsKeyPressed(DIK_RCONTROL);
					characterKeyWithCtrl.set(keyIndex, isLeftCtrlPressed || isRightCtrlPressed);
				}
				if (characterKeyWithCtrl.test(keyIndex)) {
					shouldBlockEvent = false;
				}
				if (!isPressed) {
					characterKeyWithCtrl.reset(keyIndex);
				}
			}
		}

		return shouldBlockEvent;
	}

	void Manager::OnCompositionStart()
	{
		std::scoped_lock lock(keyStateMutex);

		lastKeyInComposing.reset();
		endKeyInComposing.reset();
		shouldCaptureLastKeyInComposing = false;
		shouldCaptureEndKeyInComposing = false;
	}

	void Manager::OnCompositionEnd()
	{
		std::scoped_lock lock(keyStateMutex);

		const bool shouldCaptureKey = std::exchange(shouldCaptureLastKeyInComposing, false);
		endKeyInComposing = std::exchange(lastKeyInComposing, std::nullopt);
		shouldCaptureEndKeyInComposing = !endKeyInComposing.has_value() && shouldCaptureKey;
	}

	void Manager::ResetKeyState()
	{
		std::scoped_lock lock(keyStateMutex);

		lastKeyInComposing.reset();
		endKeyInComposing.reset();
		modifierKeyPassed.reset();
		characterKeyWithCtrl.reset();
		shouldCaptureLastKeyInComposing = false;
		shouldCaptureEndKeyInComposing = false;
	}
}
