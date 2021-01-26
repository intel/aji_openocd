noinst_LTLIBRARIES += %D%/libocdaji_client.la

%C%_libocdaji_client_la_SOURCES = 
%C%_libocdaji_client_la_CPPFLAGS =
%C%_libocdaji_client_la_LDFLAGS = 
%C%_libocdaji_client_la_LDLIBS =
%C%_libocdaji_client_la_LIBADD =


%C%_libocdaji_client_la_CPPFLAGS += -DWINDOWS=1 -DUNIX=2 -DLINUX=3
if IS_WIN32
%C%_libocdaji_client_la_CPPFLAGS += -DPORT=WINDOWS -DSYS=WINDOWS
else
%C%_libocdaji_client_la_CPPFLAGS += -DPORT=UNIX -DSYS=LINUX
endif

%C%_libocdaji_client_la_CPPFLAGS += \
    -DMODE_64_BIT \
    \
    -D_FORTIFY_SOURCE=2  -O1 \
    -D_GNU_SOURCE -D_REENTRANT \
    \
    -Wall \
    -Wno-error=coverage-mismatch -fPIC -ggdb1 -mmmx -msse \
    -fno-strict-aliasing -finline -fno-inline-functions -fno-builtin -momit-leaf-frame-pointer -fomit-frame-pointer \
    -funwind-tables -Wno-system-headers -Wno-extra -Wno-switch -Wno-unused-function -Wno-unknown-pragmas \
    -Wno-comment -Wno-parentheses -Wno-strict-overflow -Werror -Wno-error=deprecated-declarations -fopenmp \
    -Wno-misleading-indentation \
    -Wno-deprecated -Wno-attributes -Wno-unused-but-set-variable -Wno-nonnull-compare -Wno-unused-const-variable \
    -flifetime-dse=1 -frecord-gcc-switches -fstack-protector-strong \
    -Wformat-security -fPIE -fPIC  -Werror=delete-incomplete

if !IS_WIN32
%C%_libocdaji_client_la_CPPFLAGS += -Wformat
endif



include %D%/aji/Makefile.am
%C%_libocdaji_client_la_LIBADD += $(top_builddir)/%D%/aji/libocdcaji.la


%C%_libocdaji_client_la_SOURCES += $(JCLIENT_SRC)

%C%_libocdaji_client_la_CPPFLAGS += -I%D%/h -I%D%/.. \
    $(AM_CPPFLAGS)  

%C%_libocdaji_client_la_LDFLAGS += -Wl,-fuse-ld=gold -z noexecstack -z relro -rdynamic \
    -fopenmp \
    -static-libgcc 

JCLIENT_SRC = \
    %D%/aji/aji.h %D%/aji/c_aji.h \
    %D%/aji/c_jtag_client_gnuaji.h  \
    %D%/jtagservice.h %D%/jtagservice.c \
    %D%/aji_client.c 

#if IS_WIN32
JCLIENT_SRC += %D%/aji/jtag_client_gnuaji_win64.h %D%/aji/c_jtag_client_gnuaji_win64.h
#else
JCLIENT_SRC += %D%/aji/jtag_client_gnuaji_lib64.h %D%/aji/c_jtag_client_gnuaji_lib64.h
#endif

#@TODO During installation, copy all the dependent libraries
#      to its final location
