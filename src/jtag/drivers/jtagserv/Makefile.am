noinst_LTLIBRARIES += %D%/libocdjtagserv.la
%C%_libocdjtagserv_la_SOURCES = $(JTAGSERV_SRC)
%C%_libocdjtagserv_la_CPPFLAGS = -I$(top_srcdir)/src/jtag/drivers $(AM_CPPFLAGS) $(LIBUSB1_CFLAGS) $(LIBFTDI_CFLAGS)

JTAGSERV_SRC = %D%/jtagserv.c %D%/jtagserv_access.h


if JTAGSERV
JTAGSERV_SRC += %D%/jtagserv_access_libusb.c
endif
