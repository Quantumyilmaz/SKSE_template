#pragma once

#include "RE/Skyrim.h"
#include "SKSE/SKSE.h"



#include <shared_mutex>

#include "RE/Skyrim.h"
#include "SKSE/SKSE.h"

#ifdef NDEBUG
#	include <spdlog/sinks/basic_file_sink.h>
#else
#	include <spdlog/sinks/msvc_sink.h>
#endif
#include <xbyak/xbyak.h>
#include <srell.hpp>

#include "ClibUtil/editorID.hpp"
#include "ClibUtil/simpleINI.hpp"
#include "ClibUtil/singleton.hpp"
#include "ClibUtil/string.hpp"

using namespace std::literals;