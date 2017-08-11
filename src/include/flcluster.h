// =====================================================================
//
// flcluster.h
//
// Author: Dave Freese, W1HKJ
// Copyright: 2017
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
// =====================================================================

#ifndef FLCLUSTER_H
#define FLCLUSTER_H

#include <string>
#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Double_Window.H>

#include "status.h"

extern std::string HOME_DIR;
extern std::string HelpDir;
extern std::string ScriptsDir;

extern Fl_Double_Window *main_window;
extern void cb_Exit();

extern void show_help();
extern void cb_folders();
extern void url_to_file(char *path, size_t buffer_length);
extern void open_url(const char* url);


#endif
