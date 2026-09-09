#pragma once

namespace RE
{
	class GFxCharEvent : public GFxEvent
	{
	public:
		inline GFxCharEvent() :
			GFxEvent(),
			wcharCode(0),
			keyboardIndex(0)
		{}

		inline GFxCharEvent(EventType a_eventType, std::uint32_t a_wcharCode, std::uint32_t a_keyboardIndex = 0) :
			GFxEvent(a_eventType),
			wcharCode(a_wcharCode),
			keyboardIndex(a_keyboardIndex)
		{}

		// members
		std::uint32_t wcharCode;      // 04
		std::uint32_t keyboardIndex;  // 08
	};
	static_assert(sizeof(GFxCharEvent) == 0x0C);
}
