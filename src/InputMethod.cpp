#include "InputMethod.h"
#include "Scaleform.h"
#include "Utils.h"

namespace InputMethod
{
	namespace
	{
		std::optional<POINT> GetDefaultImeWindowPosition(HWND a_hWnd)
		{
			if (!a_hWnd) {
				return std::nullopt;
			}

			RECT client{};
			if (GetClientRect(a_hWnd, &client)) {
				POINT position{};
				position.x = static_cast<LONG>(client.right * 0.75);
				position.y = static_cast<LONG>(client.bottom * 0.95);
				return position;
			}

			return std::nullopt;
		}
	}

	void Manager::Initialize(HWND a_hWnd)
	{
		gameWindow = a_hWnd;
		consoleKeyCode = Utils::Game::GetConsoleKeyCode();
		Disable();
	}

	std::uint32_t Manager::GetConsoleKeyCode() const
	{
		return consoleKeyCode;
	}

	bool Manager::Enable()
	{
		if (!gameWindow) {
			return false;
		}

		const bool wasEnabled = isEnabled.load(std::memory_order_relaxed);
		isEnabled.store(true, std::memory_order_relaxed);

#pragma warning(suppress: 6387)
		if (!ImmAssociateContextEx(gameWindow, nullptr, IACE_DEFAULT)) {
			isEnabled.store(wasEnabled, std::memory_order_relaxed);
			logger::error("Failed to enable association.");
			return false;
		}
		SetImeWindowPosition(true);
		SetCompositionWindowFont(GetKeyboardLayout(0), std::nullopt);

		return true;
	}

	bool Manager::Disable()
	{
		if (!gameWindow) {
			return false;
		}

		const bool wasEnabled = isEnabled.load(std::memory_order_relaxed);
		isEnabled.store(false, std::memory_order_relaxed);

		SetImeWindowPosition(false);
#pragma warning(suppress: 6387)
		if (!ImmAssociateContextEx(gameWindow, nullptr, 0)) {
			isEnabled.store(wasEnabled, std::memory_order_relaxed);
			logger::error("Failed to disable association.");
			return false;
		}

		return true;
	}

	bool Manager::IsEnabled() const
	{
		return isEnabled.load(std::memory_order_relaxed);
	}

	void Manager::ResetState()
	{
		isComposing.store(false, std::memory_order_relaxed);
		isCandidateWindowOpen = false;
		pendingCharHighSurrogate.reset();
	}

	void Manager::UpdateImeWindowPosition()
	{
		const auto task = SKSE::GetTaskInterface();
		if (!task) {
			return;
		}

		if (isImePositionUpdatePending.exchange(true, std::memory_order_relaxed)) {
			return;
		}

		task->AddUITask([this]() {
			if (IsEnabled()) {
				SetImeWindowPosition(true);
			}

			isImePositionUpdatePending.store(false, std::memory_order_relaxed);
		});
	}

	void Manager::RecordComposing(bool a_isComposing)
	{
		isComposing.store(a_isComposing, std::memory_order_relaxed);
	}

	bool Manager::IsComposing() const
	{
		return isComposing.load(std::memory_order_relaxed);
	}

	void Manager::CancelComposition() const
	{
		if (!gameWindow) {
			return;
		}

		const auto imeContext = ImmGetContext(gameWindow);
		if (!imeContext) {
			return;
		}

		ImmNotifyIME(imeContext, NI_COMPOSITIONSTR, CPS_CANCEL, 0);
		ImmReleaseContext(gameWindow, imeContext);
	}

