// =====================================================================
//
// dxcluster.cxx
//
// Copyright (C) 2016
//		Dave Freese, W1HKJ
//
// This file is part of FLCLUSTER.
//
// Fldigi is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Fldigi is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with FLCLUSTER.  If not, see <http://www.gnu.org/licenses/>.
// =====================================================================

#include <iostream>
#include <cmath>
#include <cstring>
#include <queue>
#include <sys/types.h>
#ifndef __WIN32__
#include <sys/socket.h>
#include <netinet/in.h>
#else
#include "compat.h"
#endif

#include <stdlib.h>

#include <FL/Fl.H>
#include <FL/Fl_Text_Display.H>
#include <FL/Fl_Text_Buffer.H>

#include "flcluster.h"
#include "status.h"
#include "debug.h"
#include "icons.h"
#include "socket.h"
#include "threads.h"
#include "gettext.h"
#include "timeops.h"

#include "xml_io.h"

#include "fileselect.h"

#include "dx_dialog.h"
#include "dx_cluster.h"

using namespace std;

void abort_connection();
void restart_connection(void *);

int connection_timeout = 0;

static char zdbuf[9] = "20120602";
static char ztbuf[11] = "12:30:00 Z";

/// write stream i/o to external text file 'dx_stream.txt'
static void write_dxc_debug(char c, string txt)
{
	if (!dx_stream) return;
	string pname = HOME_DIR;
	pname.append("dx_stream.txt");
	FILE *dxcdebug = fl_fopen(pname.c_str(), "a");
	fprintf(dxcdebug, "%s [%c]:%s", ztbuf, c, txt.c_str());
	fclose(dxcdebug);
}

// =====================================================================
/// TOD_clock
/// separate thread used to maintain time synched to system clock
// =====================================================================

static pthread_t TOD_thread;

static unsigned long  _zmsec = 0;

const unsigned long zmsec(void)
{
	return _zmsec;
}

const char* zdate(void)
{
	return zdbuf;
}

const char* ztime(void)
{
	return ztbuf;
}

void ztimer(void *)
{
	struct tm tm;
	time_t t_temp;
	struct timeval tv;

	gettimeofday(&tv, NULL);

	t_temp=(time_t)tv.tv_sec;
	gmtime_r(&t_temp, &tm);

	if (!strftime(zdbuf, sizeof(zdbuf), "%Y%m%d", &tm))
		memset(zdbuf, 0, sizeof(zdbuf));

	if (!strftime(ztbuf, sizeof(ztbuf), "%H:%M:%S Z", &tm))
		memset(ztbuf, 0, sizeof(ztbuf));

	txtTOD->value(ztbuf);
	txtTOD->redraw();
//	Fl::awake(show_time);
}

//======================================================================
// TOD Thread loop
//======================================================================
static bool first_call = true;
static bool TOD_exit = false;
static bool TOD_enabled = false;

void *TOD_loop(void *args)
{
	SET_THREAD_ID(TOD_TID);

	int count = 20;
	while(1) {
		if (TOD_exit) break;
		if (first_call) {
			struct timeval tv;
			gettimeofday(&tv, NULL);
			double st = 1000.0 - tv.tv_usec / 1e3;
			MilliSleep(st);
			first_call = false;
			_zmsec += st;
		} else {
			MilliSleep(50);
			_zmsec += 50;
		}
		if (--count == 0) {
			Fl::awake(ztimer);
			count = 20;
		}
	}
	return NULL;
}

//======================================================================
//
//======================================================================
void TOD_init(void)
{
	TOD_exit = false;

	if (pthread_create(&TOD_thread, NULL, TOD_loop, NULL) < 0) {
		LOG_ERROR("%s", "pthread_create failed");
		return;
	}

	LOG_INFO("%s", "Time Of Day thread started");

	TOD_enabled = true;
}

//======================================================================
//
//======================================================================
void TOD_close(void)
{
	if (!TOD_enabled) return;

	TOD_exit = true;
	pthread_join(TOD_thread, NULL);
	TOD_enabled = false;

	LOG_INFO("%s", "Time Of Day thread terminated. ");

}

//======================================================================
//======================================================================

pthread_mutex_t dxcc_mutex     = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t dxc_line_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t brws_que_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t brws_dxc_help_mutex = PTHREAD_MUTEX_INITIALIZER;

static char even[50];
static char odd[50];

//======================================================================
// fldigi modem names
//======================================================================
enum { CW, PSK, BPSK31, BPSK63, BPSK63F, BPSK125, RTTY, CTSTIA, SSB };

struct MODEMS { int mode; std::string name; std::string srch;};

MODEMS modems[] = {
{CW, "CW", "CW"},
{PSK, "BPSK", "PSK"},
{BPSK31, "BPSK31", "PSK31"},
{BPSK63, "BPSK63", "PSK63"},
{BPSK63F, "BPSK63F", "PSK63F"},
{BPSK125, "BPSK125", "PSK125"},
{RTTY, "RTTY", "RTTY"},
{CTSTIA, "CTSTIA", "CTSTIA"},
{SSB, "SSB", "SSB"} };

//======================================================================
// Socket DXcluster i/o used on all platforms
//======================================================================

pthread_t       DXcluster_thread;
pthread_mutex_t DXcluster_mutex        = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t DXcluster_write_mutex  = PTHREAD_MUTEX_INITIALIZER;

Socket *DXcluster_socket = 0;

enum DXC_STATES {DISCONNECTED, CONNECTING, CONNECTED};

int  DXcluster_state = DISCONNECTED;
bool DXcluster_exit = false;
bool DXcluster_enabled = false;

#define DXCLUSTER_CONNECT_TIMEOUT 5000 // 5 second timeout
#define DXCLUSTER_SOCKET_TIMEOUT 100 // milliseconds
#define DXCLUSTER_LOOP_TIME 100 // milliseconds
int  DXcluster_connect_timeout =
	(DXCLUSTER_CONNECT_TIMEOUT) / (DXCLUSTER_SOCKET_TIMEOUT + DXCLUSTER_LOOP_TIME);

void write_socket(string s)
{
	try {
		guard_lock write_lock(&DXcluster_write_mutex);
		DXcluster_socket->send(s);
	} catch (const SocketException& e) {
		LOG_ERROR("%s", e.what());
		write_dxc_debug('E', e.what());
		Fl::awake(restart_connection);
	} catch (...) {
		write_dxc_debug('E', "unrecoverable error");
		exit(1);
	}
}

void set_btn_dxcc_connect(void * v)
{
	if (v)
		btn_dxcc_connect->value(1);
	else
		btn_dxcc_connect->value(0);
	btn_dxcc_connect->redraw();
}

void dxc_label(void *)
{
	switch (DXcluster_state) {
		case CONNECTING:
			lbl_dxc_connected->color(FL_YELLOW);
			cluster_tabs->value(tabDXclusterTelNetStream);
			break;
		case CONNECTED:
			lbl_dxc_connected->color(FL_GREEN);
			cluster_tabs->value(tabDXclusterTelNetStream);
			break;
		case DISCONNECTED:
		default :
			lbl_dxc_connected->color(FL_WHITE);
	}
	lbl_dxc_connected->redraw_label();
	lbl_dxc_connected->redraw();
}

string hexvals(string s)
{
	static string sret;
	static char hstr[8];
	sret.clear();
	snprintf(hstr, sizeof(hstr), "{%3d}\"", (int)s.length());
	sret = hstr;
	for (size_t n = 0; n < s.length(); n++) {
		if (s[n] < ' ' || s[n] > 0x7e ) {
			snprintf(hstr, sizeof(hstr), "[x%02X]", s[n]);
			sret.append(hstr);
		} else
			sret += s[n];
	}
	sret.append("\"");
	return sret;
}

static void write_raw(string s)
{
	return; // comment out for debugging raw text rcvd from server

	string pname = HOME_DIR;
	string hexstr;
	hexstr.append("[").append(ztbuf).append("]").append(hexvals(s));
	pname.append("raw.txt");
	FILE *raw_text = fl_fopen(pname.c_str(), "a");
	fprintf(raw_text, "%s\n", hexstr.c_str());
	fclose(raw_text);
}

