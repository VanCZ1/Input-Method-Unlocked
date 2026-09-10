#include "Scaleform.h"
#include "Utils.h"

namespace Scaleform::TextInput
{
	namespace
	{
		std::optional<RE::GRectF> GetCharacterRectangle(RE::GFxValue& a_textField, std::uint32_t a_index)
		{
			const std::array args{ RE::GFxValue(static_cast<double>(a_index)) };
			RE::GFxValue boundaries;
			if (!a_textField.Invoke("getCharBoundaries", &boundaries, args) || !boundaries.IsObject()) {
				return std::nullopt;
			}

			const auto x = Utils::ActionScript::GetFloatMember(boundaries, "x");
			const auto y = Utils::ActionScript::GetFloatMember(boundaries, "y");
			const auto width = Utils::ActionScript::GetFloatMember(boundaries, "width");
			const auto height = Utils::ActionScript::GetFloatMember(boundaries, "height");
			if (!x || !y || !width || !height) {
				return std::nullopt;
			}

			return RE::GRectF{
				.left = *x,
				.top = *y,
				.right = *x + *width,
				.bottom = *y + *height
			};
		}

		std::optional<RE::GPointF> GetEmptyTextCaretPosition(RE::GFxValue& a_textField)
		{
			const std::array args{ RE::GFxValue(0.0) };
			RE::GFxValue lineMetrics;
			if (!a_textField.Invoke("getLineMetrics", &lineMetrics, args) || !lineMetrics.IsObject()) {
				return std::nullopt;
			}

			const auto x = Utils::ActionScript::GetFloatMember(lineMetrics, "x");
			const auto height = Utils::ActionScript::GetFloatMember(lineMetrics, "height");
			if (!x || !height) {
				return std::nullopt;
			}

			float y = *height;
			if (const auto fieldHeight = Utils::ActionScript::GetFloatMember(a_textField, "_height"); fieldHeight && *fieldHeight > *height) {
				if (const auto alignment = Utils::ActionScript::GetStringMember(a_textField, "verticalAlign")) {
					if (Utils::String::CompareNoCase(*alignment, "bottom")) {
						y = *fieldHeight;
					} else if (Utils::String::CompareNoCase(*alignment, "center")) {
						y = std::midpoint(*fieldHeight, *height);
					}
				}
			}

			return RE::GPointF{
				.x = *x,
				.y = y
			};
		}

		std::optional<RE::GPointF> GetLocalCaretPosition(RE::GFxValue& a_textField)
		{
			const auto caretIndex = Utils::ActionScript::GetUint32Member(a_textField, "caretIndex");
			const auto textLength = Utils::ActionScript::GetUint32Member(a_textField, "length");
			if (!caretIndex || !textLength || *caretIndex > *textLength) {
				return std::nullopt;
			}

			RE::GPointF caret{};
			if (*textLength == 0) {
				const auto lineStart = GetEmptyTextCaretPosition(a_textField);
				if (!lineStart) {
					return std::nullopt;
				}

				caret = *lineStart;
			} else if (*caretIndex == *textLength) {
				const auto previous = GetCharacterRectangle(a_textField, *caretIndex - 1);
				if (!previous) {
					return std::nullopt;
				}

				caret = {
					.x = previous->right,
					.y = previous->bottom
				};
			} else {
				const auto current = GetCharacterRectangle(a_textField, *caretIndex);
				if (current) {
					caret = {
						.x = current->left,
						.y = current->bottom
					};
				} else {
					if (*caretIndex > 0) {
						const auto previous = GetCharacterRectangle(a_textField, *caretIndex - 1);
						if (!previous) {
							return std::nullopt;
						}

						caret = {
							.x = previous->right,
							.y = previous->bottom
						};
					} else {
						const auto lineStart = GetEmptyTextCaretPosition(a_textField);
						if (!lineStart) {
							return std::nullopt;
						}

						caret = *lineStart;
					}
				}
			}

			if (const auto hScroll = Utils::ActionScript::GetFloatMember(a_textField, "hscroll")) {
				caret.x -= *hScroll;
			}

			return caret;
		}

		std::optional<POINT> GetGlobalCaretPosition(const RE::GPointF& a_localCaret, const RE::GPtr<RE::GFxMovieView>& a_movieView, const char* a_focusPath, const RECT& a_client)
		{
			RE::GRenderer::Matrix userMatrix;
			RE::GPointF globalCaret{};
			if (!a_movieView->TranslateLocalToScreen(a_focusPath, a_localCaret, &globalCaret, &userMatrix) ||
				!std::isfinite(globalCaret.x) || !std::isfinite(globalCaret.y)) {
				return std::nullopt;
			}

			const auto minX = static_cast<float>(a_client.left);
			const auto minY = static_cast<float>(a_client.top);
			const auto maxX = static_cast<float>(a_client.right - 1);
			const auto maxY = static_cast<float>(a_client.bottom - 1);

			const auto x = static_cast<LONG>(std::lround(std::clamp(globalCaret.x, minX, maxX)));
			const auto y = static_cast<LONG>(std::lround(std::clamp(globalCaret.y, minY, maxY)));

			return POINT{
				.x = x,
				.y = y
			};
		}

		std::optional<POINT> GetCurrentMenuCaretPosition(const RE::GPtr<RE::GFxMovieView>& a_movieView, const RECT& a_client)
		{
			if (!a_movieView) {
				return std::nullopt;
			}

			RE::GFxValue focusPath;
			if (!a_movieView->Invoke("Selection.getFocus", &focusPath, nullptr, 0) || !focusPath.IsString()) {
				return std::nullopt;
			}

			const auto focusPathStr = focusPath.GetString();
			if (!focusPathStr || *focusPathStr == '\0') {
				return std::nullopt;
			}

			RE::GFxValue textField;
			if (!a_movieView->GetVariable(&textField, focusPathStr) || !textField.IsObject()) {
				return std::nullopt;
			}

			const auto localCaret = GetLocalCaretPosition(textField);
			if (!localCaret) {
				return std::nullopt;
			}

			return GetGlobalCaretPosition(*localCaret, a_movieView, focusPathStr, a_client);
		}
	}

	std::optional<POINT> GetCurrentCaretPosition(HWND a_hWnd)
	{
		if (!a_hWnd) {
			return std::nullopt;
		}

		const auto ui = RE::UI::GetSingleton();
		if (!ui) {
			return std::nullopt;
		}

		RECT client{};
		if (!GetClientRect(a_hWnd, &client) || client.right <= client.left || client.bottom <= client.top) {
			return std::nullopt;
		}

		for (auto index = ui->menuStack.size(); index > 0; --index) {
			const auto& menu = ui->menuStack[index - 1];
			if (!menu || !menu->uiMovie) {
				continue;
			}

			if (const auto caretPosition = GetCurrentMenuCaretPosition(menu->uiMovie, client)) {
				return caretPosition;
			}
		}

		return std::nullopt;
	}
}
