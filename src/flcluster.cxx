/** ********************************************************
 *
 ***********************************************************/

#include "config.h"

static const char *copyright[] = {
	" =====================================================================",
	"",
	" FLCLUSTER "  VERSION, // flcluster.cxx
	"",
	" Author(s):",
	"    Dave Freese, W1HKJ, Copyright (C) 2017",
	"",
	" This is free software; you can redistribute it and/or modify",
	" it under the terms of the GNU General Public License as published by",
	" the Free Software Foundation; either version 3 of the License, or",
	" (at your option) any later version.",
	"",
	" This software is distributed in the hope that it will be useful,",
	" but WITHOUT ANY WARRANTY; without even the implied warranty of",
	" MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the",
	" GNU General Public License for more details.",
	"",
	" You should have received a copy of the GNU General Public License",
	" along with this program.  If not, see <http://www.gnu.org/licenses/>.",
	"",
	" =====================================================================",
	0
};

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <ctime>
#include <vector>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <pthread.h>
#include <libgen.h>
#include <ctype.h>
#include <sys/time.h>

#include <FL/Fl.H>
#include <FL/Enumerations.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Sys_Menu_Bar.H>
#include <FL/x.H>
#include <FL/Fl_Help_Dialog.H>
#include <FL/Fl_Menu_Item.H>
#include <FL/Fl_File_Icon.H>
#include <FL/Fl_Tooltip.H>

#include "flcluster.h"
#include "dx_dialog.h"
#include "dx_cluster.h"

#include "debug.h"
#include "util.h"
#include "gettext.h"
#include "flinput2.h"
#include "date.h"
#include "calendar.h"
#include "icons.h"
#include "fileselect.h"
#include "status.h"
#include "pixmaps.h"
#include "threads.h"
#include "xml_io.h"

#ifdef WIN32
#  include "flclusterrc.h"
#  include "compat.h"
#endif

#include <FL/filename.H>

#include <FL/x.H>
#include <FL/Fl_Pixmap.H>
#include <FL/Fl_Image.H>

using namespace std;

Fl_Double_Window *main_window = (Fl_Double_Window *)0;

bool dx_stream = false;

//! @brief Command line help string initialization
const char *options[] = {
	"Flcluster Unique Options",
	"",
	"  --help",
	"  --version",
	"  --confg-dir folder-path-name (including drive letter on Windows)",
	"      Windows: C:/Documents and Settings/<username>/folder-name",
	"               C:/Users/<username/folder-name",
	"               H:/hamstuff/folder-name",
	"      Linux:   /home/<username>/folder-name",
	"      OS X:    /home/<username>/folder-name",
	"   --stream enable dx_stream.txt text stream",
	"",
	"  Note: Enclosing \"'s must be used when the path or file name",
	"        contains spaces.",
	"",
	"=======================================================================",
	"",
	"Fltk User Interface options",
	"",
	"  -bg\t-background [COLOR]",
	"  -bg2\t-background2 [COLOR]",
	"  -di\t-display [host:n.n]",
	"  -dn\t-dnd : enable drag and drop",
	"  -nodn\t-nodnd : disable drag and drop",
	"  -fg\t-foreground [COLOR]",
	"  -g\t-geometry [WxH+X+Y]",
	"  -i\t-iconic",
	"  -k\t-kbd : enable keyboard focus:",
	"  -nok\t-nokbd : en/disable keyboard focus",
	"  -na\t-name [CLASSNAME]",
	"  -s\t-scheme [none | gtk+ | plastic]",
	"     default = gtk+",
	"  -ti\t-title [WINDOWTITLE]",
	"  -to\t-tooltips : enable tooltips",
	"  -not\t-notooltips : disable tooltips\n",
	0
};

std::string HOME_DIR = "";
std::string HelpDir = "";
std::string ScriptsDir = "";

#if !defined(__APPLE__) && !defined(__WOE32__) && USE_X
Pixmap  flcluster_icon_pixmap;

#define KNAME "flcluster"

/** ********************************************************
 *
 ***********************************************************/