static string trim(string s)
{
	size_t p;
	while (s[s.length()-1] == ' ') s.erase(s.length()-1, 1);
	while ( (p = s.find("\x07")) != string::npos) s.erase(p, 1);
	while ( (p = s.find("\b")) != string::npos) s.erase(p,1);
	while ((p = s.find("\r")) != string::npos) s.erase(p,1);
	while ((p = s.find("\n")) != string::npos) s.erase(p,1);
	while (s[0] == ' ') s.erase(0,1);

	return s;
}

static string ucasestr(string s)
{
	for (size_t n = 0; n < s.length(); n++) s[n] = toupper(s[n]);
	return s;
}

//--------------------------------------------------------------------------------
//          1         2         3         4         5         6         7
//01234567890123456789012345678901234567890123456789012345678901234567890123456789
//          1     ^   2     ^   3        ^4         5         6         ^
//DX de KB8O:      14240.0  D66D         up 10 59 Ohio                  2059Z EN81<cr><lf>
//DX de W4DL:      18082.0  V44KAI                                      2220Z<cr><lf>
//DX de K2IOF:     14240.0  D66D         up 5                           2220Z<cr><lf>
//DX de N3ZV:      10147.0  AN400G       RTTY                           2220Z FM18<BELL><BELL><cr><lf>
//DX de W4LT:      14204.0  EA3HSO                                      2218Z EL88<BELL><BELL><cr><lf>
//--------------------------------------------------------------------------------

static string tcpip_buffer;

std::queue<string> brws_que;

void update_brws_tcpip_stream(void *)
{
	guard_lock brws_lock(&brws_que_mutex);
	static string s;

	while (!brws_que.empty()) {
		s = brws_que.front();
		brws_que.pop();
		brws_tcpip_stream->insert_position(brws_tcpip_stream->buffer()->length());
		brws_tcpip_stream->addstr(s);
	}
	brws_tcpip_stream->redraw();
}

void show_tx_stream(string buff)
{
	size_t p;
	while ((p = buff.find("\r")) != string::npos) buff.erase(p,1);
	if (buff[buff.length()-1] != '\n') buff += '\n';

	guard_lock brws_lock(&brws_que_mutex);
	brws_que.push(buff);
	Fl::awake(update_brws_tcpip_stream);

	write_dxc_debug('W', buff);
}

void show_rx_stream(string buff)
{
	for (size_t p = 0; p < buff.length(); p++) {
		if ( (buff[p] < '\n') ||
			 (buff[p] > '\n' && buff[p] < ' ') ||
			 (buff[p] > '~') )
			buff[p] = ' ';
	}

	if (buff.empty()) buff = "\n";
	else if (buff[buff.length()-1] != '\n') buff += '\n';

	guard_lock brws_lock(&brws_que_mutex);
	brws_que.push(buff);
	Fl::awake(update_brws_tcpip_stream);

	write_dxc_debug('R', buff);
}

void show_error(string buff)
{
	if (buff.empty()) return;
	if (!brws_tcpip_stream) return;
	if (buff[buff.length()-1] != '\n') buff += '\n';

	guard_lock brws_lock(&brws_que_mutex);
	brws_que.push(buff);
	Fl::awake(update_brws_tcpip_stream);

	write_dxc_debug('E', buff);
}

//---------------------------------------------------------------------------------
//         1         2         3         4         5         6         7         8
//1234567890123456789012345678901234567890123456789012345678901234567890123456789012345
//|          ||        | |            ||                                | |           |
//KB8O        10114240.0 D66D          up 10 59 Ohio                   NE 2059Z EN81 AL
//call    15/100
//freq    15/100
//spot    15/100
//comment 40/100
//time     6/100
//rem      9/100
//       100/100
static int widths[] = {13, 14, 13, 43, 7, 0};

void dxc_column_widths()
{
	widths[0] = 13 * brws_dx_cluster->w() / 100;
	widths[1] = 14 * brws_dx_cluster->w() / 100;
	widths[2] = 13 * brws_dx_cluster->w() / 100;
	widths[3] = 43 * brws_dx_cluster->w() / 100;
	widths[4] =  7 * brws_dx_cluster->w() / 100;

	brws_dx_cluster->column_char('^');
	brws_dx_cluster->column_widths(widths);
	reports_header->column_char('^');
	reports_header->column_widths(widths);

	fl_font(progStatus.DXC_textfont, progStatus.DXC_textsize);
	int hh = fl_height() + 4;
	int h = reports_header->h() + brws_dx_cluster->h();
	reports_header->resize(
		reports_header->x(), reports_header->y(),
		reports_header->w(), hh);
	brws_dx_cluster->resize(
		brws_dx_cluster->x(), brws_dx_cluster->y(),
		brws_dx_cluster->w(), h - hh);
	reports_header->redraw();
	brws_dx_cluster->redraw();
}

void dxc_colors_fonts()
{
	char hdr[200];

	dxc_column_widths();

	snprintf(odd, sizeof(odd),
			"@B%u@C%u@F%d@S%d",
			progStatus.DXC_odd_color,
			FL_BLACK,
			progStatus.DXC_textfont,
			progStatus.DXC_textsize);

	snprintf(even, sizeof(even),
		"@B%u@C%u@F%d@S%d",
		progStatus.DXC_even_color,
		FL_BLACK,
		progStatus.DXC_textfont,
		progStatus.DXC_textsize);

	snprintf(hdr, sizeof(hdr),
		"%s@cSpotter^%s@cFreq^%s@cDx Call^%s@cComments^%s@cUTC^%s@cQRA",
		even, odd, even, odd, even, odd);
	reports_header->clear();
	reports_header->add(hdr);

}

void dxc_lines()
{
	guard_lock dxcc_lock(&dxc_line_mutex);
	int n = brws_dx_cluster->size();

	if (n == 0) return;

	queue<string> lines;
	string dxc_line;

	size_t p;
	for (int i = 0; i < n; i++) {
		if (progStatus.dxc_topline)
			dxc_line = brws_dx_cluster->text(i+1);
		else
			dxc_line = brws_dx_cluster->text(n - i);

		while ((p = dxc_line.find(even)) != string::npos)
			dxc_line.erase(p, strlen(even));

		while ((p = dxc_line.find(odd)) != string::npos)
			dxc_line.erase(p, strlen(odd));

		lines.push(dxc_line);
	}

	dxc_colors_fonts();

	brws_dx_cluster->clear();

	for (int i = 0; i < n; i++) {
		dxc_line = lines.front();
		if (i % 2) {
			dxc_line.insert(0, even);
			p = 0;
			while ((p = dxc_line.find("^", p)) != string::npos) {
				dxc_line.insert(p+1, even);
				p += strlen(even);
			}
		} else {
			dxc_line.insert(0, odd);
			p = 0;
			while ((p = dxc_line.find("^", p)) != string::npos) {
				dxc_line.insert(p+1, odd);
				p += strlen(odd);
			}
		}

		lines.pop();

		if (progStatus.dxc_topline)
			brws_dx_cluster->insert(1, dxc_line.c_str());
		else
			brws_dx_cluster->add(dxc_line.c_str());
	}

	if (progStatus.dxc_topline)
		brws_dx_cluster->make_visible(1);
	else
		brws_dx_cluster->bottomline(brws_dx_cluster->size());

	brws_dx_cluster->redraw();
}

/*===============================================================================
  raw dx spot line
          1         2         3         4         5         6         7
012345678901234567890123456789012345678901234567890123456789012345678901234567890
      <   1    |    2  >  <   3        <4         5         6        <
DX de NK9O:      14074.0  TI4DJ        FT8 EN63>EK70                 0021Z WI
DX de VE9CB:      7016.9  EA8AF                                      0021Z
DX de K6KQV:     14074.0  XO1X         Many Tnx.                     0018Z CA
DX de W7MEM:     14074.0  AJ8MH        DN17NR<F2>EN66HM            MI 0014Z ID
DX de N7RD:      10112.5  W8RIK        SORRY QSB NO COPY           OH 0011Z AZ
DX de W6WDY:     14076.0  JH4IFF       DM04<>PM74AR                  0005Z CA
DX de W6LEN:     10124.0  K2FI         KFF-0613                    NJ 0004Z CA
DX de VE9CB:     14016.0  NP4R         CQ DX                       PR 0003Z
DX de W7FW:       7009.1  LU1YT        tnx Carlos                    2355Z IL
================================================================================*/

