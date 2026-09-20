#include "Hooks.h"
#include "InputMethod.h"
#include "Window.h"
#include "Input.h"
#include "Utils.h"

namespace Hooks
{
	namespace
	{
		template <class T>
		bool PatchVTable(void* a_object, std::size_t a_index, const T a_hook, T& a_original)
		{
			if (!a_object) {
				return false;
			}

			auto vTable = *reinterpret_cast<void***>(a_object);
			if (!vTable) {
				return false;
			}

			DWORD oldProtect = 0;
			bool isSuccess = VirtualProtect(&vTable[a_index], sizeof(void*), PAGE_EXECUTE_READWRITE, &oldProtect);
			if (!isSuccess) {
				return false;
			}

			a_original = reinterpret_cast<T>(vTable[a_index]);
			if (a_original) {
				vTable[a_index] = reinterpret_cast<void*>(a_hook);
			}
			VirtualProtect(&vTable[a_index], sizeof(void*), oldProtect, &oldProtect);

			return a_original != nullptr;
		}
	}

	class SetCooperativeLevelHook
	{
	public:
		static void Install(IDirectInputDevice8A* a_inputDevice)
		{
			if (isInstalled) {
				return;
			}

			keyboardDevice = a_inputDevice;
			if (PatchVTable(a_inputDevice, 13, Thunk, originalFunction)) {
				isInstalled = true;
			} else {
				logger::error("Failed to hook IDirectInputDevice8A::SetCooperativeLevel.");
			}
		}

	private:
		static HRESULT STDMETHODCALLTYPE Thunk(IDirectInputDevice8A* a_this, HWND a_hWnd, DWORD a_flags)
		{
			if (a_this && a_this == keyboardDevice) {
				a_flags &= ~(DISCL_EXCLUSIVE | DISCL_NONEXCLUSIVE);
				a_flags |= DISCL_NONEXCLUSIVE;
			}

			return originalFunction(a_this, a_hWnd, a_flags);
		}

		static inline bool isInstalled{ false };
		static inline IDirectInputDevice8A* keyboardDevice;
		static inline decltype(&Thunk) originalFunction;
	};

	class CreateDeviceHook
	{
	public:
		static void Install(IDirectInput8A* a_dinput)
		{
			if (isInstalled) {
				return;
			}

			if (PatchVTable(a_dinput, 3, Thunk, originalFunction)) {
				isInstalled = true;
			} else {
				logger::error("Failed to hook IDirectInput8::CreateDevice.");
			}
		}

	private:
		static HRESULT STDMETHODCALLTYPE Thunk(IDirectInput8A* a_this, REFGUID a_guid, LPDIRECTINPUTDEVICE8A* a_device, LPUNKNOWN a_outer)
		{
			const auto result = originalFunction(a_this, a_guid, a_device, a_outer);

			if (result == DI_OK && a_guid == GUID_SysKeyboard && a_device && *a_device) {
				SetCooperativeLevelHook::Install(*a_device);
			}

			return result;
		}

		static inline bool isInstalled{ false };
		static inline decltype(&Thunk) originalFunction;
	};

	class DirectInput8CreateHook
	{
	public:
		static void Install()
		{
			const auto originalAddress = SKSE::PatchIAT(Thunk, "dinput8.dll", "DirectInput8Create");
			originalFunction = reinterpret_cast<decltype(originalFunction)>(originalAddress);
			if (!originalFunction) {
				logger::error("Failed to hook DirectInput8Create");
			}
		}

	private:
		static HRESULT WINAPI Thunk(HINSTANCE a_hinst, DWORD a_version, REFIID a_riid, LPVOID* a_out, LPUNKNOWN a_outer)
		{
			const auto result = originalFunction(a_hinst, a_version, a_riid, a_out, a_outer);

			if (result == DI_OK && a_out && *a_out) {
				const auto dinput = static_cast<IDirectInput8A*>(*a_out);
				CreateDeviceHook::Install(dinput);
			}

			return result;
		}

		static inline decltype(&Thunk) originalFunction;
	};

	class ToUnicodeHook
	{
	public:
		static void Install()
		{
			REL::Relocation<std::uintptr_t> target{ RELOCATION_ID(67472, 68782), REL::VariantOffset(0x20D, 0x2CB, 0x1B7) };
			auto& trampoline = SKSE::GetTrampoline();
			const auto originalAddress = trampoline.write_call<6>(target.address(), Thunk);
			originalFunction = *reinterpret_cast<std::uintptr_t*>(originalAddress);
		}

