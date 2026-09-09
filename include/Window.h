#pragma once

namespace Window
{
	LRESULT ProcessWindowMessage(WNDPROC a_originalFunction, HWND a_hWnd, UINT a_uMsg, WPARAM a_wParam, LPARAM a_lParam);
	void ResetMessageState();
}