std::queue<string> dxclines;

void update_brws_dx_cluster(void *)
{
	guard_lock dxcc_lock(&dxc_line_mutex);

	string dxc_line;

	dxc_colors_fonts();

	while (!dxclines.empty()) {
		dxc_line = dxclines.front();
		dxclines.pop();

		size_t p = string::npos;
		if (brws_dx_cluster->size() % 2) {
			dxc_line.insert(0, even);
			p = 0;
			while ((p = dxc_line.find("^", p)) != string::npos) {
				dxc_line.insert(p+1, even);
				p += strlen(even);
			}
		} else {
			dxc_line.insert(0, odd);
			p = 0;
			while ((p = dxc_line.find("^", p)) != string::npos) {
				dxc_line.insert(p+1, odd);
				p += strlen(odd);
			}
		}

		if (progStatus.dxc_topline) {
			bool visible = brws_dx_cluster->displayed(1);
			brws_dx_cluster->insert(1, dxc_line.c_str());
			if (visible) brws_dx_cluster->make_visible(1);
		} else {
			bool visible = brws_dx_cluster->displayed(brws_dx_cluster->size());
			brws_dx_cluster->add(dxc_line.c_str());
			if (visible) brws_dx_cluster->bottomline(brws_dx_cluster->size());
		}
	}
}

void parse_dxline(string dxbuffer)
{
	guard_lock dxcc_lock(&dxc_line_mutex);

	dxinfo dxc;

	dxbuffer.erase(0, strlen("DX de "));
	size_t p = dxbuffer.find(":");
	if (p == string::npos || p > 16) 
		p = dxbuffer.find(" ");

	dxc.spotter = dxbuffer.substr(0,p);
	dxbuffer.erase(0,p+1);
	while(dxbuffer[0] == ' ') dxbuffer.erase(0,1);

	p = dxbuffer.find(" ");
	dxc.freq = dxbuffer.substr(0,p);
	dxbuffer.erase(0,p+1);
	while (dxbuffer[0] == ' ') dxbuffer.erase(0,1);

	p = dxbuffer.find(" ");
	dxc.call = dxbuffer.substr(0,p);
	dxbuffer.erase(0, p+1);

	string tzulu;
	p = 0;
	while (p < dxbuffer.length() - 4) {
		if (isdigit(dxbuffer[p]) &&
			isdigit(dxbuffer[p+1]) &&
			isdigit(dxbuffer[p+2]) &&
			isdigit(dxbuffer[p+3]) &&
			(dxbuffer[p+4] == 'Z' || dxbuffer[4] == 'z')) {
			dxc.time = dxbuffer.substr(p, 5);
			if (dxbuffer.length() > p+5)
				dxc.spotter_US_state = dxbuffer.substr(p+5);
			dxbuffer.erase(p-1);
		}
		p++;
	}
	dxc.comment = dxbuffer;

	while (dxc.comment[0] == ' ') dxc.comment.erase(0,1);
	while (dxc.comment[dxc.comment.length()-1] == ' ')
		dxc.comment.erase(dxc.comment.length() - 1);
	size_t n = dxc.comment.length();
	if (n == 2) dxc.comment.insert(0,34, ' ');
	else if (n > 3) {
		if ((dxc.comment[n-3] == ' ') && (isalpha(dxc.comment[n-2])) &&
			(isalpha(dxc.comment[n-1]))) {
			dxc.comment.insert(n-3, 36 - n, ' ');
		}
	}

	string spot;
	spot.assign("@.").append(dxc.spotter);
	spot.append("^@r@.").append(dxc.freq);
	spot.append("^@.").append(dxc.call);
	spot.append("^@.").append(dxc.comment);
	spot.append("^@r@.").append(dxc.time);
	spot.append("^@.").append(dxc.spotter_US_state);

	dxclines.push(spot);

	Fl::awake(update_brws_dx_cluster);
}

void parse_cc11_line(string buffer)
{

//If you do a set/ve7cc command on connection startup, you will get this
//out for spots (everything else is "normal"):

//CC11^7083.0^OP17A^26-Aug-2017^0859Z^Sp�cial call com 14-18^ON3LX^134^134^F5LEN-7^27^14^27^14^^^Belgium-ON^Belgium-ON^^JN29
//CC11^14260.0^IQ2XZ/P^26-Aug-2017^0859Z^AWARD PAOLO NESPOLI 1PT^IZ1VZG^85^85^GB7UJS^28^15^28^15^^^Lombardia-I^Liguria-Piemonte-I^^JN34

// CC11
// normalised freq (in khz, 1 dec.pl.)
// call
// date
// time
// comment
// spotter
// call country code
// spotter country code
// originating node
// call itu zone
// call cq zone
// spotter itu zone
// spotter cq zone
// call US state
// spotter US state
// call country in text
// spotted country in text
// call grid square
// spotter grid square

	dxinfo dxcc;

	size_t p = 0;
	p = buffer.find("^");
	buffer.erase(0, p+1); p = buffer.find("^"); dxcc.freq = buffer.substr(0,p);
	buffer.erase(0, p+1); p = buffer.find("^"); dxcc.call = buffer.substr(0,p);
	buffer.erase(0, p+1); p = buffer.find("^"); dxcc.date = buffer.substr(0,p);
	buffer.erase(0, p+1); p = buffer.find("^"); dxcc.time = buffer.substr(0,p);
	buffer.erase(0, p+1); p = buffer.find("^"); dxcc.comment = buffer.substr(0,p);
	buffer.erase(0, p+1); p = buffer.find("^"); dxcc.spotter = buffer.substr(0,p);
	buffer.erase(0, p+1); p = buffer.find("^"); dxcc.call_country_code = buffer.substr(0,p);
	buffer.erase(0, p+1); p = buffer.find("^"); dxcc.spotter_country_code = buffer.substr(0,p);
	buffer.erase(0, p+1); p = buffer.find("^"); dxcc.originating_node = buffer.substr(0,p);
	buffer.erase(0, p+1); p = buffer.find("^"); dxcc.call_itu_zone = buffer.substr(0,p);
	buffer.erase(0, p+1); p = buffer.find("^"); dxcc.call_cq_zone = buffer.substr(0,p);
	buffer.erase(0, p+1); p = buffer.find("^"); dxcc.spotter_itu_zone = buffer.substr(0,p);
	buffer.erase(0, p+1); p = buffer.find("^"); dxcc.spotter_cq_zone = buffer.substr(0,p);
	buffer.erase(0, p+1); p = buffer.find("^"); dxcc.call_US_state = buffer.substr(0,p);
	buffer.erase(0, p+1); p = buffer.find("^"); dxcc.spotter_US_state = buffer.substr(0,p);
	buffer.erase(0, p+1); p = buffer.find("^"); dxcc.call_country_text = buffer.substr(0,p);
	buffer.erase(0, p+1); p = buffer.find("^"); dxcc.spotter_country_text = buffer.substr(0,p);
	buffer.erase(0, p+1); p = buffer.find("^"); dxcc.call_grid_square = buffer.substr(0,p);
	buffer.erase(0, p+1); dxcc.spotter_grid_square = buffer;

	string spot;
	spot.assign("@.").append(dxcc.spotter);
	spot.append("^@r@.").append(dxcc.freq);
	spot.append("^@.").append(dxcc.call);

	if (dxcc.call_US_state.length() < 2) {
		if (dxcc.comment.length() > 36) dxcc.comment.erase(36);
		if (dxcc.comment.length() < 36) dxcc.comment.append(36 - dxcc.comment.length(), ' ');
	} else {
		if (dxcc.comment.length() > 33) dxcc.comment.erase(33);
		if (dxcc.comment.length() < 33) dxcc.comment.append(33 - dxcc.comment.length(), ' ');
		dxcc.comment.append(" ").append(dxcc.call_US_state);
	}

	spot.append("^@.").append(dxcc.comment);
	spot.append("^@r@.").append(trim(dxcc.time));

	if (!trim(dxcc.spotter_US_state).empty()) {
		spot.append("^@.").append(trim(dxcc.spotter_US_state));
		if (!trim(dxcc.call_grid_square).empty())
			spot.append(" ").append(trim(dxcc.call_grid_square));
	} else {
		if (!trim(dxcc.call_grid_square).empty())
			spot.append("^@.").append(trim(dxcc.call_grid_square));
	}

	dxclines.push(spot);

	Fl::awake(update_brws_dx_cluster);

}