	private:
		static int WINAPI Thunk(UINT a_virtualKey, UINT a_scanCode, const BYTE* a_keyState, LPWSTR a_buffer, int a_bufferLength, UINT a_flags)
		{
			constexpr UINT dontChangeKeyboardState = 1u << 2;
			a_flags |= dontChangeKeyboardState;

			return originalFunction(a_virtualKey, a_scanCode, a_keyState, a_buffer, a_bufferLength, a_flags);
		}

		static inline REL::Relocation<decltype(Thunk)> originalFunction;
	};

	class ProcessInputQueueHook
	{
	public:
		static void Install()
		{
			REL::Relocation<std::uintptr_t> target{ RELOCATION_ID(67315, 68617), REL::VariantOffset(0x7B, 0x7B, 0x81) };
			auto& trampoline = SKSE::GetTrampoline();
			originalFunction = trampoline.write_call<5>(target.address(), Thunk);
		}

	private:
		static void Thunk(RE::BSTEventSource<RE::InputEvent*>* a_dispatcher, RE::InputEvent* const* a_events)
		{
			if (!a_events || !*a_events) {
				return originalFunction(a_dispatcher, a_events);
			}

			auto inputMethodManager = InputMethod::Manager::GetSingleton();
			if (!inputMethodManager->IsEnabled()) {
				return originalFunction(a_dispatcher, a_events);
			}

			const auto eventHead = Input::Manager::GetSingleton()->ProcessInputEvent(*a_events);
			RE::InputEvent* const filteredEvents[] = { eventHead };
			originalFunction(a_dispatcher, filteredEvents);
			inputMethodManager->UpdateImeWindowPosition();
		}

		static inline REL::Relocation<decltype(Thunk)> originalFunction;
	};

	class WndProcHook
	{
	public:
		static void Install(HWND a_hWnd)
		{
			originalFunction = reinterpret_cast<WNDPROC>(SetWindowLongPtrA(a_hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(Thunk)));
			if (!originalFunction) {
				logger::error("Failed to hook WndProc.");
			}
		}

	private:
		static LRESULT CALLBACK Thunk(HWND a_hWnd, UINT a_uMsg, WPARAM a_wParam, LPARAM a_lParam)
		{
			auto inputMethodManager = InputMethod::Manager::GetSingleton();
			const bool isAllowTextInput = Utils::Game::IsAllowTextInput();
			if (isAllowTextInput != inputMethodManager->IsEnabled()) {
				if (isAllowTextInput) {
					inputMethodManager->Enable();
				} else {
					if (inputMethodManager->Disable()) {
						inputMethodManager->ResetState();
						Window::ResetMessageState();
						Input::Manager::GetSingleton()->ResetKeyState();
					}
				}
			}

			if (!inputMethodManager->IsEnabled()) {
				return CallWindowProcA(originalFunction, a_hWnd, a_uMsg, a_wParam, a_lParam);
			}

			return Window::ProcessWindowMessage(originalFunction, a_hWnd, a_uMsg, a_wParam, a_lParam);
		}

		static inline WNDPROC originalFunction;
	};

	void InstallAtLoad()
	{
		logger::info("Installing hooks at Load...");
		SKSE::AllocTrampoline(22);
		DirectInput8CreateHook::Install();
		logger::info("Hooks installation is complete at Load.");
	}

	void InstallAtPostLoad()
	{
		logger::info("Installing hooks at PostLoad...");
		ToUnicodeHook::Install();
		logger::info("Hooks installation is complete at PostLoad.");
	}

	void InstallAtInputLoaded()
	{
		logger::info("Installing hooks at InputLoaded...");
		const auto renderer = RE::BSGraphics::Renderer::GetSingleton();
		if (!renderer) {
			logger::error("Failed to find renderer.");
			return;
		}
		auto& renderData = renderer->GetRuntimeData();
		const auto hWnd = reinterpret_cast<HWND>(renderData.renderWindows[0].hWnd);
		if (!hWnd) {
			logger::error("Failed to get game window handle.");
			return;
		}

		InputMethod::Manager::GetSingleton()->Initialize(hWnd);
		ProcessInputQueueHook::Install();
		WndProcHook::Install(hWnd);
		logger::info("Hooks installation is complete at InputLoaded.");
	}
}
