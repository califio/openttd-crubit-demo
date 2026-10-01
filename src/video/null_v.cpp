/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file null_v.cpp The video driver that doesn't blit. */

#include "../stdafx.h"
#include "../gfx_func.h"
#include "../blitter/factory.hpp"
#include "../saveload/saveload.h"
#include "../window_func.h"
#include "null_v.h"

#ifdef CALIF_ICU4X_DEMO
#include <fstream>
#include "../string_func.h"
extern bool CalifWordDemoTick(uint tick);
#endif

#include "../safeguards.h"

/** Factory for the null video driver. */
static FVideoDriver_Null iFVideoDriver_Null;

std::optional<std::string_view> VideoDriver_Null::Start(const StringList &parm)
{
#ifdef _MSC_VER
	/* Disable the MSVC assertion message box. */
	_set_error_mode(_OUT_TO_STDERR);
	_CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
#endif

	this->UpdateAutoResolution();

	this->ticks = GetDriverParamInt(parm, "ticks", 1000);
	_screen.width  = _screen.pitch = _cur_resolution.width;
	_screen.height = _cur_resolution.height;
	_screen.dst_ptr = nullptr;
	ScreenSizeChanged();

#ifdef CALIF_ICU4X_DEMO
	this->demo_render = GetDriverParamBool(parm, "render");
	if (this->demo_render) {
		BlitterFactory::SelectBlitter("32bpp-simple");
		this->demo_pixels.resize(_screen.width * _screen.height);
		_screen.dst_ptr = this->demo_pixels.data();
		return std::nullopt;
	}
#endif
	/* Do not render, nor blit */
	Debug(misc, 1, "Forcing blitter 'null'...");
	BlitterFactory::SelectBlitter("null");
	return std::nullopt;
}

void VideoDriver_Null::Stop() { }

void VideoDriver_Null::MakeDirty(int, int, int, int) {}

void VideoDriver_Null::MainLoop()
{
	uint i;

	for (i = 0; i < this->ticks; i++) {
		::GameLoop();
		::InputLoop();
#ifdef CALIF_ICU4X_DEMO
		bool capture = this->demo_render && CalifWordDemoTick(i);
#endif
		::UpdateWindows();
#ifdef CALIF_ICU4X_DEMO
		if (capture) {
			DrawDirtyBlocks();
			auto dir = GetEnv("CALIF_RECORD_DIR").value_or(".");
			std::ofstream out(fmt::format("{}/frame-{:03}.ppm", dir, i), std::ios::binary);
			out << "P6\n" << _screen.width << " " << _screen.height << "\n255\n";
			for (const auto &pixel : this->demo_pixels) {
				out.put(pixel.r); out.put(pixel.g); out.put(pixel.b);
			}
		}
#endif
	}

	/* If requested, make a save just before exit. The normal exit-flow is
	 * not triggered from this driver, so we have to do this manually. */
	if (_settings_client.gui.autosave_on_exit) {
		DoExitSave();
	}
}

bool VideoDriver_Null::ChangeResolution(int, int) { return false; }

bool VideoDriver_Null::ToggleFullscreen(bool) { return false; }
