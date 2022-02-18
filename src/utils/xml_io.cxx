// ----------------------------------------------------------------------------
// Copyright (C) 2014
//              David Freese, W1HKJ
//
// This file is part of flrig.
//
// flrig is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// flrig is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
// ----------------------------------------------------------------------------

#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include <queue>
#include <iostream>

#include <FL/Fl.H>

#include "xml_io.h"
#include "debug.h"
#include "threads.h"
#include "XmlRpc.h"
#include "status.h"
#include "dx_dialog.h"

using XmlRpc::XmlRpcValue;

struct DX_STA {
	std::string call;
	long freq;
	std::string mode;
	DX_STA() { call = ""; freq = 0; mode = "BPSK31"; }
	DX_STA(std::string c, int f, std::string md) { call = c; freq = f; mode = md;}
};

std::queue<DX_STA> dx_stas;

static const double TIMEOUT = 1.0;

// these are get/set
static const char *main_set_frequency = "main.set_frequency";

static const char* log_get_frequency = "log.get_frequency";
static const char *log_set_call = "log.set_call";
static const char *log_get_call = "log.get_call";

static const char *modem_get_name = "modem.get_name";
static const char *modem_set_by_name = "modem.set_by_name";

static XmlRpc::XmlRpcClient* client;

pthread_t *fldigi_xmlrpc_thread = 0;
pthread_mutex_t fldigi_xmlrpc_mutex = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t send_queue_mutex = PTHREAD_MUTEX_INITIALIZER;

#define XML_UPDATE_INTERVAL  100 // milliseconds
#define CHECK_UPDATE_COUNT   (1000 / XML_UPDATE_INTERVAL)

bool run_fldigi_xmlrpc_thread = true;
bool fldigi_online = false;

int try_count = CHECK_UPDATE_COUNT;

//=====================================================================
// socket ops
//=====================================================================

static inline void execute(const char* name, const XmlRpcValue& param, XmlRpcValue& result)
{
	if (!client)
		throw "Client not enabled";
	if (client)
		if (!client->execute(name, param, result, TIMEOUT))
			throw XmlRpc::XmlRpcException(name);
}

// --------------------------------------------------------------------
// send functions
// --------------------------------------------------------------------

static void send_new_freq(long freq)
{
	if (!fldigi_online) return;
	try {
		XmlRpcValue f((double)freq), res;
		execute(main_set_frequency, f, res);
	}
	catch (const XmlRpc::XmlRpcException& e) {
	}
	catch (const char *){
	}
}

static void send_new_call(std::string call)
{
	if (!fldigi_online) return;
	try {
		XmlRpcValue c((std::string)call), res;
		execute(log_set_call, c, res);
	}
	catch (const XmlRpc::XmlRpcException& e) {
	}
	catch (const char *){
	}
}

static void send_new_mode(std::string mode)
{
	if (!fldigi_online) return;
	try {
		XmlRpcValue c((std::string)mode), res;
		execute(modem_set_by_name, c, res);
	}
	catch (const XmlRpc::XmlRpcException& e) {
	}
	catch (const char *) {
	}
}

// add mutex for frequency queu

void send_xml_dx_sta(std::string call, long freq, std::string mode)
{
	if (!fldigi_online) return;
	guard_lock gl(&send_queue_mutex);

	dx_stas.push( DX_STA(call, freq, mode));
}

void send_queue()
{
	if (!fldigi_online) return;
	while (!dx_stas.empty()) {
		guard_lock gl(&send_queue_mutex);
		send_new_call(dx_stas.front().call);
		send_new_freq(dx_stas.front().freq);
		send_new_mode(dx_stas.front().mode);
		dx_stas.pop();
	}
}

// --------------------------------------------------------------------
// receive functions
// --------------------------------------------------------------------

void set_fldigi_connect (void *d)
{
	bool b = (long *)d;
	lbl_fldigi_connected->color(b ? FL_RED : FL_BACKGROUND2_COLOR);
	lbl_fldigi_connected->redraw();
}