void make_pixmap(Pixmap *xpm, const char **data)
{
	Fl_Window w(0,0, KNAME);
	w.xclass(KNAME);
	w.show();
	w.make_current();
	Fl_Pixmap icon(data);
	int maxd = (icon.w() > icon.h()) ? icon.w() : icon.h();
	*xpm = fl_create_offscreen(maxd, maxd);
	fl_begin_offscreen(*xpm);
	fl_color(FL_BACKGROUND_COLOR);
	fl_rectf(0, 0, maxd, maxd);
	icon.draw(maxd - icon.w(), maxd - icon.h());
	fl_end_offscreen();
}

#endif

/** ********************************************************
 *
 ***********************************************************/
void checkdirectories(void)
{

	struct DIRS {
		string& dir;
		const char* suffix;
		void (*new_dir_func)(void);
	};

	{
		char dirbuf[FL_PATH_MAX + 1];
#ifdef __WOE32__
		if (HOME_DIR.empty()) {
			fl_filename_expand(dirbuf, sizeof(dirbuf) -1, "$USERPROFILE/");
			HOME_DIR = dirbuf;
			HOME_DIR.append("flcluster.files/");
		}
#else
		if (HOME_DIR.empty()) {
			fl_filename_expand(dirbuf, sizeof(dirbuf) -1, "$HOME/");
			HOME_DIR = dirbuf;
			HOME_DIR.append(".flcluster/");
		}
#endif
	}
	if ((HOME_DIR[HOME_DIR.length()-1] != '/') ||
		(HOME_DIR[HOME_DIR.length()-1] != '/')) HOME_DIR += '/';

	DIRS FLCLUSTER_dirs[] = {
		{ HOME_DIR, 0, 0 },
		{ HelpDir, "help", 0 },
		{ ScriptsDir, "scripts", 0 },
	};

	int r;

	for (size_t i = 0; i < sizeof(FLCLUSTER_dirs)/sizeof(*FLCLUSTER_dirs); i++) {
		if (FLCLUSTER_dirs[i].suffix)
			FLCLUSTER_dirs[i].dir.assign(HOME_DIR).append(FLCLUSTER_dirs[i].suffix).append("/");

		if ((r = mkdir(FLCLUSTER_dirs[i].dir.c_str(), 0777)) == -1 && errno != EEXIST) {
			cerr << _("Could not make directory") << ' ' << FLCLUSTER_dirs[i].dir
			     << ": " << strerror(errno) << '\n';
			exit(EXIT_FAILURE);
		}
		else if (r == 0 && FLCLUSTER_dirs[i].new_dir_func)
			FLCLUSTER_dirs[i].new_dir_func();
	}

}

/** ********************************************************
 *
 ***********************************************************/
double time_f(void)
{
	struct timeval now;
	gettimeofday(&now, NULL);
	return ((double) now.tv_sec) + (((double)now.tv_usec) * 0.000001);
}

/** ********************************************************
 *
 ***********************************************************/
int parse_args(int argc, char **argv, int& idx)
{
	if (strstr(argv[idx], "--config-dir")) {
		std::string temp_dir = "";
		idx++;
		string tmp = argv[idx];
		if (!tmp.empty()) temp_dir = tmp;
		size_t p = string::npos;
		while ( (p = temp_dir.find("\\")) != string::npos)
			temp_dir[p] = '/';
		if (temp_dir[temp_dir.length()-1] != '/')
			temp_dir += '/';
		idx++;
		HOME_DIR.assign(temp_dir);
		return 1;
	}

	if (strcasecmp(argv[idx], "--stream") == 0) {
		dx_stream = true;
		idx++;
		return 1;
	}

	if (strcasecmp(argv[idx], "--help") == 0) {
		int i = 0;
		while (copyright[i] != NULL) {
			printf("%s\n", copyright[i]);
			i++;
		}

		printf("\n\n");
		i = 0;
		while (options[i] != NULL) {
			printf("%s\n", options[i]);
			i++;
		}
		exit (0);
	}

	if (strcasecmp(argv[idx], "--version") == 0) {
		printf("Version: "VERSION"\n");
		exit (0);
	}

	return 0;
}

/** ********************************************************
 *
 ***********************************************************/

static void close_app()
{
	progStatus.saveLastState();
	close_fldigi_xmlrpc();
	exit(0);
}

void exit_main(Fl_Widget *w)
{
	if (Fl::event_key() == FL_Escape)
		return;
	close_app();
}

void cb_Exit()
{
	if (Fl::event_key() == FL_Escape)
		return;
	close_app();
}

