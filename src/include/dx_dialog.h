// generated by Fast Light User Interface Designer (fluid) version 1.0304

#ifndef dx_dialog_h
#define dx_dialog_h
#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_Group.H>
extern Fl_Group *btn_select_host;
#include "flinput2.h"
extern Fl_Input2 *inp_dxcc_host_url;
#include <FL/Fl_Button.H>
extern Fl_Button *btn_show_host_tab;
extern Fl_Input2 *inp_dccc_host_port;
extern Fl_Input2 *inp_dccc_login;
extern Fl_Input2 *inp_dxcc_password;
extern Fl_Button *btnDXCLUSTERpasswordShow;
#include <FL/Fl_Check_Button.H>
#include "status.h"
extern Fl_Check_Button *btn_dxcc_connect;
#include <FL/Fl_Box.H>
extern Fl_Box *lbl_dxc_connected;
extern Fl_Check_Button *btn_dxc_auto_connect;
#include <FL/Fl_Tabs.H>
extern Fl_Tabs *cluster_tabs;
extern Fl_Group *tabDXclusterTelNetStream;
extern Fl_Group *gp_resize_telnet;
#include "FTextView.h"
extern FTextView *brws_tcpip_stream;
extern void dxc_click_m1(Fl_Button*, void*);
extern Fl_Button *dxc_macro_1;
extern void dxc_click_m2(Fl_Button*, void*);
extern Fl_Button *dxc_macro_2;
extern void dxc_click_m3(Fl_Button*, void*);
extern Fl_Button *dxc_macro_3;
extern void dxc_click_m4(Fl_Button*, void*);
extern Fl_Button *dxc_macro_4;
extern void dxc_click_m5(Fl_Button*, void*);
extern Fl_Button *dxc_macro_5;
extern void dxc_click_m6(Fl_Button*, void*);
extern Fl_Button *dxc_macro_6;
extern void dxc_click_m7(Fl_Button*, void*);
extern Fl_Button *dxc_macro_7;
extern void dxc_click_m8(Fl_Button*, void*);
extern Fl_Button *dxc_macro_8;
extern Fl_Group *gp_spot_cmds;
extern Fl_Input2 *inp_dxcluster_cmd;
extern Fl_Button *btn_cluster_spot;
extern Fl_Button *btn_cluster_submit;
extern Fl_Group *tabDXclusterReports;
#include <FL/Fl_Browser.H>
extern Fl_Browser *reports_header;
extern Fl_Browser *brws_dx_cluster;
extern Fl_Button *btn_dxc_cluster_clear;
extern Fl_Check_Button *brws_order;
extern Fl_Button *btn_cluster_spot2;
#include <FL/Fl_Output.H>
extern Fl_Output *txtTOD;
extern Fl_Box *box_keepalive;
extern Fl_Button *btn_dxc_cluster_align;
extern Fl_Group *tabDXclusterConfig;
extern Fl_Group *cc_resize_1;
extern Fl_Browser *brws_dxcluster_hosts;
extern void dxcluster_hosts_select(Fl_Button*, void*);
extern Fl_Button *btn_dxcluster_hosts_select;
extern void dxcluster_hosts_add(Fl_Button*, void*);
extern Fl_Button *btn_dxcluster_hosts_add;
extern void dxcluster_hosts_delete(Fl_Button*, void*);
extern Fl_Button *btn_dxcluster_hosts_delete;
extern void dxcluster_servers(Fl_Button*, void*);
extern Fl_Button *btn_dxcluster_servers;
extern Fl_Group *gp_resize_dummy1;
extern FTextEdit *ed_telnet_cmds;
extern void dxcluster_hosts_load_setup(Fl_Button*, void*);
extern Fl_Button *btn_dxcluster_hosts_load_setup;
extern void dxcluster_hosts_save_setup(Fl_Button*, void*);
extern Fl_Button *btn_dxcluster_hosts_save_setup;
extern void dxcluster_hosts_send_setup(Fl_Button*, void*);
extern Fl_Button *btn_dxcluster_hosts_send_setup;
extern Fl_Group *gp_resize_dummy2;
extern Fl_Check_Button *btn_spot_when_logged;
extern Fl_Check_Button *btn_dxc_hertz;
#include <FL/Fl_Input.H>
extern Fl_Input *mlabel_1;
extern Fl_Input2 *mtext_1;
extern Fl_Input *mlabel_2;
extern Fl_Input2 *mtext_2;
extern Fl_Input *mlabel_3;
extern Fl_Input2 *mtext_3;
extern Fl_Input *mlabel_4;
extern Fl_Input2 *mtext_4;
extern Fl_Input *mlabel_5;
extern Fl_Input2 *mtext_5;
extern Fl_Input *mlabel_6;
extern Fl_Input2 *mtext_6;
extern Fl_Input *mlabel_7;
extern Fl_Input2 *mtext_7;
extern Fl_Input *mlabel_8;
extern Fl_Input2 *mtext_8;
extern void dxcluster_ar_help(Fl_Button*, void*);
extern Fl_Button *btn_dxcluster_ar_help;
extern void dxcluster_cc_help(Fl_Button*, void*);
extern Fl_Button *btn_dxcluster_cc_help;
extern void dxcluster_dx_help(Fl_Button*, void*);
extern Fl_Button *btn_dxcluster_dx_help;
extern Fl_Group *tabIOconfig;
extern Fl_Check_Button *btn_connect_to_fldigi;
extern Fl_Input *inp_fldigi_address;
extern Fl_Input *inp_fldigi_port;
extern Fl_Box *lbl_fldigi_connected;
#include <FL/Fl_Counter.H>
extern Fl_Counter *intPSKsweetspot;
extern Fl_Check_Button *btn_PSK_on_USB;
extern Fl_Counter *intCWsweetspot;
extern Fl_Check_Button *btn_cw_mode_is_USB;
extern Fl_Check_Button *btn_cw_mode_is_LSB;
extern Fl_Check_Button *btn_cw_mode_is_CW;
extern Fl_Check_Button *btn_rtty_is_mark;
extern Fl_Counter *intRTTYsweetspot;
extern Fl_Input *serversURL;
extern Fl_Input *AR_help_URL;
extern Fl_Input *CC_help_URL;
extern Fl_Input *DX_help_URL;
extern Fl_Group *gp_resize_io;
extern Fl_Group *tabUserConfig;
extern Fl_Input *inp_myCall;
extern Fl_Input *inp_myName;
extern Fl_Input *inp_myQth;
extern Fl_Input *inp_myLocator;
extern Fl_Button *btn_send_name;
extern Fl_Button *btn_send_qth;
extern Fl_Button *btn_send_qra;
extern Fl_Output *DXC_display;
#include "font_browser.h"
extern Fl_Button *btn_DXC_font;
extern Fl_Button *btnDXCdefault_colors_font;
#include <FL/fl_show_colormap.H>
extern Fl_Button *btn_DXC_even_lines;
extern Fl_Button *btn_DXC_odd_lines;
extern Fl_Input *StreamText;
extern Fl_Button *btnDXcolor;
extern Fl_Button *btnDXfont;
extern Fl_Button *btnDXalt_color;
extern Fl_Button *btnDXdefault_colors_font;
#include <FL/Fl_Round_Button.H>
extern Fl_Round_Button *btn_zero_keepalive;
extern Fl_Round_Button *btn_one_keepalive;
extern Fl_Round_Button *btn_five_keepalive;
extern Fl_Round_Button *btn_ten_keepalive;
extern Fl_Group *grp_misc;
#include <FL/Fl_Tooltip.H>
extern Fl_Check_Button *btn_tooltips;
extern Fl_Group *grp_resize_uc;
extern Fl_Group *tabDXclusterHelp;
extern FTextView *brws_dxc_help;
extern Fl_Button *btn_dxc_help_query;
extern Fl_Input2 *inp_help_string;
extern Fl_Button *btn_dxc_help_clear;
Fl_Double_Window* dxc_window();
extern unsigned char menu__i18n_done;
extern Fl_Menu_Item menu_[];
#define mnuFile (menu_+0)
#define mnuExit (menu_+1)
#define mnu_help (menu_+3)
#endif
