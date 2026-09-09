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

	RE::InputEvent* Manager::ProcessInputEvent(RE::InputEvent* a_eventHead)
	{
		if (!a_eventHead) {
			return a_eventHead;
		}

		std::scoped_lock lock(keyStateMutex);

		auto inputMethodManager = InputMethod::Manager::GetSingleton();
		if (!inputMethodManager->IsEnabled()) {
			return a_eventHead;
		}

		const bool isComposing = inputMethodManager->IsComposing();
		const auto consoleKeyCode = inputMethodManager->GetConsoleKeyCode();
		bool isLeftCtrlPressed = Utils::Input::WasDirectInputKeyPressed(DIK_LCONTROL);
		bool isRightCtrlPressed = Utils::Input::WasDirectInputKeyPressed(DIK_RCONTROL);

		auto currentEventPtr = &a_eventHead;
		while (*currentEventPtr) {
			bool shouldBlockCurrentEvent = false;
			const auto currentEvent = *currentEventPtr;
			const auto eventType = currentEvent->GetEventType();

			switch (eventType) {
			case RE::INPUT_EVENT_TYPE::kButton:
				{
					const auto buttonEvent = currentEvent->AsButtonEvent();
					if (buttonEvent && buttonEvent->GetDevice() == RE::INPUT_DEVICE::kKeyboard) {
						const auto keyCode = buttonEvent->GetIDCode();
						const auto keyIndex = static_cast<std::size_t>(keyCode);

						if (isComposing) {
							shouldBlockCurrentEvent = true;

							if (IsPassableModifierKey(keyCode)) {
								shouldBlockCurrentEvent = !modifierKeyPassed.test(keyIndex);
								if (!buttonEvent->IsPressed()) {
									modifierKeyPassed.reset(keyIndex);
								}
							}

							if (buttonEvent->IsDown()) {
								shouldCaptureLastKeyInComposing = true;
								lastKeyInComposing = keyCode;
							} else if (!buttonEvent->IsPressed() && keyCode == lastKeyInComposing) {
								lastKeyInComposing.reset();
							}
						} else {
							if (shouldCaptureEndKeyInComposing && buttonEvent->IsDown()) {
								shouldCaptureEndKeyInComposing = false;
								endKeyInComposing = keyCode;
							}

							if (keyCode == endKeyInComposing) {
								shouldBlockCurrentEvent = true;
								endKeyInComposing.reset();
							} else if (keyCode == consoleKeyCode) {
								shouldBlockCurrentEvent = false;
							} else if (Utils::Input::IsTextInputModifierKey(keyCode)) {
								shouldBlockCurrentEvent = true;

								if (IsPassableModifierKey(keyCode)) {
									shouldBlockCurrentEvent = false;

									if (keyCode == DIK_LCONTROL) {
										isLeftCtrlPressed = buttonEvent->IsPressed();
									} else if (keyCode == DIK_RCONTROL) {
										isRightCtrlPressed = buttonEvent->IsPressed();
									}

									modifierKeyPassed.set(keyIndex, buttonEvent->IsPressed());
								}
							} else if (Utils::Input::IsTextInputCharacterKey(keyCode)) {
								shouldBlockCurrentEvent = true;

								if (buttonEvent->IsDown()) {
									characterKeyWithCtrl.set(keyIndex, isLeftCtrlPressed || isRightCtrlPressed);
								}
								if (characterKeyWithCtrl.test(keyIndex)) {
									shouldBlockCurrentEvent = false;
								}
								if (!buttonEvent->IsPressed()) {
									characterKeyWithCtrl.reset(keyIndex);
								}
							}
						}
					}
				}
				break;
			case RE::INPUT_EVENT_TYPE::kChar:
				{
					shouldBlockCurrentEvent = true;
				}
				break;
			default:
				break;
			}

			if (shouldBlockCurrentEvent) {
				*currentEventPtr = (*currentEventPtr)->next;
			} else {
				currentEventPtr = &(*currentEventPtr)->next;
			}
		}

		return a_eventHead;
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