	void Manager::SetCompositionWindowFont(HKL a_keyboardLayout, std::optional<BYTE> a_characterSet) const
	{
		if (!gameWindow) {
			return;
		}

		if (!a_keyboardLayout) {
			return;
		}

		const auto imeContext = ImmGetContext(gameWindow);
		if (!imeContext) {
			return;
		}

		LOGFONTW compositionFont{};
		if (ImmGetCompositionFontW(imeContext, &compositionFont)) {
			BYTE characterSet = DEFAULT_CHARSET;
			if (a_characterSet) {
				characterSet = *a_characterSet;
			} else {
				const auto languageId = LOWORD(reinterpret_cast<std::uintptr_t>(a_keyboardLayout));
				const auto localeId = MAKELCID(languageId, SORT_DEFAULT);

				CHARSETINFO charsetInfo{};
				if (TranslateCharsetInfo(reinterpret_cast<DWORD*>(static_cast<std::uintptr_t>(localeId)), &charsetInfo, TCI_SRCLOCALE)) {
					characterSet = static_cast<BYTE>(charsetInfo.ciCharset);
				}
			}

			if (compositionFont.lfCharSet != characterSet) {
				compositionFont.lfCharSet = characterSet;
				compositionFont.lfFaceName[0] = L'\0';
				ImmSetCompositionFontW(imeContext, &compositionFont);
			}
		}

		ImmReleaseContext(gameWindow, imeContext);
	}

	void Manager::RecordCandidateWindowOpen(bool a_isOpen)
	{
		isCandidateWindowOpen = a_isOpen;
	}

	bool Manager::IsCandidateWindowOpen() const
	{
		return isCandidateWindowOpen;
	}

	void Manager::ClearPendingCharResult()
	{
		pendingCharHighSurrogate.reset();
	}

	void Manager::ProcessCharResult(std::uint16_t a_codeUnit, std::uint16_t a_repeatCount)
	{
		if (IS_HIGH_SURROGATE(a_codeUnit)) {
			pendingCharHighSurrogate = a_codeUnit;
			return;
		}

		std::uint32_t codePoint = static_cast<std::uint32_t>(a_codeUnit);

		const auto charHighSurrogate = std::exchange(pendingCharHighSurrogate, std::nullopt);
		if (IS_LOW_SURROGATE(a_codeUnit)) {
			if (!charHighSurrogate) {
				return;
			}
			codePoint = Utils::Unicode::DecodeSurrogatePair(*charHighSurrogate, a_codeUnit);
		}

		if (Utils::Unicode::IsTextCodePoint(codePoint)) {
			const auto repeatCount = std::max<std::size_t>(a_repeatCount, 1);
			for (std::size_t index = 0; index < repeatCount; ++index) {
				SendCodePoint(codePoint);
			}
		}
	}

	std::wstring Manager::GetImeResultString() const
	{
		std::wstring result;
		if (!gameWindow) {
			return result;
		}

		const auto imeContext = ImmGetContext(gameWindow);
		if (!imeContext) {
			return result;
		}

		constexpr auto codeUnitSize = static_cast<LONG>(sizeof(wchar_t));
		const auto bufferLength = ImmGetCompositionStringW(imeContext, GCS_RESULTSTR, nullptr, 0);
		if (bufferLength > 0) {
			result.resize(static_cast<std::size_t>(bufferLength) / codeUnitSize);
			const auto bufferReadLength = ImmGetCompositionStringW(imeContext, GCS_RESULTSTR, result.data(), static_cast<DWORD>(bufferLength));
			if (bufferReadLength > 0) {
				result.resize(static_cast<std::size_t>(bufferReadLength) / codeUnitSize);
			} else {
				result.clear();
			}
		}

		ImmReleaseContext(gameWindow, imeContext);
		return result;
	}

	void Manager::ProcessImeResult()
	{
		const auto imeResultStr = GetImeResultString();
		if (imeResultStr.empty()) {
			return;
		}

		constexpr std::uint32_t replacementCharacter = 0xFFFD;
		for (std::size_t index = 0; index < imeResultStr.size(); ++index) {
			const auto firstCodeUnit = static_cast<std::uint16_t>(imeResultStr[index]);
			std::uint32_t codePoint = static_cast<std::uint32_t>(firstCodeUnit);

			if (IS_HIGH_SURROGATE(firstCodeUnit)) {
				const std::size_t nextIndex = index + 1;
				if (nextIndex < imeResultStr.size()) {
					const auto secondCodeUnit = static_cast<std::uint16_t>(imeResultStr[nextIndex]);
					if (IS_LOW_SURROGATE(secondCodeUnit)) {
						codePoint = Utils::Unicode::DecodeSurrogatePair(firstCodeUnit, secondCodeUnit);
						++index;
					} else {
						codePoint = replacementCharacter;
					}
				} else {
					codePoint = replacementCharacter;
				}
			} else if (IS_LOW_SURROGATE(firstCodeUnit)) {
				codePoint = replacementCharacter;
			}

			SendCodePoint(codePoint);
		}
	}

