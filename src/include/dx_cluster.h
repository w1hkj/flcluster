// ----------------------------------------------------------------------------
// dx_cluster.h  --  constants, variables, arrays & functions that need to be
//                  outside of any thread
//
// Copyright (C) 2006-2007
//		Dave Freese, W1HKJ
// Copyright (C) 2007-2010
//		Stelios Bounanos, M0GLD
//
// This file is part of fldigi.  Adapted in part from code contained in gmfsk
// source code distribution.
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
// along with fldigi.  If not, see <http://www.gnu.org/licenses/>.
// ----------------------------------------------------------------------------

#ifndef DX_CLUSTER_H
#define DX_CLUSTER_H

#include <string>

extern void DXcluster_init(void);
extern void DXcluster_close(void);
extern void DXcluster_connect(bool);
extern void DXcluster_submit();
extern void DXcluster_select();
extern void dxc_wwv_query();
extern void dxc_wwv_clear();
extern void dxc_help_query();
extern void dxc_help_clear();

extern void DXcluster_logoff();
extern void DXcluster_add_record();

extern void DXcluster_mode_check();
extern void DXcluster_band_check();
extern bool DXcluster_dupcheck();

extern bool DXcluster_connected;

extern void send_DXcluster_spot();

extern void dxcluster_hosts_save();
extern void dxcluster_hosts_load();

extern void dxcluster_hosts_clear_setup(Fl_Button*, void*);
extern void dxcluster_hosts_load_setup(Fl_Button*, void*);
extern void dxcluster_hosts_save_setup(Fl_Button*, void*);
extern void dxcluster_hosts_send_setup(Fl_Button*, void*);
extern void dxcluster_chelp(Fl_Button*, void*);
extern void dxcluster_arc_help(Fl_Button*, void*);
extern void dxcluster_dxc_help(Fl_Button*, void*);
extern void dxcluser_servers(Fl_Button*, void*);
extern void dxcluster_hosts_load();

extern void dxc_lines();
extern void dxc_column_widths();

extern void dxc_click_m1(Fl_Button*, void*);
extern void dxc_click_m2(Fl_Button*, void*);
extern void dxc_click_m3(Fl_Button*, void*);
extern void dxc_click_m4(Fl_Button*, void*);
extern void dxc_click_m5(Fl_Button*, void*);
extern void dxc_click_m6(Fl_Button*, void*);
extern void dxc_click_m7(Fl_Button*, void*);
extern void dxc_click_m8(Fl_Button*, void*);

extern void send_name();
extern void send_qth();
extern void send_qra();

struct dxinfo {
	string freq;
	string call;
	string date;
	string time;
	string comment;
	string spotter;
	string call_country_code;
	string spotter_country_code;
	string originating_node;
	string call_itu_zone;
	string call_cq_zone;
	string spotter_itu_zone;
	string spotter_cq_zone;
	string call_US_state;
	string spotter_US_state;
	string call_country_text;
	string spotter_country_text;
	string call_grid_square;
	string spotter_grid_square;

	dxinfo() { clear(); } 
	void clear() {
		freq.clear();
		call.clear();
		date.clear();
		time.clear();
		comment.clear();
		spotter.clear();
		call_country_code.clear();
		spotter_country_code.clear();
		originating_node.clear();
		call_itu_zone.clear();
		call_cq_zone.clear();
		spotter_itu_zone.clear();
		spotter_cq_zone.clear();
		call_US_state.clear();
		spotter_US_state.clear();
		call_country_text.clear();
		spotter_country_text.clear();
		call_grid_square.clear();
		spotter_grid_square.clear();
	}
	~dxinfo() {}
};

extern bool dx_stream;

#endif
