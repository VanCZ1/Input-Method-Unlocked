#include "Log.h"
#include "Hooks.h"

namespace
{
	void Load()
	{
		Hooks::InstallAtLoad();
	}

	void MessageHandler(SKSE::MessagingInterface::Message* a_msg)
	{
		switch (a_msg->type) {
		case SKSE::MessagingInterface::kDataLoaded:
			break;
		case SKSE::MessagingInterface::kPostLoad:
			{
				Hooks::InstallAtPostLoad();
				break;
			}
		case SKSE::MessagingInterface::kPostPostLoad:
			break;
		case SKSE::MessagingInterface::kInputLoaded:
			{
				Hooks::InstallAtInputLoaded();
				break;
			}
		case SKSE::MessagingInterface::kPreLoadGame:
			break;
		case SKSE::MessagingInterface::kPostLoadGame:
			break;
		}
	}
}

SKSEPluginInfo(
	.Version = REL::Version{ Version::MAJOR, Version::MINOR, Version::PATCH },
	.Name = Version::PROJECT,
	.Author = "VanCZ1"sv,
	.SupportEmail = ""sv,
	.StructCompatibility = SKSE::StructCompatibility::Independent,
	.RuntimeCompatibility = SKSE::VersionIndependence::AddressLibrary
)

SKSEPluginLoad(const SKSE::LoadInterface* a_skse)
{
	SKSE::Init(a_skse);

	SetupLog();
	const auto plugin = SKSE::PluginDeclaration::GetSingleton();
	logger::info("{} v{}", plugin->GetName(), plugin->GetVersion());
	const auto gameVersion = a_skse->RuntimeVersion().string();
	logger::info("Game version: {}", gameVersion);

	const auto messaging = SKSE::GetMessagingInterface();
	if (!messaging->RegisterListener("SKSE", MessageHandler)) {
		return false;
	}
	Load();

	return true;
}
