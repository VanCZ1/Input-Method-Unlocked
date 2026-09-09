#include "Utils.h"

namespace Utils::String
{
	std::string ToLower(std::string_view a_str)
	{
		std::string result(a_str);
		std::ranges::transform(result, result.begin(), [](const unsigned char ch) {
			return static_cast<unsigned char>(std::tolower(ch));
		});

		return result;
	}

	bool CompareNoCase(std::string_view a_lhs, std::string_view a_rhs)
	{
		if (a_lhs.size() != a_rhs.size()) {
			return false;
		}

		return std::equal(a_lhs.begin(), a_lhs.end(), a_rhs.begin(), [](const unsigned char a, const unsigned char b) {
			return std::tolower(a) == std::tolower(b);
		});
	}

	bool LessNoCase(std::string_view a_left, std::string_view a_right)
	{
		const auto size = std::min(a_left.size(), a_right.size());
		for (std::size_t i = 0; i < size; ++i) {
			const auto left = std::tolower(static_cast<unsigned char>(a_left[i]));
			const auto right = std::tolower(static_cast<unsigned char>(a_right[i]));
			if (left != right) {
				return left < right;
			}
		}

		return a_left.size() < a_right.size();
	}
}

namespace Utils::Input
{
	namespace
	{
		constexpr std::uint32_t kScanCodeShift = 16;
		constexpr std::uint32_t kScanCodeMask = 0xFF;
		constexpr std::uint32_t kExtendedFlagShift = 24;
		constexpr std::uint32_t kExtendedFlagMask = 0x01;

		constexpr std::uint32_t kDirectInputExtendedFlag = 0x80;
		constexpr std::uint8_t kDirectInputPressedMask = 0x80;
	}

	std::uint32_t GetDirectInputKeyCodeFromKeyData(std::uint32_t a_keyData)
	{
		auto keyCode = static_cast<std::uint32_t>((a_keyData >> kScanCodeShift) & kScanCodeMask);
		const auto isExtended = ((a_keyData >> kExtendedFlagShift) & kExtendedFlagMask) != 0;
		if (isExtended) {
			keyCode |= kDirectInputExtendedFlag;
		}

		return keyCode;
	}

	bool WasDirectInputKeyPressed(std::uint8_t a_keyCode)
	{
		bool result = false;
		if (const auto inputDeviceManager = RE::BSInputDeviceManager::GetSingleton()) {
			if (const auto keyboard = inputDeviceManager->GetKeyboard()) {
				const auto& prevState = keyboard->GetRuntimeData().prevState;
				result = (prevState[a_keyCode] & kDirectInputPressedMask) != 0;
			}
		}

		return result;
	}

	bool IsTextInputModifierKey(std::uint32_t a_keyCode)
	{
		// 102-key keyboard
		if ((a_keyCode == DIK_LCONTROL) || (a_keyCode == DIK_RCONTROL) ||
			(a_keyCode == DIK_LSHIFT) || (a_keyCode == DIK_RSHIFT) ||
			(a_keyCode == DIK_LMENU) || (a_keyCode == DIK_RMENU) ||
			(a_keyCode == DIK_CAPITAL) || ((a_keyCode == DIK_NUMLOCK))) {
			return true;
		}

		// other key
		switch (a_keyCode) {
		case DIK_KANA:
		case DIK_CONVERT:
		case DIK_NOCONVERT:
		case DIK_KANJI:
			return true;
		default:
			return false;
		}
	}

	bool IsTextInputCharacterKey(std::uint32_t a_keyCode)
	{
		// 102-key keyboard
		if ((a_keyCode >= DIK_1 && a_keyCode <= DIK_EQUALS) ||
			(a_keyCode >= DIK_Q && a_keyCode <= DIK_RBRACKET) ||
			(a_keyCode >= DIK_A && a_keyCode <= DIK_GRAVE) ||
			(a_keyCode >= DIK_BACKSLASH && a_keyCode <= DIK_SLASH) ||
			(a_keyCode == DIK_MULTIPLY) ||
			(a_keyCode == DIK_SPACE) ||
			(a_keyCode >= DIK_NUMPAD7 && a_keyCode <= DIK_DECIMAL)) {
			return true;
		}

		// other key
		switch (a_keyCode) {
		case DIK_OEM_102:
		case DIK_ABNT_C1:
		case DIK_YEN:
		case DIK_ABNT_C2:
		case DIK_NUMPADEQUALS:
		case DIK_AT:
		case DIK_COLON:
		case DIK_UNDERLINE:
		case DIK_NUMPADCOMMA:
		case DIK_DIVIDE:
			return true;
		default:
			return false;
		}
	}
}

