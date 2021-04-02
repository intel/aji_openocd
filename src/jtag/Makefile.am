noinst_LTLIBRARIES += %D%/libjtag.la

JTAG_SRCS = %D%/commands.c
%C%_libjtag_la_LIBADD =

BUILT_SOURCES += %D%/minidriver_imp.h
CLEANFILES += %D%/minidriver_imp.h

if MINIDRIVER

if AJI_CLIENT
include %D%/aji_client/Makefile.am
JTAG_MINIDRIVER_DIR = %D%/aji_client
%C%_libjtag_la_LIBADD += $(top_builddir)/%D%/aji_client/libocdaji_client.la
MINIDRIVER_IMP_DIR = %D%/aji_client
endif

%D%/jtag_minidriver.h: $(JTAG_MINIDRIVER_DIR)/jtag_minidriver.h
	cp $< $@

BUILT_SOURCES += %D%/jtag_minidriver.h

CLEANFILES += %D%/jtag_minidriver.h

else

MINIDRIVER_IMP_DIR = %D%/drivers

if HLADAPTER
include %D%/hla/Makefile.am
%C%_libjtag_la_LIBADD += $(top_builddir)/%D%/hla/libocdhla.la
endif

if AICE
include %D%/aice/Makefile.am
%C%_libjtag_la_LIBADD += $(top_builddir)/%D%/aice/libocdaice.la
endif

include %D%/drivers/Makefile.am
%C%_libjtag_la_LIBADD += $(top_builddir)/%D%/drivers/libocdjtagdrivers.la

endif

%D%/minidriver_imp.h: $(MINIDRIVER_IMP_DIR)/minidriver_imp.h
	cp $< $@

%C%_libjtag_la_SOURCES = \
	%D%/adapter.c \
	%D%/core.c \
	%D%/interface.c \
	%D%/interfaces.c \
	%D%/tcl.c \
	%D%/swim.c \
	%D%/commands.h \
	%D%/driver.h \
	%D%/interface.h \
	%D%/interfaces.h \
	%D%/minidriver.h \
	%D%/jtag.h \
	%D%/swd.h \
	%D%/swim.h \
	%D%/tcl.h \
	$(JTAG_SRCS)

STARTUP_TCL_SRCS += %D%/startup.tcl
