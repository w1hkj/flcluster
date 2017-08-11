// =====================================================================
//
// status.h
//
// Author(s):
// 	Dave Freese, W1HKJ Copyright (C) 2010
//	Robert Stiles, KK5VD Copyright (C) 2013
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

#ifndef _status_H
#define _status_H

#include <string>

using namespace std;

struct status {
	int		mainX;
	int		mainY;

	std::string myCall;
	std::string myName;
	std::string myQth;
	std::string myLocator;

	std::string dxcluster_hosts;
	std::string dxcc_host_url;
	std::string dxcc_host_port;
	std::string dxcc_login;
	std::string dxcc_password;

	bool cluster_connected;
	bool dxc_auto_connect;
	bool dxc_topline;
	bool spot_when_logged;
	bool dxc_hertz;

	std::string dxcm_label_1;
	std::string dxcm_label_2;
	std::string dxcm_label_3;
	std::string dxcm_label_4;
	std::string dxcm_label_5;
	std::string dxcm_label_6;
	std::string dxcm_label_7;
	std::string dxcm_label_8;

	std::string dxcm_text_1;
	std::string dxcm_text_2;
	std::string dxcm_text_3;
	std::string dxcm_text_4;
	std::string dxcm_text_5;
	std::string dxcm_text_6;
	std::string dxcm_text_7;
	std::string dxcm_text_8;

	int DX_Color_R;
	int DX_Color_G;
	int DX_Color_B;

	Fl_Color DXC_even_color;
	Fl_Color DXC_odd_color;
	Fl_Color DXC_textcolor;

	std::string DXC_textname;
	Fl_Font  DXC_textfont;
	int DXC_textsize;

	Fl_Color  DXfontcolor;
	Fl_Color  DXalt_color;

	std::string  DXfontname;
	Fl_Font DXfontnbr;
	int DXfontsize;

	Fl_Color  TabsColor;

// fldigi control parameters
	bool connect_to_fldigi;
	std::string fldigi_address;
	std::string fldigi_port;
	int CWsweetspot;
	int PSKsweetspot;
	int RTTYsweetspot;
	bool useMARKfreq;
	bool USB;
	bool USBCW;
	bool LSBCW;

	bool connect_to_flrig;
	std::string flrig_address;
	std::string flrig_port;

	bool changed;
	void saveLastState();
	void loadLastState();
};

extern status progStatus;

#endif
