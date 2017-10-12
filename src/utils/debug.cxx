// ----------------------------------------------------------------------------
//      debug.cxx
//
// Copyright (C) 2008, 2012
//              Stelios Bounanos, M0GLD, Dave Freese, W1HKJ
//
//
// This file is part of FLCLUSTER.
//
// This is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// This software is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
// ----------------------------------------------------------------------------

#include <cstdio>
#include <cstring>
#include <cstdarg>
#include <string>
#include <iostream>
#include <fstream>

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <pthread.h>

#include <queue>

#include <sys/time.h>

#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Slider.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Menu_Item.H>
#include <FL/Fl_Menu_Button.H>

#include <FL/Fl_Browser.H>

#include "debug.h"
#include "icons.h"
#include "gettext.h"
#include "flcluster.h"

#include "threads.h"

#ifdef __MINGW32__
#include "timeops.h"
#endif

using namespace std;

#define MAX_LINES 65536

static queue<string>dbg_buffers;

debug* debug::inst = 0;

//debug::level_e debug::level = debug::INFO_LEVEL;
debug::level_e debug::level = debug::INFO_LEVEL;
uint32_t debug::mask = ~0u;

const char* prefix[] = { _("Quiet"), _("Error"), _("Warning"), _("Info"), _("Debug") };

pthread_mutex_t mutex_log = PTHREAD_MUTEX_INITIALIZER;

/** ********************************************************
 *
 ***********************************************************/
int strlen_n(char *buf, size_t limit)
{
	size_t index = 0;
	int count = 0;
	int value = 0;

	if(limit < 1) return 0;
	if(!buf) return 0;

	for(index = 0; index < limit; index++) {
		value = *buf++;
		if(value == 0) break;
		count++;
	}
	return count;
}

/** ********************************************************
 *
 ***********************************************************/
static string fname;

void debug::start(const char* filename)
{
	if (debug::inst)
		return;
	fname = filename;
	debug::inst = new debug(fname.c_str());

	while (!dbg_buffers.empty()) dbg_buffers.pop();
}

/** ********************************************************
 *
 ***********************************************************/
//void debug::stop(void)
//{
//}

static char fmt[1024];
static char sztemp[1024];

/** ********************************************************
 *
 ***********************************************************/
static void append_to_file(void *)
{
	guard_lock gl (&mutex_log);

	if (fname.empty()) {
		while (!dbg_buffers.empty()) {
			std::cout << dbg_buffers.front();
			dbg_buffers.pop();
		}
		return;
	}

	ofstream afile(fname.c_str(), ios::app);
	if (!afile) {
		while (!dbg_buffers.empty()) dbg_buffers.pop();
		return;
	}

	while (!dbg_buffers.empty()) {
		afile << dbg_buffers.front();
		std::cout << dbg_buffers.front();
		dbg_buffers.pop();
	}
}

/** ********************************************************
 *
 ***********************************************************/
static char ztbuf[20] = "12:30:00";

void debug::log(level_e level, const char* func, const char* srcf, int line, const char* format, ...)
{
	if (!inst)
		return;

	guard_lock gl (&mutex_log);

	struct tm tm;
	time_t t_temp;
	struct timeval tv;

	gettimeofday(&tv, NULL);

	t_temp=(time_t)tv.tv_sec;
	gmtime_r(&t_temp, &tm);
	memset(ztbuf, 0, sizeof(ztbuf));
	strftime(ztbuf, sizeof(ztbuf), "%H:%M:%S", &tm);

	snprintf(fmt, sizeof(fmt), "%s [%c] %s: %s\n", ztbuf, *prefix[level], func, format);

	va_list args;
	va_start(args, format);

	vsnprintf(sztemp, sizeof(sztemp), fmt, args);

	va_end(args);

	dbg_buffers.push(sztemp);

	Fl::awake(append_to_file);
}

/** ********************************************************
 *
 ***********************************************************/
debug::debug(const char* filename)
{
	ofstream wfile(filename, ios::out);
	if (!wfile) throw strerror(errno);
}

/** ********************************************************
 *
 ***********************************************************/
debug::~debug()
{
}
