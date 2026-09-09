#pragma once

inline void SetupLog()
{
	const auto logFolder = SKSE::log::log_directory();
	if (!logFolder) {
		SKSE::stl::report_and_fail("Failed to find standard logging directory."sv);
	}

	const auto pluginName = SKSE::PluginDeclaration::GetSingleton()->GetName();
	const auto logPath = *logFolder / std::format("{}.log", pluginName);
	auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logPath.string(), true);
	auto log = std::make_shared<spdlog::logger>("log", std::move(sink));

	spdlog::set_default_logger(std::move(log));
	spdlog::set_pattern("[%Y-%m-%d %H:%M:%S] [%l] %v");

#ifdef NDEBUG
	spdlog::set_level(spdlog::level::info);
	spdlog::flush_on(spdlog::level::info);
#else
	spdlog::set_level(spdlog::level::trace);
	spdlog::flush_on(spdlog::level::trace);
#endif
}
