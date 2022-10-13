#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")
#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")
using namespace Gdiplus;
#include <comdef.h>

#include <tchar.h>
#include <strsafe.h>

#include <algorithm>
#include <memory>
#include <vector>

#include "AviUtl/aviutl_exedit_sdk/aviutl.hpp"
#include "AviUtl/aviutl_exedit_sdk/exedit.hpp"
#include "Common/Tracer.h"
#include "Common/Hook.h"
#include "Common/AviUtlInternal.h"