std::string get_fldigi_mode()
{
	if (!fldigi_online) return "";

	XmlRpcValue res;
	try {
		execute(modem_get_name, XmlRpcValue(), res);
		std::string dxmode = (std::string)res;
		return dxmode;
	} catch (const XmlRpc::XmlRpcException& e) {
		fldigi_online = false;
		Fl::awake(set_fldigi_connect, (void *)0);
		throw e;
	}
	catch (const char *){
		fldigi_online = false;
		Fl::awake(set_fldigi_connect, (void *)0);
	}
	return "";
}

std::string get_fldigi_call()
{
	if (!fldigi_online) return "";

	XmlRpcValue res;
	try {
		execute(log_get_call, XmlRpcValue(), res);
		std::string dxcall = (std::string)res;
		return dxcall;
	} catch (const XmlRpc::XmlRpcException& e) {
		fldigi_online = false;
		Fl::awake(set_fldigi_connect, (void *)0);
		throw e;
	}
	catch (const char *){
		fldigi_online = false;
		Fl::awake(set_fldigi_connect, (void *)0);
	}
	return "";
}

int  lfreq = 0;

int  get_fldigi_freq()
{
	return lfreq;
}

static void read_fldigi_freq()
{
	XmlRpcValue res;
	try {
		execute(log_get_frequency, XmlRpcValue(), res);
		std::string sfreq((std::string)res);
		size_t p = sfreq.find(".");
		if (p != std::string::npos) sfreq.erase(p,1);
		lfreq = atol(sfreq.c_str());
		fldigi_online = true;
		Fl::awake(set_fldigi_connect, (void *)1);
	} catch (const XmlRpc::XmlRpcException& e) {
		lfreq = 0;
		fldigi_online = false;
		Fl::awake(set_fldigi_connect, (void *)0);
		throw e;
	}
	catch (const char *){
		lfreq = 0;
		fldigi_online = false;
		Fl::awake(set_fldigi_connect, (void *)0);
	}
}

void * fldigi_xmlrpc_loop(void *d)
{
	for (;;) {
		MilliSleep(XML_UPDATE_INTERVAL);
		if (!run_fldigi_xmlrpc_thread) {
			LOG_ERROR("Exiting digi loop");
			break;
		}

		if (!client && progStatus.connect_to_fldigi)
			open_client();
		if (client && !progStatus.connect_to_fldigi)
			close_client();

		if (--try_count == 0) {
			if (!fldigi_online) {
				try {
					guard_lock gl_xmloop(&fldigi_xmlrpc_mutex);
					read_fldigi_freq();
				} catch (const XmlRpc::XmlRpcException& e) {
					fldigi_online = false;
					Fl::awake(set_fldigi_connect, (void *)0);
				}
				try_count = CHECK_UPDATE_COUNT;
			} else {
				try {
					guard_lock gl_xmloop(&fldigi_xmlrpc_mutex);
					send_queue();
					read_fldigi_freq();
				} catch (const XmlRpc::XmlRpcException& e) {
					LOG_ERROR("%s", e.getMessage().c_str());
					fldigi_online = false;
					Fl::awake(set_fldigi_connect, (void *)0);
				}
			}
			try_count = CHECK_UPDATE_COUNT;
		}
	}
	return NULL;
}

void close_client()
{
	guard_lock gl(&fldigi_xmlrpc_mutex);
	delete client;
	client = 0;
}

void open_client()
{
	if (!progStatus.connect_to_fldigi)
		return;

	guard_lock gl(&fldigi_xmlrpc_mutex);
	int server_port = atoi(progStatus.fldigi_port.c_str());
	client = new XmlRpc::XmlRpcClient(progStatus.fldigi_address.c_str(),
			server_port );
	XmlRpc::setVerbosity(0); // 0...5
}

void close_fldigi_xmlrpc()
{
	close_client();

	pthread_mutex_lock(&fldigi_xmlrpc_mutex);

	run_fldigi_xmlrpc_thread = false;

	pthread_mutex_unlock(&fldigi_xmlrpc_mutex);
	pthread_join(*fldigi_xmlrpc_thread, 0);
}

void open_fldigi_xmlrpc()
{
//	if (progStatus.connect_to_fldigi)
//		open_client();

	run_fldigi_xmlrpc_thread = true;

	fldigi_xmlrpc_thread = new pthread_t;
	if (pthread_create(fldigi_xmlrpc_thread, NULL, fldigi_xmlrpc_loop, NULL)) {
		perror("pthread_create");
		exit(EXIT_FAILURE);
	}

}


