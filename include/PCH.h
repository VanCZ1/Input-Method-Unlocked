#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include "RE/Skyrim.h"
#include "SKSE/SKSE.h"
#include "REX/REX/Singleton.h"

#include <spdlog/sinks/basic_file_sink.h>

using namespace std::literals;
namespace logger = SKSE::log;

#define DIRECTINPUT_VERSION 0x0800
#include <Windows.h>
#include <dinput.h>
#include <imm.h>

#pragma comment(lib, "imm32.lib")
#pragma comment(lib, "dxguid.lib")

#include "Version.h"
