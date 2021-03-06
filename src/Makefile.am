# Copyright (c) 2017 Dave Freese, W1HKJ (w1hkj AT bellsout DOT net)

bin_PROGRAMS = flcluster

MINGW32_SRC = flcluster-rc.rc include/flclusterrc.h \
	include/compat.h compat/getsysinfo.c compat/mingw.c compat/mingw.h

# Build the xmlrpcpp source if libflxmlrpc is not found
XMLRPCPP_SRC = \
	xmlrpcpp/XmlRpcBase64.h \
	xmlrpcpp/XmlRpcClient.cpp \
	xmlrpcpp/XmlRpcClient.h \
	xmlrpcpp/XmlRpcDispatch.cpp \
	xmlrpcpp/XmlRpcDispatch.h \
	xmlrpcpp/XmlRpcException.h \
	xmlrpcpp/XmlRpc.h \
	xmlrpcpp/XmlRpcMutex.cpp \
	xmlrpcpp/XmlRpcMutex.h \
	xmlrpcpp/XmlRpcServerConnection.cpp \
	xmlrpcpp/XmlRpcServerConnection.h \
	xmlrpcpp/XmlRpcServer.cpp \
	xmlrpcpp/XmlRpcServer.h \
	xmlrpcpp/XmlRpcServerMethod.cpp \
	xmlrpcpp/XmlRpcServerMethod.h \
	xmlrpcpp/XmlRpcSocket.cpp \
	xmlrpcpp/XmlRpcSocket.h \
	xmlrpcpp/XmlRpcSource.cpp \
	xmlrpcpp/XmlRpcSource.h \
	xmlrpcpp/XmlRpcUtil.cpp \
	xmlrpcpp/XmlRpcUtil.h \
	xmlrpcpp/XmlRpcValue.cpp \
	xmlrpcpp/XmlRpcValue.h

# We distribute these but do not always compile them
EXTRA_flcluster_SOURCES = $(FLCLUSTER_WIN32_RES_SRC) $(MINGW32_SRC) $(XMLRPCPP_SRC)

flcluster_SOURCES =

if !ENABLE_FLXMLRPC
  flcluster_SOURCES += $(XMLRPCPP_SRC)
  flcluster_CPPFLAGS = @FLCLUSTER_BUILD_CPPFLAGS@
  flcluster_CXXFLAGS = @FLCLUSTER_BUILD_CXXFLAGS@
  flcluster_CFLAGS = $(flcluster_CXXFLAGS)
  flcluster_LDFLAGS = @FLCLUSTER_BUILD_LDFLAGS@
  flcluster_LDADD = @FLCLUSTER_BUILD_LDADD@
else
  flcluster_CPPFLAGS = @FLCLUSTER_BUILD_CPPFLAGS@ @FLXMLRPC_CFLAGS@
  flcluster_CXXFLAGS = @FLCLUSTER_BUILD_CXXFLAGS@ @FLXMLRPC_CFLAGS@
  flcluster_CFLAGS = $(flcluster_CXXFLAGS)
  flcluster_LDFLAGS = @FLCLUSTER_BUILD_LDFLAGS@ @FLXMLRPC_CFLAGS@
  flcluster_LDADD = @FLCLUSTER_BUILD_LDADD@
endif

if MINGW32
if HAVE_WINDRES
  flcluster_SOURCES += $(MINGW32_SRC)
endif
endif

########################################################################


flcluster_VERSION_MAJOR = @FLCLUSTER_VERSION_MAJOR@
flcluster_VERSION_MINOR = @FLCLUSTER_VERSION_MINOR@
flcluster_VERSION_PATCH = @FLCLUSTER_VERSION_PATCH@
flcluster_VERSION = @FLCLUSTER_VERSION@

.EXPORT_ALL_VARIABLES: nsisinst appbundle

# Sources that are generated,
BUILT_SOURCES =
# not distributed,
nodist_flcluster_SOURCES = $(BUILT_SOURCES)
# and deleted by the clean targets
CLEANFILES = $(BUILT_SOURCES)
CLEAN_LOCAL =

if MINGW32
if HAVE_WINDRES
.rc.o:
	$(WINDRES) -Iinclude -I../data/win32 $< -O coff $@
endif
endif

install-data-local:
	if test -f ../data/flcluster.xpm; then \
	    $(mkinstalldirs) $(DESTDIR)/$(datadir)/pixmaps; \
	    $(INSTALL_DATA) ../data/flcluster.xpm $(DESTDIR)/$(datadir)/pixmaps; \
	fi
	if test -f ../data/flcluster.desktop; then \
	    $(mkinstalldirs) $(DESTDIR)/$(datadir)/applications; \
	    $(INSTALL_DATA) ../data/flcluster.desktop $(DESTDIR)/$(datadir)/applications; \
	fi

