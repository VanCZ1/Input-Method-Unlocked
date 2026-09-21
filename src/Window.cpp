#include "Window.h"
#include "InputMethod.h"
#include "Input.h"
#include "Utils.h"

namespace Window
{
	namespace
	{
		bool shouldBlockNextResult{ false };
	}

	LRESULT ProcessWindowMessage(WNDPROC a_originalFunction, HWND a_hWnd, UINT a_uMsg, WPARAM a_wParam, LPARAM a_lParam)
	{
		auto inputMethodManager = InputMethod::Manager::GetSingleton();
		if (!inputMethodManager->IsEnabled()) {
			return CallWindowProcA(a_originalFunction, a_hWnd, a_uMsg, a_wParam, a_lParam);
		}

		switch (a_uMsg) {
		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
			{
				if (!inputMethodManager->IsComposing()) {
					const auto keyCode = Utils::DirectInput::GetKeyCodeFromKeyData(static_cast<std::uint32_t>(a_lParam));
					const auto consoleKeyCode = inputMethodManager->GetConsoleKeyCode();
					if (keyCode == consoleKeyCode) {
						shouldBlockNextResult = true;
					}
				}

				return CallWindowProcA(a_originalFunction, a_hWnd, a_uMsg, a_wParam, a_lParam);
			}
		case WM_CHAR:
			{
				if (shouldBlockNextResult) {
					shouldBlockNextResult = false;
					inputMethodManager->ClearPendingCharResult();
					return CallWindowProcA(a_originalFunction, a_hWnd, a_uMsg, a_wParam, a_lParam);
				}

				if (a_wParam <= std::numeric_limits<std::uint8_t>::max()) {
					inputMethodManager->ProcessCharResult(static_cast<std::uint8_t>(a_wParam), LOWORD(a_lParam));
				} else {
					auto charBytes = a_wParam;
					do {
						inputMethodManager->ProcessCharResult(static_cast<std::uint8_t>(charBytes), LOWORD(a_lParam));
						charBytes >>= 8;
					} while (charBytes != 0);
				}
				
				return CallWindowProcA(a_originalFunction, a_hWnd, a_uMsg, a_wParam, a_lParam);
			}
		case WM_KILLFOCUS:
			{
				Input::Manager::GetSingleton()->ResetKeyState();
				return CallWindowProcA(a_originalFunction, a_hWnd, a_uMsg, a_wParam, a_lParam);
			}
		case WM_INPUTLANGCHANGE:
			{
				const auto keyboardLayout = reinterpret_cast<HKL>(a_lParam);
				inputMethodManager->ClearPendingCharResult();
				inputMethodManager->UpdateCharCodePage(keyboardLayout);
				inputMethodManager->SetCompositionWindowFont(keyboardLayout, static_cast<BYTE>(a_wParam));
				return CallWindowProcA(a_originalFunction, a_hWnd, a_uMsg, a_wParam, a_lParam);
			}
		case WM_IME_SETCONTEXT:
			{
				return DefWindowProcA(a_hWnd, a_uMsg, a_wParam, a_lParam);
			}
		case WM_IME_STARTCOMPOSITION:
			{
				Input::Manager::GetSingleton()->OnCompositionStart();
				inputMethodManager->RecordComposing(true);
				return DefWindowProcA(a_hWnd, a_uMsg, a_wParam, a_lParam);
			}
		case WM_IME_COMPOSITION:
			{
				if (shouldBlockNextResult) {
					shouldBlockNextResult = false;
					inputMethodManager->CancelComposition();
					return 0;
				}

				if ((a_lParam & GCS_RESULTSTR) != 0) {
					inputMethodManager->ProcessImeResult();
					a_lParam &= ~GCS_RESULTSTR;
				}

				return DefWindowProcA(a_hWnd, a_uMsg, a_wParam, a_lParam);
			}
		case WM_IME_ENDCOMPOSITION:
			{
				Input::Manager::GetSingleton()->OnCompositionEnd();
				inputMethodManager->RecordComposing(false);
				return DefWindowProcA(a_hWnd, a_uMsg, a_wParam, a_lParam);
			}
		case WM_IME_CHAR:
			{
				return 0;
			}
		case WM_IME_NOTIFY:
			{
				return DefWindowProcA(a_hWnd, a_uMsg, a_wParam, a_lParam);
			}
		default:
			break;
		}

		return CallWindowProcA(a_originalFunction, a_hWnd, a_uMsg, a_wParam, a_lParam);
	}

	void ResetMessageState()
	{
		shouldBlockNextResult = false;
	}
}
