// ----------------------------------------------------------------------------
// Copyright (C) 2014
//              David Freese, W1HKJ
//
// This file is part of flcluster.
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

#ifndef SOCK_XML_IO_H
#define SOCK_XML_IO_H

extern bool wait_query;
extern bool fldigi_online;
extern bool run_fldigi_xmlrpc_thread;
extern bool bypass_fldigi_xmlrpc_thread;

extern void open_fldigi_xmlrpc();
extern void close_fldigi_xmlrpc();

extern void close_client();
extern void open_client();

extern void send_xml_dx_sta(std::string, long); // keep
extern int  get_fldigi_freq();

extern std::string get_fldigi_call();
extern std::string get_fldigi_mode();

extern void * fldigi_xmlrpc_loop(void *d);

#endif
