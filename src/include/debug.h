// ----------------------------------------------------------------------------
//	  debug.h
//
// Copyright (C) 2008, 2012
//			  Stelios Bounanos, M0GLD; Dave Freese, W1HKJ
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

#ifndef _DEBUG_H_
#define _DEBUG_H_

#include "util.h"

class debug
{
public:
	enum level_e { QUIET_LEVEL, ERROR_LEVEL, WARN_LEVEL, INFO_LEVEL, DEBUG_LEVEL, LOG_NLEVELS };
	enum source_e { LOG_CONTROL = 1 << 0, LOG_OTHER = 1 << 1 };

	static void start(const char* filename);
	static void log(level_e level, const char* func, const char* srcf, int line,
					const char* format, ...) format__(printf, 5, 6);
	static level_e level;
	static uint32_t mask;
private:
	debug(const char* filename);
	debug(const debug&);
	debug& operator=(const debug&);
	~debug();
	static debug* inst;
};

#define LOG(level__, source__, ...)							\
do {										\
if (level__ <= debug::level && source__ & debug::mask)			\
debug::log(level__, __func__, __FILE__, __LINE__, __VA_ARGS__); \
} while (0)

#define LOG_DEBUG(...) LOG(debug::DEBUG_LEVEL, log_source_, __VA_ARGS__)
#define LOG_INFO(...) LOG(debug::INFO_LEVEL, log_source_, __VA_ARGS__)
#define LOG_WARN(...) LOG(debug::WARN_LEVEL, log_source_, __VA_ARGS__)
#define LOG_ERROR(...) LOG(debug::ERROR_LEVEL, log_source_, __VA_ARGS__)
#define LOG_QUIET(...) LOG(debug::QUIET_LEVEL, log_source_, __VA_ARGS__)

unused__ static uint32_t log_source_ = debug::LOG_OTHER;

#if defined(__GNUC__) && (__GNUC__ >= 3)
#  define LOG_FILE_SOURCE(source__)						\
__attribute__((constructor))						\
static void log_set_source_(void) { log_source_ = source__; }
#else
#  define LOG_FILE_SOURCE(source__)
#endif

#define LOG_SET_SOURCE(source__) log_source_ = source__

#endif // _DEBUG_H_

// Local Variables:
// mode: c++
// c-file-style: "linux"
// End:
