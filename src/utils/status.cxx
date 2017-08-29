// =====================================================================
//
// status.cxx
//
// Author(s):
//	Dave Freese, W1HKJ Copyright (C) 2017
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


#include <iostream>
#include <fstream>
#include <string>

#include <FL/Fl.H>
#include <FL/Fl_Preferences.H>

#include "status.h"
#include "config.h"
#include "flcluster.h"
#include "dx_dialog.h"

status progStatus = {
	50,				// int mainX;
	50,				// int mainY;
	640,			// int mainW;
	420,			// int mainH;

	"call sign",	// std::string myCall;
	"name",			// std::string myName;
	"city, state",	// std::string myQth;
	"maidenhead",	// std::string myLocator;

	"",				// std::string dxcluster_hosts
	"",				// std::string dxcc_host_url;
	"",				// std::string dxcc_host_port;
	"",				// std::string dxcc_login;
	"",				// std::string dxcc_password;

	false,			//bool cluster_connected;
	false,			//bool dxc_auto_connect;
	false,			//bool dxc_topline;
	false,			//bool spot_when_logged;
	false,			//bool dxc_hertz;

	"",				// std::string dxcm_label_1;
	"",				// std::string dxcm_label_2;
	"",				// std::string dxcm_label_3;
	"",				// std::string dxcm_label_4;
	"",				// std::string dxcm_label_5;
	"",				// std::string dxcm_label_6;
	"",				// std::string dxcm_label_7;
	"",				// std::string dxcm_label_8;

	"",				// std::string dxcm_text_1;
	"",				// std::string dxcm_text_2;
	"",				// std::string dxcm_text_3;
	"",				// std::string dxcm_text_4;
	"",				// std::string dxcm_text_5;
	"",				// std::string dxcm_text_6;
	"",				// std::string dxcm_text_7;
	"",				// std::string dxcm_text_8;

	5,				// bool keepalive;

	0,				// DX_Color_R;
	0,				// DX_Color_G;
	130,			// DX_Color_B;

	7,				// Fl_Color DXC_even_color
	246,			// Fl_Color DXC_odd_color
	FL_YELLOW,		// Fl_Color DXC_textcolor

	FL_COURIER,		// Fl_Font  DXC_textfont
	14,				// int DXC_textsize

	FL_YELLOW,		// Fl_Color  DXfontcolor
	FL_DARK_RED,	// Fl_Color, DXalt_color

	FL_COURIER,		// Fl_Font DXfontnbr
	14,				// int DXfontsize

	FL_WHITE,		// Fl_Color  TabsColor;

// fldigi control parameters
	false,			// bool connect_to_fldigi;
	"127.0.0.1",	// std::string fldigi_address;
	"7362",			// std::string fldigi_port;
	700,			// bool CWsweetspot;
	1500,			// bool PSKsweetspot;
	1500,			// bool RTTYsweetspot;
	1,				// bool useMARKfreq;
	1,				// bool USB;
	0,				// bool USBCW;
	0,				// bool LSBCW;

	false,			// bool connect_to_flrig;
	"127.0.0.1",	// std::string flrig_address;
	"12345",		// std::string flrig_port;

	true,			// bool tooltips
	false,			// bool changed;

};

/** ********************************************************
 *
 ***********************************************************/
void status::saveLastState()
{
	Fl_Preferences FLCLUSTERpref(HOME_DIR.c_str(), "w1hkj.com",  PACKAGE_NAME);

	int mX = main_window->x();
	int mY = main_window->y();
	mainW = main_window->w();
	mainH = main_window->h();
	if (mX >= 0 && mX >= 0) {
		mainX = mX;
		mainY = mY;
	}

	FLCLUSTERpref.set("version", PACKAGE_VERSION);
	FLCLUSTERpref.set("mainx", mainX);
	FLCLUSTERpref.set("mainy", mainY);
	FLCLUSTERpref.set("mainw", mainW);
	FLCLUSTERpref.set("mainh", mainH);

	FLCLUSTERpref.set("dxcc_host_url", dxcc_host_url.c_str());
	FLCLUSTERpref.set("dxcc_host_port", dxcc_host_port.c_str());
	FLCLUSTERpref.set("dxcc_login", dxcc_login.c_str());
	FLCLUSTERpref.set("dxcc_password", dxcc_password.c_str());
	FLCLUSTERpref.set("dxcluster_hosts", dxcluster_hosts.c_str());

	FLCLUSTERpref.set("myCall", myCall.c_str());
	FLCLUSTERpref.set("myName", myName.c_str());
	FLCLUSTERpref.set("myQth", myQth.c_str());
	FLCLUSTERpref.set("myLocator", myLocator.c_str());

	FLCLUSTERpref.set("dxcm_label_1", dxcm_label_1.c_str());
	FLCLUSTERpref.set("dxcm_label_2", dxcm_label_2.c_str());
	FLCLUSTERpref.set("dxcm_label_3", dxcm_label_3.c_str());
	FLCLUSTERpref.set("dxcm_label_4", dxcm_label_4.c_str());
	FLCLUSTERpref.set("dxcm_label_5", dxcm_label_5.c_str());
	FLCLUSTERpref.set("dxcm_label_6", dxcm_label_6.c_str());
	FLCLUSTERpref.set("dxcm_label_7", dxcm_label_7.c_str());
	FLCLUSTERpref.set("dxcm_label_8", dxcm_label_8.c_str());

	FLCLUSTERpref.set("dxcm_text_1", dxcm_text_1.c_str());
	FLCLUSTERpref.set("dxcm_text_2", dxcm_text_2.c_str());
	FLCLUSTERpref.set("dxcm_text_3", dxcm_text_3.c_str());
	FLCLUSTERpref.set("dxcm_text_4", dxcm_text_4.c_str());
	FLCLUSTERpref.set("dxcm_text_5", dxcm_text_5.c_str());
	FLCLUSTERpref.set("dxcm_text_6", dxcm_text_6.c_str());
	FLCLUSTERpref.set("dxcm_text_7", dxcm_text_7.c_str());
	FLCLUSTERpref.set("dxcm_text_8", dxcm_text_8.c_str());

	FLCLUSTERpref.set("keepalive", keepalive);

	FLCLUSTERpref.set("cluster_connected", cluster_connected);
	FLCLUSTERpref.set("dxc_auto_connect", dxc_auto_connect);
	FLCLUSTERpref.set("dxc_topline", dxc_topline);
	FLCLUSTERpref.set("spot_when_logged", spot_when_logged);
	FLCLUSTERpref.set("dxc_hertz", dxc_hertz);

	FLCLUSTERpref.set("DX_Color_R", (int)DX_Color_R);
	FLCLUSTERpref.set("DX_Color_G", (int)DX_Color_G);
	FLCLUSTERpref.set("DX_Color_B", (int)DX_Color_B);

	FLCLUSTERpref.set("DXC_even_color", (int)DXC_even_color);
	FLCLUSTERpref.set("DXC_odd_color", (int)DXC_odd_color);
	FLCLUSTERpref.set("DXC_textcolor", (int)DXC_textcolor);

	FLCLUSTERpref.set("DXC_textfont", DXC_textfont);
	FLCLUSTERpref.set("DXC_textsize", DXC_textsize);

	FLCLUSTERpref.set("DXfontcolor", (int)DXfontcolor);
	FLCLUSTERpref.set("DXalt_color", (int)DXalt_color);

	FLCLUSTERpref.set("DXfontnbr", DXfontnbr);
	FLCLUSTERpref.set("DXfontsize", DXfontsize);

	FLCLUSTERpref.set("TabsColor", (int)TabsColor);

// fldigi control parameters
	FLCLUSTERpref.set("connect_to_fldigi", connect_to_fldigi);
	FLCLUSTERpref.set("fldigi_address", fldigi_address.c_str());
	FLCLUSTERpref.set("fldigi_port", fldigi_port.c_str());
	FLCLUSTERpref.set("CWsweetspot", CWsweetspot);
	FLCLUSTERpref.set("PSKsweetspot", PSKsweetspot);
	FLCLUSTERpref.set("RTTYsweetspot", RTTYsweetspot);
	FLCLUSTERpref.set("useMARKfreq", useMARKfreq);
	FLCLUSTERpref.set("USB", USB);
	FLCLUSTERpref.set("USBCW", USBCW);
	FLCLUSTERpref.set("LSBCW", LSBCW);
// flrig control parameters
	FLCLUSTERpref.set("connect_to_flrig", connect_to_flrig);
	FLCLUSTERpref.set("flrig_address", flrig_address.c_str());
	FLCLUSTERpref.set("flrig_port", flrig_port.c_str());

	FLCLUSTERpref.set("tooltips", tooltips);
}

/** ********************************************************
 *
 ***********************************************************/
void status::loadLastState()
{
	Fl_Preferences FLCLUSTERpref(HOME_DIR.c_str(), "w1hkj.com", PACKAGE_NAME);

	if (FLCLUSTERpref.entryExists("version")) {
		char *defbuffer;

		FLCLUSTERpref.get("mainx", mainX, mainX);
		FLCLUSTERpref.get("mainy", mainY, mainY);
		FLCLUSTERpref.get("mainw", mainW, mainW);
		FLCLUSTERpref.get("mainh", mainH, mainH);

		FLCLUSTERpref.get("dxcc_host_url", defbuffer, "");
		dxcc_host_url = defbuffer; free(defbuffer);

		FLCLUSTERpref.get("dxcc_host_port", defbuffer, "");
		dxcc_host_port = defbuffer; free(defbuffer);

		FLCLUSTERpref.get("dxcc_login", defbuffer, "");
		dxcc_login = defbuffer; free(defbuffer);

		FLCLUSTERpref.get("dxcc_password", defbuffer, "");
		dxcc_password = defbuffer; free(defbuffer);

		FLCLUSTERpref.get("dxcluster_hosts", defbuffer, "");
		dxcluster_hosts = defbuffer; free(defbuffer);

		FLCLUSTERpref.get("myCall", defbuffer, "");
		myCall = defbuffer; free(defbuffer);

		FLCLUSTERpref.get("myName", defbuffer, "");
		myName = defbuffer; free(defbuffer);

		FLCLUSTERpref.get("myQth", defbuffer, "");
		myQth = defbuffer; free(defbuffer);

		FLCLUSTERpref.get("myLocator", defbuffer, "");
		myLocator = defbuffer; free(defbuffer);

		FLCLUSTERpref.get("dxcm_label_1", defbuffer, "");
		dxcm_label_1 = defbuffer; free(defbuffer);

		FLCLUSTERpref.get("dxcm_label_2", defbuffer, "");
		dxcm_label_2 = defbuffer; free(defbuffer);

		FLCLUSTERpref.get("dxcm_label_3", defbuffer, "");
		dxcm_label_3 = defbuffer; free(defbuffer);

		FLCLUSTERpref.get("dxcm_label_4", defbuffer, "");
		dxcm_label_4 = defbuffer; free(defbuffer);

		FLCLUSTERpref.get("dxcm_label_5", defbuffer, "");
		dxcm_label_5 = defbuffer; free(defbuffer);

		FLCLUSTERpref.get("dxcm_label_6", defbuffer, "");
		dxcm_label_6 = defbuffer; free(defbuffer);

		FLCLUSTERpref.get("dxcm_label_7", defbuffer, "");
		dxcm_label_7 = defbuffer; free(defbuffer);

		FLCLUSTERpref.get("dxcm_label_8", defbuffer, "");
		dxcm_label_8 = defbuffer; free(defbuffer);

		FLCLUSTERpref.get("dxcm_text_1", defbuffer, "");
		dxcm_text_1 = defbuffer; free(defbuffer);

		FLCLUSTERpref.get("dxcm_text_2", defbuffer, "");
		dxcm_text_2 = defbuffer; free(defbuffer);

		FLCLUSTERpref.get("dxcm_text_3", defbuffer, "");
		dxcm_text_3 = defbuffer; free(defbuffer);

		FLCLUSTERpref.get("dxcm_text_4", defbuffer, "");
		dxcm_text_4 = defbuffer; free(defbuffer);

		FLCLUSTERpref.get("dxcm_text_5", defbuffer, "");
		dxcm_text_5 = defbuffer; free(defbuffer);

		FLCLUSTERpref.get("dxcm_text_6", defbuffer, "");
		dxcm_text_6 = defbuffer; free(defbuffer);

		FLCLUSTERpref.get("dxcm_text_7", defbuffer, "");
		dxcm_text_7 = defbuffer; free(defbuffer);

		FLCLUSTERpref.get("dxcm_text_8", defbuffer, "");
		dxcm_text_8 = defbuffer; free(defbuffer);

		int i = 0;

		FLCLUSTERpref.get("keepalive", keepalive, keepalive);

		FLCLUSTERpref.get("cluster_connected", i, cluster_connected);
		cluster_connected = i;

		FLCLUSTERpref.get("dxc_auto_connect", i, dxc_auto_connect);
		dxc_auto_connect = i;

		FLCLUSTERpref.get("dxc_topline", i, dxc_topline);
		dxc_topline = i;

		FLCLUSTERpref.get("spot_when_logged", i, spot_when_logged);
		spot_when_logged = i;

		FLCLUSTERpref.get("dxc_hertz", i, dxc_hertz);
		dxc_hertz = i;

		FLCLUSTERpref.get("DX_Color_R", i, (int)DX_Color_R);
		DX_Color_R = i;
		FLCLUSTERpref.get("DX_Color_G", i, (int)DX_Color_G);
		DX_Color_G = i;
		FLCLUSTERpref.get("DX_Color_B", i, (int)DX_Color_B);
		DX_Color_B = i;

		FLCLUSTERpref.get("DXC_even_color", i, (int)DXC_even_color);
		DXC_even_color = i;
		FLCLUSTERpref.get("DXC_odd_color", i, (int)DXC_odd_color);
		DXC_odd_color = i;
		FLCLUSTERpref.get("DXC_textcolor", i, (int)DXC_textcolor);
		DXC_textcolor = i;

		FLCLUSTERpref.get("DXC_textfont", i, DXC_textfont);
		DXC_textfont = i;
		FLCLUSTERpref.get("DXC_textsize", i, DXC_textsize);
		DXC_textsize = i;

		FLCLUSTERpref.get("DXfontcolor", i, (int)DXfontcolor);
		DXfontcolor = i;
		FLCLUSTERpref.get("DXalt_color", i, (int)DXalt_color);
		DXalt_color = i;

		FLCLUSTERpref.get("DXfontnbr", i, DXfontnbr);
		DXfontnbr = i;
		FLCLUSTERpref.get("DXfontsize", i, DXfontsize);
		DXfontsize = i;

		FLCLUSTERpref.get("TabsColor", i, (int)TabsColor);
		TabsColor = i;

// fldigi control parameters
		FLCLUSTERpref.get("connect_to_fldigi", i, connect_to_fldigi);
		connect_to_fldigi = i;
		FLCLUSTERpref.get("fldigi_address",  defbuffer, "");
		fldigi_address = defbuffer; free(defbuffer);
		FLCLUSTERpref.get("fldigi_port",  defbuffer, "");
		fldigi_port = defbuffer; free(defbuffer);
		FLCLUSTERpref.get("CWsweetspot", i, CWsweetspot);
		CWsweetspot = i;
		FLCLUSTERpref.get("PSKsweetspot", i, PSKsweetspot);
		PSKsweetspot = i;
		FLCLUSTERpref.get("RTTYsweetspot", i, RTTYsweetspot);
		RTTYsweetspot = i;
		FLCLUSTERpref.get("useMARKfreq", i, useMARKfreq);
		useMARKfreq = i;
		FLCLUSTERpref.get("USB", i, USB);
		USB = i;
		FLCLUSTERpref.get("USBCW", i, USBCW);
		USBCW = i;
		FLCLUSTERpref.get("LSBCW", i, LSBCW);
		LSBCW = i;
// flrig control parameters
		FLCLUSTERpref.get("connect_to_flrig", i, connect_to_flrig);
		connect_to_flrig = i;
		FLCLUSTERpref.get("flrig_address",  defbuffer, "");
		flrig_address = defbuffer; free(defbuffer);
		FLCLUSTERpref.get("flrig_port",  defbuffer, "");
		flrig_port = defbuffer; free(defbuffer);

		FLCLUSTERpref.get("tooltips", i, tooltips);
		tooltips = i;

	}
}