uninstall-local:
	rm -f $(DESTDIR)/$(datadir)/pixmaps/flcluster.xpm
	rm -f $(DESTDIR)/$(datadir)/applications/flcluster.desktop

FL_SRC = dxcluster/dx_dialog.fl

if HAVE_FLUID
flgen: $(FL_SRC)
	(cd include; \
	for f in $(FL_SRC); do \
	    c=$${f%.fl}.cxx; \
		h=$${f%.fl}.h; \
		h=$${h##*/}; \
	    $(FLUID) -c -o ../$$c -h $$h ../$$f; \
	done)
endif


INSTALLER_FILE = flcluster-$(flcluster_VERSION)_setup.exe
APPBUNDLE=flcluster-$(flcluster_VERSION)

if HAVE_NSIS
nsisinst: $(bin_PROGRAMS)
	sh ../scripts/mknsisinst.sh "../data" .
    CLEANFILES += $(INSTALLER_FILE)
endif

if DARWIN
appbundle: $(bin_PROGRAMS)
	sh ../scripts/mkappbundle.sh "../data" .
    CLEAN_LOCAL += $(APPBUNDLE) $(APPBUNDLE)*.dmg
endif

clean-local:
	-rm -rf $(CLEAN_LOCAL)

# Sources that we build. It is OK to have headers here.
flcluster_SOURCES += \
	flcluster.cxx \
	dxcluster/dx_dialog.cxx \
	dxcluster/dxcluster.cxx \
	utils/debug.cxx \
	utils/socket.cxx \
	utils/status.cxx \
	utils/threads.cxx \
	utils/timeops.cxx \
	utils/util.cxx \
	utils/xml_io.cxx \
	widgets/combo.cxx \
	widgets/date.cxx \
	widgets/fileselect.cxx \
	widgets/flinput2.cxx \
	widgets/flslider2.cxx \
	widgets/Fl_Text_Buffer_mod.cxx \
	widgets/Fl_Text_Editor_mod.cxx \
	widgets/Fl_Text_Display_mod.cxx \
	widgets/FTextView.cxx \
	widgets/icons.cxx \
	widgets/pixmaps.cxx \
	widgets/pixmaps_tango.cxx \
	widgets/font_browser.cxx
#	widgets/calendar.cxx

# Additional source files that are distributed
EXTRA_DIST = \
	config.h \
	compat/mingw.c \
	compat/mingw.h \
	compat/getsysinfo.c \
	compat/regex.c \
	compat/regex.h \
	widgets/flnfc_common.cxx \
	dxcluster/arc-help.cxx \
	dxcluster/CCC_Commands.cxx \
	dxcluster/DXClusterServers.cxx \
	dxcluster/DXSpiderCommandReference.cxx \
	dxcluster/dx_dialog.fl \
	dxcluster/DXSpiderCommandReference.html \
	include/ascii.h \
	include/calendar.h \
	include/combo.h \
	include/compat.h \
	include/date.h \
	include/debug.h \
	include/dirent-check.h \
	include/dx_dialog.h \
	include/dx_cluster.h \
	include/fileselect.h \
	include/fl_lock.h \
	include/flmisc.h \
	include/flclusterrc.h \
	include/flinput2.h \
	include/flslider2.h \
	include/font_browser.h \
	include/gettext.h \
	include/icons.h \
	include/flcluster.h \
	include/missing_strings.h \
	include/socket.h \
	include/pixmaps.h \
	include/status.h \
	include/threads.h \
	include/timeops.h \
	include/util.h \
	include/xml_io.h \
	widgets/Fl_Text_Buffer_mod.cxx \
	widgets/Fl_Text_Display_mod.cxx \
	widgets/Fl_Text_Editor_mod.cxx \
	include/Fl_Text_Buffer_mod.H \
	include/Fl_Text_Display_mod.H \
	include/Fl_Text_Editor_mod.H \
	include/FTextView.h

# Additional non-source files that are distributed
#EXTRA_DIST += \
#	../data/template.rtf
#	../flcluster-doc/flcluster-quick-guide.html

# Additional source files that support non-Linux cross compilation
EXTRA_DIST += \
	../ChangeLog \
	../data/flcluster.desktop \
	../data/flcluster.png \
	../data/win32/fl_app.nsi \
	../data/win32/flcluster.ico \
	../scripts/buildmxe.sh \
	../scripts/builddist.sh \
	../scripts/mkappbundle.sh \
	../scripts/mknsisinst.sh \
	../data/flcluster.xpm \
	../data/flcluster.png \
	../data/mac/Info.plist.in \
	../data/mac/flcluster.icns \
	$(flcluster_WIN32_SRC) \
	$(flcluster_FL_SRC)