std::queue<string> help_que;

void update_brws_dxc_help(void *)
{
	guard_lock brws_lock(&brws_dxc_help_mutex);
	while (!help_que.empty()) {
		brws_dxc_help->insert_position(brws_dxc_help->buffer()->length());
		brws_dxc_help->addstr(help_que.front());
		help_que.pop();
	}
	brws_dxc_help->redraw();
}

void show_help_line(std::string helpbuff) //void *)
{
	guard_lock brws_lock(&brws_dxc_help_mutex);
	help_que.push(helpbuff.append("\n"));
	Fl::awake(update_brws_dxc_help);

	write_dxc_debug('H', helpbuff);
	helpbuff.clear();
}

enum server_type {NIL, DX_SPIDER, AR_CLUSTER, CC_CLUSTER};
static int  cluster_login = NIL;
static int  cluster_type = NIL;
static bool logged_in = false;

static char minute = 'a';
static char tens = 'b';

void clear_keepalive_box(void *)
{
	box_keepalive->color(FL_BACKGROUND_COLOR);
	box_keepalive->redraw();
}

void restore_keepalive_box(void *)
{
	Fl::add_timeout(2.0, clear_keepalive_box);
}

void red_keepalive_box(void *)
{
	box_keepalive->color(FL_RED);
	box_keepalive->redraw();
}

void green_keepalive_box(void *)
{
	box_keepalive->color(FL_GREEN);
	box_keepalive->redraw();
}

int wait_for_keepalive = -1;

void keepalive()
{
// disabled if 'never'
	if (progStatus.keepalive == 0)
		return;

// send \b every NN minutes, NN = 0 (never), 1, 5 or 10
	bool b_keepalive = false;
//	if (!progStatus.keepalive) return; // no keep alive

	if ((progStatus.keepalive) == 1 && (ztbuf[4] != minute))
		b_keepalive = true;

	else if (progStatus.keepalive == 5) {
		if ((ztbuf[4] == '0') && (minute != '0'))
			b_keepalive = true;
		if ((ztbuf[4] == '5') && (minute != '5'))
			b_keepalive = true;
	}

//	else if ((progStatus.keepalive == 10) && (ztbuf[3] != tens))
	else if (ztbuf[3] != tens)
		b_keepalive = true;

	if (b_keepalive) {
		minute = ztbuf[4];
		tens = ztbuf[3];
		if (DXcluster_socket && logged_in) {
			switch (cluster_type) {
				case AR_CLUSTER:
					write_socket("show/time\r\n");
					break;
				case CC_CLUSTER:
					write_socket("show/time\r\n");
					break;
				case DX_SPIDER:
					write_socket("\r\n");
					break;
				default:
					return;
			}
			wait_for_keepalive = 300; // 30000 msec,  30 second timeout
			Fl::awake(green_keepalive_box);
		}
	}
}

void login_to_dxspider()
{
	if (!DXcluster_socket) return;
	try {
		string login = progStatus.dxcc_login;
		login.append("\r\n");
		write_socket(login);
		show_tx_stream(login);

		logged_in = true;
		connection_timeout = 0;
		cluster_login = NIL;

		string str_loggedin = "Logged in to ";
		str_loggedin.append(progStatus.dxcc_host_url).append(":");
		str_loggedin.append(progStatus.dxcc_host_port);

		LOG_INFO("%s", str_loggedin.c_str());

	} catch (const SocketException& e) {
		LOG_ERROR("%s", e.what() );
		show_error(e.what());
	}
}

void login_to_arcluster()
{
	try {
		string login = progStatus.dxcc_login;
		login.append("\r\n");
		write_socket(login);
		show_tx_stream(login);

		logged_in = true;
		connection_timeout = 0;
		cluster_login = NIL;

		string str_loggedin = "Logged in to ";
		str_loggedin.append(progStatus.dxcc_host_url).append(":");
		str_loggedin.append(progStatus.dxcc_host_port);

		LOG_INFO("%s", str_loggedin.c_str());

	} catch (const SocketException& e) {
		LOG_ERROR("%s", e.what() );
		show_error(e.what());
	}
}

/*
 * telnet session with dxspots.com 7300
 *
Trying 204.221.76.52...
Connected to dxspots.com.
Escape character is '^]'.
Greetings from the AE5E CC Cluster in Thief River Falls MN USA

Running CC Cluster software version 3.101b

*************************************************************************
*                                                                       *
*     Please login with a callsign indicating your correct country      *
*                          Portable calls are ok.                       *
*                                                                       *
************************************************************************

New commands:

set/skimmer   turns on Skimmer spots.
set/noskimmer turns off Skimmer spots.

set/own      turns on Skimmer spots for own call.
set/noown    turns them off.

set/nobeacon  turns off spots for beacons.
set/beacon    turns them back on.

For information on CC Cluster software see:

http://bcdxc.org/ve7cc/ccc/CCC_Commands.htm
Please enter your call:

login: w1hkj
W1HKJ

Hello David.

CC-User is the recommended telnet acess program. It simplifies filtering,
may be used stand alone or can feed spots to your logging program.
CC_User is free at: http://ve7cc.net/default.htm#prog
CC_User group at: http://groups.yahoo.com/group/ARUser

Using telnet port 7000

Cluster: 423 nodes  459 Locals  4704 Total users   Uptime   7 days 15:32

Date        Hour   SFI   A   K Forecast                               Logger
19-Nov-2016    0    78   3   0 No Storms -> No Storms                <VE7CC>
W1HKJ de AE5E 19-Nov-2016 0154Z   CCC >
W1HKJ de AE5E 19-Nov-2016 0154Z   CCC >
bye
73 David.  W1HKJ de AE5E 19-Nov-2016 0154Z  CCC >
Connection closed by foreign host.

*/

void send_name()
{
	if (!DXcluster_socket || logged_in == false) return;

	try {
		guard_lock dxc_lock(&DXcluster_mutex);
		string login;
		login.assign("set/name ").append(progStatus.myName);
		login.append("\r\n");
		write_socket(login);
		show_tx_stream(login);
	} catch (const SocketException& e) {
		LOG_ERROR("%s", e.what() );
		show_error(e.what());
	}
}

void send_qth()
{
	if (!DXcluster_socket || logged_in == false) return;

	try {
		guard_lock dxc_lock(&DXcluster_mutex);
		string login;
		login.assign("set/qth ").append(progStatus.myQth);
		login.append("\r\n");
		write_socket(login);
		show_tx_stream(login);
	} catch (const SocketException& e) {
		LOG_ERROR("%s", e.what() );
		show_error(e.what());
	}
}

void send_qra()
{
	if (!DXcluster_socket || logged_in == false) return;

	try {
		guard_lock dxc_lock(&DXcluster_mutex);
		string login;
		login.assign("set/qra ").append(progStatus.myLocator);
		login.append("\r\n");
		write_socket(login);
		show_tx_stream(login);
	} catch (const SocketException& e) {
		LOG_ERROR("%s", e.what() );
		show_error(e.what());
	}
}

void login_to_cccluster()
{
	if (!DXcluster_socket) return;

	try {
		string login = progStatus.dxcc_login;
		login.append("\r\n");
		write_socket(login);
		show_tx_stream(login);

		logged_in = true;
		connection_timeout = 0;
		cluster_login = NIL;

		string str_loggedin = "Logged in to ";
		str_loggedin.append(progStatus.dxcc_host_url).append(":");
		str_loggedin.append(progStatus.dxcc_host_port);

		LOG_INFO("%s", str_loggedin.c_str());
		show_tx_stream(str_loggedin);

	} catch (const SocketException& e) {
		LOG_ERROR("%s", e.what() );
		show_error(e.what());
	}
}