	void Manager::SetImeWindowPosition(bool a_isFollowCaret) const
	{
		if (!gameWindow) {
			return;
		}

		const auto position = a_isFollowCaret ? Scaleform::TextInput::GetCurrentCaretPosition(gameWindow) : GetDefaultImeWindowPosition(gameWindow);
		if (!position) {
			return;
		}

		const auto imeContext = ImmGetContext(gameWindow);
		if (!imeContext) {
			return;
		}

		if (NeedSetImeWindowPosition(imeContext, *position)) {
			SetCompositionWindowPosition(imeContext, *position);
			SetCandidateWindowPosition(imeContext, *position);
		}

		ImmReleaseContext(gameWindow, imeContext);
	}

	bool Manager::NeedSetImeWindowPosition(HIMC a_imeContext, POINT a_position) const
	{
		COMPOSITIONFORM composition{};
		if (!ImmGetCompositionWindow(a_imeContext, &composition)) {
			return true;
		}

		if (composition.dwStyle != CFS_FORCE_POSITION ||
			composition.ptCurrentPos.x != a_position.x ||
			composition.ptCurrentPos.y != a_position.y) {
			return true;
		}

		return false;
	}

	void Manager::SetCompositionWindowPosition(HIMC a_imeContext, POINT a_position) const
	{
		COMPOSITIONFORM composition{};
		composition.dwStyle = CFS_FORCE_POSITION;
		composition.ptCurrentPos = a_position;
		ImmSetCompositionWindow(a_imeContext, &composition);
	}

	void Manager::SetCandidateWindowPosition(HIMC a_imeContext, POINT a_position) const
	{
		constexpr std::size_t candidateWindowCount = 4;
		for (std::size_t index = 0; index < candidateWindowCount; ++index) {
			CANDIDATEFORM candidate{};
			candidate.dwIndex = static_cast<DWORD>(index);
			candidate.dwStyle = CFS_CANDIDATEPOS;
			candidate.ptCurrentPos = a_position;
			ImmSetCandidateWindow(a_imeContext, &candidate);
		}
	}

	void Manager::SendCodePoint(std::uint32_t a_codePoint)
	{
		const auto uiQueue = RE::UIMessageQueue::GetSingleton();
		const auto interfaceStrings = RE::InterfaceStrings::GetSingleton();
		const auto factoryManager = RE::MessageDataFactoryManager::GetSingleton();
		if (!uiQueue || !interfaceStrings || !factoryManager) {
			logger::error("Failed to send code point.");
			return;
		}

		const auto scaleformDataCreator = factoryManager->GetCreator<RE::BSUIScaleformData>(interfaceStrings->bsUIScaleformData);
		if (!scaleformDataCreator) {
			logger::error("Failed to create BSTDerivedCreator.");
			return;
		}

		const auto scaleformData = scaleformDataCreator->Create();
		if (!scaleformData) {
			logger::error("Failed to create BSUIScaleformData.");
			return;
		}

		RE::GFxCharEvent& charEvent = charEventPool[charEventIndex];
		charEventIndex = (charEventIndex + 1) % charEventPoolSize;
		charEvent.type = RE::GFxEvent::EventType::kCharEvent;
		charEvent.wcharCode = a_codePoint;
		charEvent.keyboardIndex = 0;
		scaleformData->scaleformEvent = &charEvent;

		uiQueue->AddMessage(interfaceStrings->topMenu, RE::UI_MESSAGE_TYPE::kScaleformEvent, scaleformData);
	}
}
