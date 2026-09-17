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
			return CallWindowProcW(a_originalFunction, a_hWnd, a_uMsg, a_wParam, a_lParam);
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

				return CallWindowProcW(a_originalFunction, a_hWnd, a_uMsg, a_wParam, a_lParam);
			}
		case WM_CHAR:
			{
				if (shouldBlockNextResult) {
					shouldBlockNextResult = false;
					inputMethodManager->ClearPendingCharResult();
					return CallWindowProcW(a_originalFunction, a_hWnd, a_uMsg, a_wParam, a_lParam);
				}

				inputMethodManager->ProcessCharResult(static_cast<std::uint16_t>(a_wParam), LOWORD(a_lParam));
				return CallWindowProcW(a_originalFunction, a_hWnd, a_uMsg, a_wParam, a_lParam);
			}
		case WM_KILLFOCUS:
			{
				Input::Manager::GetSingleton()->ResetKeyState();
				return CallWindowProcW(a_originalFunction, a_hWnd, a_uMsg, a_wParam, a_lParam);
			}
		case WM_INPUTLANGCHANGE:
			{
				inputMethodManager->ClearPendingCharResult();
				inputMethodManager->SetCompositionWindowFont(reinterpret_cast<HKL>(a_lParam), static_cast<BYTE>(a_wParam));
				return CallWindowProcW(a_originalFunction, a_hWnd, a_uMsg, a_wParam, a_lParam);
			}
		case WM_IME_SETCONTEXT:
			{
				return DefWindowProcW(a_hWnd, a_uMsg, a_wParam, a_lParam);
			}
		case WM_IME_STARTCOMPOSITION:
			{
				Input::Manager::GetSingleton()->OnCompositionStart();
				inputMethodManager->ClearPendingCharResult();
				inputMethodManager->RecordComposing(true);
				return DefWindowProcW(a_hWnd, a_uMsg, a_wParam, a_lParam);
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

				return DefWindowProcW(a_hWnd, a_uMsg, a_wParam, a_lParam);
			}
		case WM_IME_ENDCOMPOSITION:
			{
				Input::Manager::GetSingleton()->OnCompositionEnd();
				inputMethodManager->ClearPendingCharResult();
				inputMethodManager->RecordComposing(false);
				return DefWindowProcW(a_hWnd, a_uMsg, a_wParam, a_lParam);
			}
		case WM_IME_CHAR:
			{
				return 0;
			}
		case WM_IME_NOTIFY:
			{
				if (a_wParam == IMN_OPENCANDIDATE) {
					inputMethodManager->RecordCandidateWindowOpen(true);
				} else if (a_wParam == IMN_CLOSECANDIDATE) {
					inputMethodManager->RecordCandidateWindowOpen(false);
				}

				return DefWindowProcW(a_hWnd, a_uMsg, a_wParam, a_lParam);
			}
		default:
			break;
		}

		return CallWindowProcW(a_originalFunction, a_hWnd, a_uMsg, a_wParam, a_lParam);
	}

	void ResetMessageState()
	{
		shouldBlockNextResult = false;
	}
}