void send_password()
{
	if (logged_in == false || inp_dxcc_password->value()[0] == 0)
		return;

	try {
		string password = inp_dxcc_password->value();
		password.append("\r\n");
		write_socket(password);
		show_tx_stream(password);

	} catch (const SocketException& e) {
		LOG_ERROR("%s", e.what() );
		show_error(e.what());
	}

}

static bool help_lines = false;

void clear_brws_tcpip_stream(void *)
{
	brws_tcpip_stream->clear();
	brws_tcpip_stream->redraw();
}

void init_cluster_stream()
{
	string buffer;
	help_lines = false;
	tcpip_buffer.clear();
	Fl::awake(clear_brws_tcpip_stream);
}

void parse_DXcluster_stream(string input_buffer)
{
	guard_lock dxcc_lock(&dxcc_mutex);
	string buffer;
	string ucasebuffer = ucasestr(input_buffer);

	if (!logged_in) {
		if (ucasebuffer.find("AR-CLUSTER") != string::npos) { // AR cluster
			init_cluster_stream();
			cluster_type = cluster_login = AR_CLUSTER;
		}
		else if (ucasebuffer.find("CC CLUSTER") != string::npos) { // CC cluster
			init_cluster_stream();
			cluster_type = cluster_login = CC_CLUSTER;
		}
		else if (ucasebuffer.find("LOGIN:") != string::npos) { // DX Spider
			init_cluster_stream();
			cluster_type = cluster_login = DX_SPIDER;
		}
	}

	tcpip_buffer.append(input_buffer);

	string strm;
	string its_me = progStatus.dxcc_login;
	its_me.append(" DE ");
	its_me = ucasestr(its_me);
	size_t p;

	while (!tcpip_buffer.empty()) {

		p = tcpip_buffer.find("\n");
		if (p != string::npos) {
			write_raw(tcpip_buffer.substr(0,p+1));
			buffer = trim(tcpip_buffer.substr(0, p+1));
			tcpip_buffer.erase(0, p + 1);
		} else {
			write_raw(tcpip_buffer);
			buffer = trim(tcpip_buffer);
			tcpip_buffer.clear();
		}

		if (buffer.empty())
			continue;

		show_rx_stream(buffer);

		ucasebuffer = ucasestr(buffer);
		if (ucasebuffer.find("DX DE") != string::npos) {
			parse_dxline(buffer);
			continue;
		}

		if (ucasebuffer.find("CC11^") != string::npos) {
			parse_cc11_line(buffer);
			continue;
		}

		if ((ucasebuffer.find("HELP") != string::npos) ||
			(ucasebuffer.find("APROPOS") != string::npos)){
			help_lines = true;
		}
		if (ucasebuffer.find(its_me) != string::npos) {
			help_lines = false;
			continue;
		}
		if (help_lines) {
			show_help_line(buffer);
			continue;
		}

		if (cluster_login == AR_CLUSTER && ucasebuffer.find("CALL:") != string::npos)
			login_to_arcluster();

		if (cluster_login == CC_CLUSTER && ucasebuffer.find("CALL:") != string::npos)
			login_to_cccluster();

		if (cluster_login == DX_SPIDER && ucasebuffer.find("LOGIN:") != string::npos)
			login_to_dxspider();

		if (ucasebuffer.find("PASSWORD:") != string::npos)
			send_password();

	}

	if (wait_for_keepalive > 0) {
		Fl::awake(restore_keepalive_box);
		wait_for_keepalive = -1;
	}
}

void clear_dxcluster_viewer()
{
	guard_lock dxcc_lock(&dxcc_mutex);
	brws_dx_cluster->clear();
}

//======================================================================
// Receive tcpip data stream
//======================================================================
void DXcluster_recv_data()
{
	if (!DXcluster_socket) return;
	string tempbuff;
	try {
		guard_lock dxc_lock(&DXcluster_mutex);
		if (DXcluster_state == CONNECTED) {
			DXcluster_socket->recv(tempbuff);
			if (!tempbuff.empty())
				parse_DXcluster_stream(tempbuff);
		}
		if (wait_for_keepalive > 0) {
			wait_for_keepalive--;
			if (wait_for_keepalive == 150)
				Fl::awake(red_keepalive_box);
			if (wait_for_keepalive == 0) {
				LOG_ERROR("Failed keepalive");
				Fl::awake(restart_connection);
			}
		}
	} catch (const SocketException& e) {
		LOG_ERROR("Error %d, %s", e.error(), e.what());
		Fl::awake(restart_connection);
//		abort_connection();
	}
}

//======================================================================
//
//======================================================================
const char *default_help[]={
"Help available after logging on\n",
"Try URL: k4zr.no-ip.org, PORT 7300\n",
"\n",
"Visit http://www.dxcluster.info/telnet/ for a listing of dx cluster servers",
NULL };

void dxc_help_query()
{
//	brws_dxc_help->clear();
	if ((DXcluster_state == DISCONNECTED) || !DXcluster_socket) {
		brws_dxc_help->clear();
		const char **help = default_help;
		while (*help) {
			brws_dxc_help->add(*help);
			help++;
		}
		return;
	}

	try {
		guard_lock dxc_lock(&DXcluster_mutex);
		string sendbuf = "help";
		if (inp_help_string->value()[0]) {
			string helptype = inp_help_string->value();
			if (ucasestr(helptype).find("HELP") != string::npos)
				sendbuf = helptype;
			else if (ucasestr(helptype).find("APROPOS") != string::npos)
				sendbuf = helptype;
			else
				sendbuf.append(" ").append(helptype);
		}
		sendbuf.append("\r\n");
		write_socket(sendbuf.c_str());
		show_tx_stream(sendbuf);
	} catch (const SocketException& e) {
		LOG_ERROR("%s", e.what() );
		show_error(e.what());
	}

}

void dxc_help_clear()
{
	guard_lock dxcc_lock(&dxcc_mutex);
	brws_dxc_help->clear();
}

//======================================================================
//
//======================================================================
void DXcluster_submit()
{
	if (!DXcluster_socket) return;
	try {
		guard_lock dxc_lock(&DXcluster_mutex);
		string sendbuf = trim(inp_dxcluster_cmd->value());
		string test = ucasestr(sendbuf);
		if (test.find("BYE") != string::npos) {
			fl_alert2("Uncheck the \"Connect\" button to disconnect!");
			logged_in = false;
			return;
		}
		sendbuf.append("\r\n");
		write_socket(sendbuf.c_str());
		show_tx_stream(sendbuf);
	} catch (const SocketException& e) {
		LOG_ERROR("%s", e.what() );
		show_error(e.what());
	}
	inp_dxcluster_cmd->value("");
}

void rf_af(long &rf, long &af, int md)
{
	if (md == SSB) {
		af = 0;
		return;
	}

	if (md == CW) {
		af = progStatus.CWsweetspot;
		if (progStatus.USBCW)
			rf -= af;
		else if (progStatus.LSBCW)
			rf += af;
	} else if (md == RTTY) {
		int shift = 170;
		af = progStatus.RTTYsweetspot;
		if (progStatus.useMARKfreq) {
			if (progStatus.USB)
				rf -= (af + shift / 2);
			else
				rf += (af - shift / 2);
		}
		else {
			if (progStatus.USB)
				rf -= af;
			else
				rf += af;
		}
	} else {
		af = progStatus.PSKsweetspot;
		if (progStatus.USB)
			rf -= af;
		else
			rf += af;
	}
	return;
}


