MINIJTAGSERV_BUILTINS_CPPFLAGS = -DWINDOWS=1 -DUNIX=2 -DLINUX=3
if IS_WIN32
MINIJTAGSERV_BUILTINS_CPPFLAGS += -DPORT=WINDOWS -DSYS=WINDOWS
else
MINIJTAGSERV_BUILTINS_CPPFLAGS += -DPORT=UNIX -DSYS=LINUX
endif

MINIJTAGSERV_GENERAL_CPPFLAGS = -DUSE_STD_NAMESPACE -DUSE_STD_IOSTREAMS -D_CRT_NONSTDC_NO_WARNINGS
MINIJTAGSERV_ALTERA_CPPFLAGS  = -DALTERA_NO_FSTREAM_WRAPPER  -DUSE_QT5 -DMODE_64_BIT \
    -DCODE_PREFIX_RAW=JTAG \
    -DJTAG_DLLEXPORT=ALTERA_DLLEXPORT \
    -DJTAG_DLLPRIVATE=ALTERA_PRIVATE \
    -DJTAG_DLLEXPORT_CALLBACK=ALTERA_DLLEXPORT_CALLBACK \
    -DALTERA_NO_MEM_MANAGER \
    -DMEM_TRACKER_TEMPLATE=MEM_TRACKER_TEMPLATE_JTAG \
    -Dmem_get_tracker_thunk=mem_get_tracker_thunk_jtag \
    -Dmsg_vdebug=jtag_msg_debug \
    -Dmem_msg_vdebug=jtag_mem_msg_debug \
    -DFITTER_USE_DRL \
    -DSDB_USE_DRL \
    -DGEN_STRING_IMPL=1 \
    -D_FILE_OFFSET_BITS=64 \
    -DALTERA_DEFINE_MACRO_BEFORE_USE  

MINIJTAGSERV_MISC_FLAGS = -Wno-error=coverage-mismatch    -fPIC -fvisibility=hidden -ggdb1 -O1 -mmmx -msse \
    -fno-strict-aliasing -finline -fno-inline-functions -fno-builtin -momit-leaf-frame-pointer -fomit-frame-pointer \
    -funwind-tables -Wall -Wno-system-headers -Wno-extra -Wno-switch -Wno-unused-function -Wno-unknown-pragmas \
    -Wno-comment -Wno-parentheses -Wno-strict-overflow -Werror -Wno-error=deprecated-declarations -fopenmp \
    -DBOOST_NO_AUTO_PTR -DBOOST_SYSTEM_NO_DEPRECATED -D_GLIBCXX_USE_CXX11_ABI=0 -Wno-misleading-indentation \
    -Wno-deprecated -Wno-attributes -Wno-unused-but-set-variable -Wno-nonnull-compare -Wno-unused-const-variable \
    -D_GNU_SOURCE -D_REENTRANT -flifetime-dse=1 -frecord-gcc-switches -fstack-protector-strong \
    -Wformat-security -fPIE -fPIC -D_FORTIFY_SOURCE=2 -Werror=delete-incomplete

## @FIXME Now disabling print string format check on windows to avoid problem with 
## src/jtag/minijtagserv/minijtagserv.c:858:16: warning: format '%u' expects argument of type 'unsigned int', 
##     but argument 7 has type 'DWORD' {aka 'long unsigned int'} [-Wformat=]
##  858 |      LOG_ERROR("Expecting SOCVHPS to be used, i.e. tap_position %d, but got tap position %u instead",
##      |                ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
##  859 |                  jtagservice.in_use_device_tap_position,
##  860 |                tap_position
##      |                ~~~~~~~~~~~~
##      |                |
##      |                DWORD {aka long unsigned int}
##
##  The reason for this is the formating string is written for with DWORD for linux in mind, this causes problem when 
##  DWORD for windows is a different datatype from linux

if !IS_WIN32
MINIJTAGSERV_MISC_FLAGS += -Wformat
endif



noinst_LTLIBRARIES += %D%/libocdminijtagserv.la

%C%_libocdminijtagserv_la_SOURCES = 
%C%_libocdminijtagserv_la_CPPFLAGS =
%C%_libocdminijtagserv_la_LDFLAGS = 
%C%_libocdminijtagserv_la_LDLIBS =
%C%_libocdminijtagserv_la_LIBADD =

include %D%/aji/Makefile.am
%C%_libocdminijtagserv_la_LIBADD += $(top_builddir)/%D%/aji/libocdcaji.la


%C%_libocdminijtagserv_la_SOURCES += $(MINIJTAGSERV_SRC)

%C%_libocdminijtagserv_la_CPPFLAGS += -I%D%/h -I%D%/.. \
    $(AM_CPPFLAGS) \
    $(MINIJTAGSERV_BUILTINS_CPPFLAGS) $(MINIJTAGSERV_GENERAL_CPPFLAGS) $(MINIJTAGSERV_ALTERA_CPPFLAGS) \
    $(MINIJTAGSERV_MISC_FLAGS) 

%C%_libocdminijtagserv_la_LDFLAGS += -Wl,-fuse-ld=gold -z noexecstack -z relro -rdynamic \
    -fopenmp \
    -static-libgcc 

MINIJTAGSERV_SRC = \
    %D%/aji/aji.h %D%/aji/c_aji.h  %D%/c2_aji.h \
    %D%/aji/c_jtag_client_gnuaji.h  \
    %D%/jtagservice.h %D%/jtagservice.c \
    %D%/minijtagserv.c 

#if IS_WIN32
MINIJTAGSERV_SRC += %D%/aji/jtag_client_gnuaji_win64.h %D%/aji/c_jtag_client_gnuaji_win64.h
#else
#MINIJTAGSERV_SRC += %D%/aji/jtag_client_gnuaji_lib64.h %D%/aji/c_jtag_client_gnuaji_lib64.h
#endif

#@TODO During installation, copy all the dependent libraries
#      to its final location