/** ********************************************************
 *
 ***********************************************************/

int main(int argc, char *argv[])
{
	int arg_idx;
	if (Fl::args(argc, argv, arg_idx, parse_args) != argc) {
		return 0;
	}

	Fl::lock();
//	Fl::scheme("gtk+");

	checkdirectories();

	progStatus.loadLastState();

	string debug_file = HOME_DIR;
	debug_file.append("debug_log.txt");
	debug::start(debug_file.c_str());

	LOG_INFO("flcluster debug");

	main_window = dxc_window();
	main_window->callback(exit_main);

	Fl_File_Icon::load_system_icons();
	FSEL::create();

#if defined(__WOE32__)
#  ifndef IDI_ICON
#	define IDI_ICON 101
#  endif
	main_window->icon((char*)LoadIcon(fl_display, MAKEINTRESOURCE(IDI_ICON)));
	main_window->show (argc, argv);
#elif !defined(__APPLE__)
	make_pixmap(&flcluster_icon_pixmap, flcluster_icon);
	main_window->icon((char *)flcluster_icon_pixmap);
	main_window->show(argc, argv);
#else
	main_window->show(argc, argv);
#endif

	string main_label = PACKAGE_NAME;
	main_label.append(": ").append(PACKAGE_VERSION);
	main_window->label(main_label.c_str());

	main_window->resize( progStatus.mainX, progStatus.mainY, progStatus.mainW, progStatus.mainH);

	open_fldigi_xmlrpc();

	DXcluster_init();

	Fl_Tooltip::enable(progStatus.tooltips);

	return Fl::run();
}

/** ********************************************************
 *
 ***********************************************************/
void open_url(const char* url)
{
	LOG_INFO("%s", url);
#ifndef __WOE32__
	const char* browsers[] = {
#  ifdef __APPLE__
		getenv("FLDIGI_BROWSER"),   // valid for any OS - set by user
		"open"                      // OS X
#  else
		"fl-xdg-open",			    // Puppy Linux
		"xdg-open",			        // other Unix-Linux distros
		getenv("FLDIGI_BROWSER"),   // force use of spec'd browser
		getenv("BROWSER"),		    // most Linux distributions
		"sensible-browser",
		"firefox",
		"mozilla"				    // must be something out there!
#  endif
	};
	switch (fork()) {
		case 0:
#  ifndef NDEBUG
			unsetenv("MALLOC_CHECK_");
			unsetenv("MALLOC_PERTURB_");
#  endif
			for (size_t i = 0; i < sizeof(browsers)/sizeof(browsers[0]); i++)
				if (browsers[i])
					execlp(browsers[i], browsers[i], url, (char*)0);
			exit(EXIT_FAILURE);
		case -1:
			fl_alert2(_("Could not run a web browser:\n%s\n\n"
						"Open this URL manually:\n%s"),
					  strerror(errno), url);
	}
#else
	if ((int)ShellExecute(NULL, "open", url, NULL, NULL, SW_SHOWNORMAL) <= 32)
		fl_alert2(_("Could not open url:\n%s\n"), url);
#endif
}

/** ********************************************************
 *
 ***********************************************************/
void show_help()
{
//	open_url("http://www.w1hkj.com/flcluster-help/index.html");
}

/** ********************************************************
 *
 ***********************************************************/
void cb_folders()
{
	open_url(HOME_DIR.c_str());
}

/** ********************************************************
 *
 ***********************************************************/
void url_to_file(char *path, size_t buffer_length)
{
	if(!path && buffer_length < 1) return;

	char *convert_buffer = (char *)0;
	char *cPtr = path;
	char *cEnd = &path[buffer_length];
	char *dPtr = (char *)0;
	int value = 0;
	int count = 0;

	convert_buffer = new char [buffer_length + 1];
	if(!convert_buffer) return;

	memset(convert_buffer, 0, buffer_length + 1);
	dPtr = convert_buffer;

	while(cPtr < cEnd) {
		if(*cPtr == '%') {
			sscanf(cPtr + 1, "%02x", &value);
			*dPtr++ = (char) value;
			cPtr += 3;
		} else {
			*dPtr++ = *cPtr++;
		}
		count++;
	}

	memset(path, 0, buffer_length);
	memcpy(path, convert_buffer, count);

	delete [] convert_buffer;
}