void DXcluster_select()
{
	int sel = brws_dx_cluster->value();
	if (sel == 0) return;

//@B246@C56@F4@S15@.SQ6JFR^			# spotter
//@B246@C56@F4@S15@r@.14071.0^		# frequency
//@B246@C56@F4@S15@.LZ100SK^		# dx call
//@B246@C56@F4@S15@.Gosho 73^		# comments
//@B246@C56@F4@S15@r@.0920Z^		# time
//@B246@C56@F4@S15@.				# qra

	string dxcline = brws_dx_cluster->text(sel);

	size_t p = dxcline.find("@.");
	if (p == string::npos)
		return;

// remove rendition characters
	dxcline = dxcline.substr(p + 2);

// remove reporting stations call
	p = dxcline.find("@.");
	dxcline.erase(0, p + 2);

// find reported frequency
	p = dxcline.find("^");
	string sfreq = dxcline.substr(0, p);
//	dxcline.erase(0, p + 1);

// find dx call
	p = dxcline.find("@.");
	dxcline.erase(0, p + 2);
	p = dxcline.find("^");
	string dxcall = trim(dxcline.substr(0, p));
//	dxcline.erase(0, p + 1);

// treat remainder as remarks
// search for a mode name in the remarks

	p = dxcline.find("@.");
	dxcline.erase(0, p + 2);
	p = dxcline.find("^");
	dxcline.erase(p);

	dxcline = ucasestr(dxcline);
	int md = CW;
	for (size_t i = 0; i < sizeof(modems) / sizeof(*modems); i++) {
		if (dxcline.find(modems[i].srch) != string::npos) {
			md = modems[i].mode;
		}
	}
	if (md == PSK) md = BPSK31;

	long freq = (long)(atof(sfreq.c_str()) * 1000.0 + 0.5);
// does remark section have a [nn] block?
	p = dxcline.find("[");
	if (p != string::npos) {
		dxcline.erase(0, p+1);
		p = dxcline.find("]");
		if (p == 2)
			freq += atoi(dxcline.c_str());
	}
	long af = 1500;

	rf_af(freq, af, md);

	send_xml_dx_sta(dxcall, freq, modems[md].name);

}

//--------------------------------------------------------------------------------
//          1         2         3         4         5         6         7
//01234567890123456789012345678901234567890123456789012345678901234567890123456789
//          1     ^   2     ^   3        ^4         5         6         ^
//7080.4 CO3VR Optional Comment

void freqstrings( string &khz, string &hz )
{
	int freq = get_fldigi_freq();
	int ikhz = freq / 100;
	int ihz = freq % 100;

	char szfreq[20];
	snprintf(szfreq, sizeof(szfreq), "%d.%d", ikhz / 10, ikhz % 10);
	khz = szfreq;

	snprintf(szfreq, sizeof(szfreq), "%d", ihz);
	hz = szfreq;

	return;
}


void send_DXcluster_spot()
{
	std::string dxcall = get_fldigi_call();

	string hz, khz;
	freqstrings( khz, hz );

	string spot = "dx ";
	spot.append(khz)
		.append(" ")
		.append(dxcall)
		.append(" ");

	string comments = trim(inp_dxcluster_cmd->value());
	string currmode = get_fldigi_mode();

	if (comments.find(currmode) == string::npos) {
		if (progStatus.dxc_hertz) {
			currmode.append(" [")
					.append(hz).
					append("] ");
		}
		comments.insert(0, currmode);
	}
	spot.append(comments);

	inp_dxcluster_cmd->value(spot.c_str());
	tabDXclusterTelNetStream->show();

}


//======================================================================
//
//======================================================================

bool connect_changed;
bool connect_to_cluster;

void abort_connection()
{
	LOG_INFO("%s", "Aborting connection to server");

	logged_in = false;

	if (DXcluster_socket) {
		DXcluster_socket->shut_down();
		DXcluster_socket->close();
		delete DXcluster_socket;
		DXcluster_socket = 0;
	}
	DXcluster_state = DISCONNECTED;
	Fl::awake(dxc_label);
	progStatus.cluster_connected = false;
	Fl::awake(set_btn_dxcc_connect, (void *)(0));
	LOG_INFO("%s", "Socket connection closed");
}

void restart_connection(void *)
{
	LOG_INFO("%s", "Restart telnet session");

	logged_in = false;

	if (DXcluster_socket) {
		DXcluster_socket->shut_down();
		DXcluster_socket->close();
		delete DXcluster_socket;
		DXcluster_socket = 0;
	}
	DXcluster_state = DISCONNECTED;

	clear_keepalive_box(0);
	Fl::awake(set_btn_dxcc_connect, (void *)(1));
	connect_changed = true;
	connect_to_cluster = true;
}

void set_socket_options()
{
	DXcluster_socket->set_nonblocking(true);
	DXcluster_socket->set_timeout((double)(DXCLUSTER_SOCKET_TIMEOUT / 1000.0));

#ifdef __WIN32__
	int optval = 1;
	int optlen = sizeof (optval);
	int s = DXcluster_socket->fd();

	getsockopt(s, SOL_SOCKET, SO_TYPE, (char *)&optval, &optlen);
	LOG_INFO("socket is type %d", optval);

	if (getsockopt(s, SOL_SOCKET, SO_KEEPALIVE, (char *)&optval, &optlen) < 0) {
		LOG_ERROR("getsockopt failed");
		exit (1);
	}
	LOG_INFO("SO_KEEPALIVE is %s", (optval ? "ON" : "OFF"));

	optval = 1;
	if (setsockopt(s, SOL_SOCKET, SO_KEEPALIVE, (char *)&optval, optlen) < 0) {
		LOG_ERROR("setsockopt(...) error");
		exit(1);
	}

	getsockopt(s, SOL_SOCKET, SO_KEEPALIVE, (char *)&optval, &optlen);
	LOG_INFO("SO_KEEPALIVE is now set %s", (optval ? "ON" : "OFF"));

#else
	int optval = 1;
	socklen_t optlen = sizeof(optval);
	int s = DXcluster_socket->fd();

	getsockopt(s, SOL_SOCKET, SO_TYPE, &optval, &optlen);
	LOG_INFO("socket is type %d", optval);

	if (getsockopt(s, SOL_SOCKET, SO_KEEPALIVE, &optval, &optlen) < 0) {
		LOG_ERROR("getsockopt failed");
		exit (1);
	}
	LOG_INFO("SO_KEEPALIVE is %s", (optval ? "ON" : "OFF"));

	optval = 1;
	if (setsockopt(s, SOL_SOCKET, SO_KEEPALIVE, &optval, optlen) < 0) {
		LOG_ERROR("setsockopt(...) error");
		exit(1);
	}

	getsockopt(s, SOL_SOCKET, SO_KEEPALIVE, &optval, &optlen);
	LOG_INFO("SO_KEEPALIVE is now set %s", (optval ? "ON" : "OFF"));

#ifdef __APPLE__
	optval = 1;
	if (setsockopt(s, SOL_SOCKET, SO_NOSIGPIPE, &optval, optlen) < 0) {
		LOG_ERROR("set SO_NOSIGPIPE failed");
	}
	LOG_INFO("set SO_NOSIGPIPE");
#endif // __APPLE__
#endif // __WIN32__
}