namespace Utils::Unicode
{
	namespace
	{
		constexpr std::uint32_t kHighSurrogateStart = 0xD800;
		constexpr std::uint32_t kHighSurrogateEnd = 0xDBFF;
		constexpr std::uint32_t kLowSurrogateStart = 0xDC00;
		constexpr std::uint32_t kLowSurrogateEnd = 0xDFFF;
		constexpr std::uint32_t kSupplementaryPlaneStart = 0x10000;

		constexpr std::uint32_t kMaxUnicodeCodePoint = 0x10FFFF;
		constexpr std::uint32_t kC0ControlEnd = 0x1F;
		constexpr std::uint32_t kDeleteCharacter = 0x7F;
		constexpr std::uint32_t kC1ControlStart = 0x80;
		constexpr std::uint32_t kC1ControlEnd = 0x9F;
	}

	std::uint32_t DecodeSurrogatePair(std::uint16_t a_highSurrogate, std::uint16_t a_lowSurrogate)
	{
		const auto highOffset = static_cast<std::uint32_t>(a_highSurrogate) - kHighSurrogateStart;
		const auto lowOffset = static_cast<std::uint32_t>(a_lowSurrogate) - kLowSurrogateStart;

		return kSupplementaryPlaneStart + (highOffset << 10) + lowOffset;
	}

	bool IsTextCodePoint(std::uint32_t a_code)
	{
		if (a_code > kMaxUnicodeCodePoint ||
			a_code <= kC0ControlEnd || a_code == kDeleteCharacter ||
			(a_code >= kC1ControlStart && a_code <= kC1ControlEnd) ||
			(a_code >= kHighSurrogateStart && a_code <= kLowSurrogateEnd)) {
			return false;
		}

		return true;
	}
}

namespace Utils::ActionScript
{
	std::optional<std::string> GetStringMember(const RE::GFxValue& a_object, const char* a_name)
	{
		RE::GFxValue value;
		if (!a_object.GetMember(a_name, &value) || !value.IsString()) {
			return std::nullopt;
		}

		const auto str = value.GetString();
		return str ? std::optional<std::string>(str) : std::nullopt;
	}

	std::optional<double> GetNumberMember(const RE::GFxValue& a_object, const char* a_name)
	{
		RE::GFxValue value;
		if (!a_object.GetMember(a_name, &value) || !value.IsNumber()) {
			return std::nullopt;
		}

		const auto number = value.GetNumber();
		return std::isfinite(number) ? std::optional<double>(number) : std::nullopt;
	}

	std::optional<float> GetFloatMember(const RE::GFxValue& a_object, const char* a_name)
	{
		const auto number = GetNumberMember(a_object, a_name);
		if (!number || *number < std::numeric_limits<float>::lowest() ||
			*number > std::numeric_limits<float>::max()) {
			return std::nullopt;
		}

		return static_cast<float>(*number);
	}

	std::optional<std::uint32_t> GetUint32Member(const RE::GFxValue& a_object, const char* a_name)
	{
		const auto number = GetNumberMember(a_object, a_name);
		if (!number || std::trunc(*number) != *number ||
			*number < std::numeric_limits<std::uint32_t>::min() ||
			*number > std::numeric_limits<std::uint32_t>::max()) {
			return std::nullopt;
		}

		return static_cast<std::uint32_t>(*number);
	}
}

namespace Utils::Game
{
	bool IsAllowTextInput()
	{
		const auto controlMap = RE::ControlMap::GetSingleton();
		if (!controlMap) {
			return false;
		}

		return controlMap->GetRuntimeData().textEntryCount > 0;
	}

	std::uint32_t GetConsoleKeyCode()
	{
		const auto controlMap = RE::ControlMap::GetSingleton();
		if (!controlMap) {
			return RE::ControlMap::kInvalid;
		}

		return controlMap->GetMappedKey(
			"Console",
			RE::INPUT_DEVICE::kKeyboard,
			RE::ControlMap::InputContextID::kGameplay);
	}
}
