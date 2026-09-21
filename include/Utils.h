#pragma once

namespace Utils
{
	namespace String
	{
		[[nodiscard]] std::string ToLower(std::string_view a_value);
		[[nodiscard]] bool CompareNoCase(std::string_view a_left, std::string_view a_right);
		[[nodiscard]] bool LessNoCase(std::string_view a_left, std::string_view a_right);
	}

	namespace Charset
	{
		namespace Unicode
		{
			[[nodiscard]] std::uint32_t DecodeSurrogatePair(std::uint16_t a_highSurrogate, std::uint16_t a_lowSurrogate);
			[[nodiscard]] bool IsTextCodePoint(std::uint32_t a_code);
		}
	}

	namespace Encoding
	{
		namespace UTF8
		{
			[[nodiscard]] bool IsValidContinuation(std::uint8_t a_charByte, std::size_t a_byteIndex, std::uint8_t a_firstByte);
		}

		[[nodiscard]] std::size_t GetCharByteCount(UINT a_codePage, std::uint8_t a_firstByte);
	}

	namespace DirectInput
	{
		[[nodiscard]] std::uint32_t GetKeyCodeFromKeyData(std::uint32_t a_keyData);
		[[nodiscard]] bool IsKeyPressed(std::uint8_t a_keyCode);
		[[nodiscard]] bool WasKeyPressed(std::uint8_t a_keyCode);
		[[nodiscard]] bool IsTextInputModifierKey(std::uint32_t a_keyCode);
		[[nodiscard]] bool IsTextInputCharacterKey(std::uint32_t a_keyCode);
	}

	namespace ActionScript
	{
		[[nodiscard]] std::optional<std::string> GetStringMember(const RE::GFxValue& a_object, const char* a_name);
		[[nodiscard]] std::optional<double> GetNumberMember(const RE::GFxValue& a_object, const char* a_name);
		[[nodiscard]] std::optional<float> GetFloatMember(const RE::GFxValue& a_object, const char* a_name);
		[[nodiscard]] std::optional<std::uint32_t> GetUint32Member(const RE::GFxValue& a_object, const char* a_name);
	}
	
	namespace Game
	{
		[[nodiscard]] bool IsAllowTextInput();
		[[nodiscard]] std::uint32_t GetConsoleKeyCode();
	}
}