void DXcluster_doconnect()
{
	int result = 0;
	if (connect_to_cluster) {
		clear_dxcluster_viewer();
		try {
			if (DXcluster_state == DISCONNECTED) {
				if (DXcluster_socket) {
					DXcluster_socket->shut_down();
					DXcluster_socket->close();
					delete DXcluster_socket;
					DXcluster_socket = 0;
					DXcluster_state = DISCONNECTED;
					Fl::awake(dxc_label);
				}
				Address addr = Address(
					progStatus.dxcc_host_url.c_str(),
					progStatus.dxcc_host_port.c_str() );

				DXcluster_socket = new Socket( addr );

				set_socket_options();
			}

			result = DXcluster_socket->connect();
			if ( (result == 0) || (result == EISCONN) || (result == EALREADY) ) {
				DXcluster_state = CONNECTED;
				Fl::awake(dxc_label);
				LOG_INFO( "%s", "Connected to socket");
				connect_changed = false;
				connection_timeout = 150;// 15 seconds timeout on new connection
				show_rx_stream("Connected to socket, waiting for login prompt");
				return;
			} else if ( (result == EWOULDBLOCK) || (result == EINPROGRESS) )
				DXcluster_state = CONNECTING;
			else
				DXcluster_state = DISCONNECTED;

			if ((DXcluster_state == CONNECTING) &&
				(DXcluster_connect_timeout-- <= 0) ) {
				show_error("Connection attempt timed out");
				LOG_ERROR("%s", "Connection attempt timed out");
				DXcluster_state = DISCONNECTED;
				Fl::awake(set_btn_dxcc_connect, (void *)(0));
				Fl::awake(dxc_label);
				if (DXcluster_socket) {
					DXcluster_socket->shut_down();
					DXcluster_socket->close();
					delete DXcluster_socket;
					DXcluster_socket = 0;
				}
			} else {
				LOG_INFO("Connecting %f seconds remaining", DXcluster_connect_timeout * DXCLUSTER_LOOP_TIME * 0.001);
			}
			connect_changed = false;
			return;
		} catch (const SocketException& e) {

			LOG_ERROR("%s", e.what() );
			show_error(e.what());

			if (DXcluster_socket) {
				DXcluster_socket->shut_down();
				DXcluster_socket->close();
				delete DXcluster_socket;
				DXcluster_socket = 0;
			}
			DXcluster_state = DISCONNECTED;
			Fl::awake(dxc_label);
			progStatus.cluster_connected = false;
			Fl::awake(set_btn_dxcc_connect, (void *)(0));

		}
	}	else {
// disconnect from server
		if (!DXcluster_socket) return;

		if (logged_in) {
			try {
				string bye = "BYE\r\n";
				write_socket(bye);
				show_tx_stream(bye);
				MilliSleep(100);
			} catch (const SocketException& e) {
				LOG_ERROR("%s", e.what() );
				show_error(e.what());
			}
		}

		connection_timeout = 0;

		logged_in = false;

		if (DXcluster_socket) {
			DXcluster_socket->shut_down();
			DXcluster_socket->close();
			delete DXcluster_socket;
			DXcluster_socket = 0;
		}
		DXcluster_state = DISCONNECTED;
		Fl::awake(dxc_label);
		progStatus.cluster_connected = false;
		Fl::awake(set_btn_dxcc_connect, (void *)(0));
		LOG_INFO("%s", "Disconnected from dxserver");
	}
	connect_changed = false;
}

void DXcluster_connect(bool val)
{
	connect_changed = true;
	connect_to_cluster = val;
}

//======================================================================
// Thread loop
//======================================================================
void *DXcluster_loop(void *args)
{
	SET_THREAD_ID(DXCC_TID);

	while(1) {
		MilliSleep(DXCLUSTER_LOOP_TIME);
		if (DXcluster_exit) break;
		if (!logged_in && connection_timeout) {
			--connection_timeout;
			if (!connection_timeout) {
				abort_connection();
				continue;
			}
		}
		if (connect_changed || (DXcluster_state == CONNECTING) )
			DXcluster_doconnect();
		if (DXcluster_state == CONNECTED) {
			DXcluster_recv_data();
			keepalive();
		}
	}
	// exit the DXCC thread
	SET_THREAD_CANCEL();
	return NULL;
}

//======================================================================
//
//======================================================================
void DXcluster_init(void)
{
	DXcluster_enabled = false;
	DXcluster_exit = false;

	TOD_init();

	if (pthread_create(&DXcluster_thread, NULL, DXcluster_loop, NULL) < 0) {
		LOG_ERROR("%s", "pthread_create failed");
		return;
	}

	LOG_INFO("%s", "dxserver thread started");

	DXcluster_enabled = true;

	dxc_colors_fonts();

	brws_tcpip_stream->color(fl_rgb_color(
		progStatus.DX_Color_R,
		progStatus.DX_Color_G,
		progStatus.DX_Color_B));
	brws_tcpip_stream->setFont(progStatus.DXfontnbr);
	brws_tcpip_stream->setFontSize(progStatus.DXfontsize);
	brws_tcpip_stream->setFontColor(progStatus.DXfontcolor, FTextBase::RECV);
	brws_tcpip_stream->setFontColor(progStatus.DXalt_color, FTextBase::XMIT);
	brws_tcpip_stream->setFontColor(
		fl_contrast(progStatus.DXfontcolor,
			fl_rgb_color(	progStatus.DX_Color_R,
				progStatus.DX_Color_G,
				progStatus.DX_Color_B) ),
		FTextBase::CTRL);

	ed_telnet_cmds->color(fl_rgb_color(
		progStatus.DX_Color_R,
		progStatus.DX_Color_G,
		progStatus.DX_Color_B));
	ed_telnet_cmds->setFont(progStatus.DXfontnbr);
	ed_telnet_cmds->setFontSize(progStatus.DXfontsize);
	ed_telnet_cmds->setFontColor(progStatus.DXfontcolor);

	brws_dxc_help->color(fl_rgb_color(
		progStatus.DX_Color_R,
		progStatus.DX_Color_G,
		progStatus.DX_Color_B));
	brws_dxc_help->setFontColor(progStatus.DXfontcolor, FTextBase::RECV);
	brws_dxc_help->setFont(progStatus.DXfontnbr);
	brws_dxc_help->setFontSize(progStatus.DXfontsize);

	brws_dxcluster_hosts->color(fl_rgb_color(
		progStatus.DX_Color_R,
		progStatus.DX_Color_G,
		progStatus.DX_Color_B));
	brws_dxcluster_hosts->textcolor(progStatus.DXfontcolor);
	brws_dxcluster_hosts->textfont(progStatus.DXfontnbr);
	brws_dxcluster_hosts->textsize(progStatus.DXfontsize);

	cluster_tabs->selection_color(progStatus.TabsColor);

	Fl::awake(set_btn_dxcc_connect, (void *)(0));

	if (progStatus.dxc_auto_connect) {
		DXcluster_connect(true);
		Fl::awake(set_btn_dxcc_connect, (void *)(1));
	}

}

//======================================================================
//
//======================================================================
void DXcluster_close(void)
{
	if (!DXcluster_enabled) return;

	if ((DXcluster_state != DISCONNECTED) && DXcluster_socket) {
		DXcluster_connect(false);
		int n = 500;
		while ((DXcluster_state != DISCONNECTED) && n) {
			MilliSleep(10);
			n--;
		}
		if (n == 0) {
			LOG_ERROR("%s", _("Failed to shut down dxcluster socket"));
			fl_message2(_("Failed to shut down dxcluster socket"));
			exit(1);
			return;
		}
	}

	DXcluster_exit = true;
	pthread_join(DXcluster_thread, NULL);
	DXcluster_enabled = false;

	LOG_INFO("%s", "dxserver thread terminated. ");

	if (DXcluster_socket) {
		DXcluster_socket->shut_down();
		DXcluster_socket->close();
		delete DXcluster_socket;
		DXcluster_socket = 0;
	}

	TOD_close();
}

void dxcluster_hosts_save()
{
	string hosts = "";
	int nlines = brws_dxcluster_hosts->size();
	if (!nlines) {
		progStatus.dxcluster_hosts = hosts;
		return;
	}
	string hostline;
	size_t p;
	for (int n = 1; n <= nlines; n++) {
		hostline = brws_dxcluster_hosts->text(n);
		p = hostline.find("@.");
		if (p != string::npos) hostline.erase(0,p+2);
		hosts.append(hostline).append("|");
	}
	progStatus.dxcluster_hosts = hosts;
	progStatus.changed = true;
}

void dxcluster_hosts_load()
{
	brws_dxcluster_hosts->color(fl_rgb_color(
		progStatus.DX_Color_R,
		progStatus.DX_Color_G,
		progStatus.DX_Color_B));
	brws_dxcluster_hosts->textcolor(progStatus.DXfontcolor);
	brws_dxcluster_hosts->textfont(progStatus.DXfontnbr);
	brws_dxcluster_hosts->textsize(progStatus.DXfontsize);

	brws_dxcluster_hosts->clear();

	if (progStatus.dxcluster_hosts.empty()) {
		return;
	}
	string hostline;

	string hosts = progStatus.dxcluster_hosts;
	size_t p = hosts.find("|");
	size_t p2;
	while (p != string::npos && p != 0) {
		hostline.assign(hosts.substr(0,p+1));
		p2 = hostline.find("::|");
		if (p2 != string::npos)
			hostline.insert(p2 + 1, progStatus.myCall);
		p2 = hostline.find("|");
		if (p2 != string::npos) hostline.erase(p2, 1);
		brws_dxcluster_hosts->add(hostline.c_str());
		hosts.erase(0, p+1);
		p = hosts.find("|");
	}
	brws_dxcluster_hosts->sort(FL_SORT_ASCENDING);
	brws_dxcluster_hosts->redraw();
}

void dxcluster_hosts_select(Fl_Button*, void*)
{
	string host_line;
	int line_nbr = brws_dxcluster_hosts->value();
	if (line_nbr == 0) return;
	host_line = brws_dxcluster_hosts->text(line_nbr);
	string	host_name,
			host_port,
			host_login,
			host_password;
	size_t p = host_line.find("@.");
	if (p != string::npos) host_line.erase(0, p + 2);
	p = host_line.find(":");
	if (p == string::npos) return;
	host_name = host_line.substr(0, p);
	host_line.erase(0, p+1);
	p = host_line.find(":");
	if (p == string::npos) return;
	host_port = host_line.substr(0, p);
	host_line.erase(0, p+1);
	p = host_line.find(":");
	if (p == string::npos)
		host_login = host_line;
	else {
		host_login = host_line.substr(0, p);
		host_line.erase(0, p+1);
		host_password = host_line;
	}

	progStatus.dxcc_host_url = host_name;
	inp_dxcc_host_url->value(host_name.c_str());
	inp_dxcc_host_url->redraw();

	progStatus.dxcc_host_port = host_port;
	inp_dccc_host_port->value(host_port.c_str());
	inp_dccc_host_port->redraw();

	progStatus.dxcc_login = host_login;
	inp_dccc_login->value(host_login.c_str());
	inp_dccc_login->redraw();

	progStatus.dxcc_password = host_password;
	inp_dxcc_password->value(host_password.c_str());
	inp_dxcc_password->redraw();

}

void dxcluster_hosts_delete(Fl_Button*, void*)
{
	int line_nbr = brws_dxcluster_hosts->value();
	if (line_nbr == 0) return;
	brws_dxcluster_hosts->remove(line_nbr);
	dxcluster_hosts_save();
	brws_dxcluster_hosts->redraw();
}

void dxcluster_hosts_clear(Fl_Button*, void*)
{
	brws_dxcluster_hosts->clear();
	dxcluster_hosts_save();
	brws_dxcluster_hosts->redraw();
}

void dxcluster_hosts_add(Fl_Button*, void*)
{
	brws_dxcluster_hosts->color(fl_rgb_color(
		progStatus.DX_Color_R,
		progStatus.DX_Color_G,
		progStatus.DX_Color_B));
	brws_dxcluster_hosts->textcolor(progStatus.DXfontcolor);
	brws_dxcluster_hosts->textfont(progStatus.DXfontnbr);
	brws_dxcluster_hosts->textsize(progStatus.DXfontsize);

	string host_line = progStatus.dxcc_host_url.c_str();
	host_line.append(":").append(progStatus.dxcc_host_port.c_str());
	host_line.append(":").append(progStatus.dxcc_login);
	host_line.append(":").append(progStatus.dxcc_password);
	if (brws_dx_cluster->size() > 0) {
		for (int i = 1; i <= brws_dxcluster_hosts->size(); i++) {
			if (host_line == brws_dxcluster_hosts->text(i))
				return;
		}
	}
	brws_dxcluster_hosts->add(host_line.c_str());
	brws_dxcluster_hosts->sort(FL_SORT_ASCENDING);
	brws_dxcluster_hosts->redraw();
	dxcluster_hosts_save();
}

void dxcluster_hosts_clear_setup(Fl_Button*, void*)
{
	ed_telnet_cmds->clear();
}

void dxcluster_hosts_load_setup(Fl_Button*, void*)
{
	const char* p = FSEL::select( _("Load dxcluster setup file"), "*.dxc", ScriptsDir.c_str());
	if (!p) return;
	if (!*p) return;
	ed_telnet_cmds->buffer()->loadfile(p);
}

void dxcluster_hosts_save_setup(Fl_Button*, void*)
{
	string defaultfilename = ScriptsDir;
	defaultfilename.append("default.dxc");
	const char* p = FSEL::saveas( _("Save dxcluster setup file"), "*.dxc",
		defaultfilename.c_str());
	if (!p) return;
	if (!*p) return;
	ed_telnet_cmds->buffer()->savefile(p);
}

void dxc_send_string(string &tosend)
{
	if (!DXcluster_socket) return;

	string line;
	size_t p;
	while (!tosend.empty()) {
		p = tosend.find("\n");
		if (p != string::npos) {
			line = tosend.substr(0,p);
			tosend.erase(0,p+1);
		} else {
			line = tosend;
			tosend.clear();
		}
		line.append("\r\n");
		try {
			write_socket(line);
			show_tx_stream(line);
		} catch (const SocketException& e) {
			LOG_ERROR("%s", e.what() );
			show_error(e.what());
		}
	}
}

void dxcluster_hosts_send_setup(Fl_Button*, void*)
{
	char *str = ed_telnet_cmds->buffer()->text();
	string tosend = str;
	free(str);
	dxc_send_string(tosend);
}

#include <fstream>

#include "arc-help.cxx"
void dxcluster_ar_help(Fl_Button*, void*)
{
	if (progStatus.AR_help_URL == "INTERNAL") {
		string fn_help = HelpDir;
		fn_help.append("arc_help.html");
			ofstream fo_help(fn_help.c_str());
			fo_help << arc_commands;
			fo_help.close();
		open_url(fn_help.c_str());
	} else
		open_url(progStatus.AR_help_URL.c_str());
}

#include "CCC_Commands.cxx"
void dxcluster_cc_help(Fl_Button*, void*)
{
	if (progStatus.CC_help_URL == "INTERNAL") {
		string fn_help = HelpDir;
		fn_help.append("ccc_help.html");
			ofstream fo_help(fn_help.c_str());
			fo_help << ccc_commands;
			fo_help.close();
		open_url(fn_help.c_str());
	} else
		open_url(progStatus.CC_help_URL.c_str());
}

#include "DXSpiderCommandReference.cxx"
void dxcluster_dx_help(Fl_Button*, void*)
{
	if (progStatus.DX_help_URL == "INTERNAL") {
		string fn_help = HelpDir;
		fn_help.append("dxc_help.html");
			ofstream fo_help(fn_help.c_str());
			fo_help << dxspider_cmds;
			fo_help.close();
		open_url(fn_help.c_str());
	} else
		open_url(progStatus.DX_help_URL.c_str());
}

#include "DXClusterServers.cxx"
void dxcluster_servers(Fl_Button*, void*)
{
	if (progStatus.serversURL == "INTERNAL") {
		string fn_help = HelpDir;
		fn_help.append("dxc_servers.html");
			ofstream fo_help(fn_help.c_str());
			fo_help << dxcc_servers;
			fo_help.close();
		open_url(fn_help.c_str());
	} else
		open_url(progStatus.serversURL.c_str());
}

void dxc_click_m1(Fl_Button*, void*)
{
	inp_dxcluster_cmd->value(progStatus.dxcm_text_1.c_str());
}

void dxc_click_m2(Fl_Button*, void*)
{
	inp_dxcluster_cmd->value(progStatus.dxcm_text_2.c_str());
}

void dxc_click_m3(Fl_Button*, void*)
{
	inp_dxcluster_cmd->value(progStatus.dxcm_text_3.c_str());
}

void dxc_click_m4(Fl_Button*, void*)
{
	inp_dxcluster_cmd->value(progStatus.dxcm_text_4.c_str());
}

void dxc_click_m5(Fl_Button*, void*)
{
	inp_dxcluster_cmd->value(progStatus.dxcm_text_5.c_str());
}

void dxc_click_m6(Fl_Button*, void*)
{
	inp_dxcluster_cmd->value(progStatus.dxcm_text_6.c_str());
}

void dxc_click_m7(Fl_Button*, void*)
{
	inp_dxcluster_cmd->value(progStatus.dxcm_text_7.c_str());
}

void dxc_click_m8(Fl_Button*, void*)
{
	inp_dxcluster_cmd->value(progStatus.dxcm_text_8.c_str());
}
