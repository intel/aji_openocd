noinst_LTLIBRARIES += %D%/libocdcaji.la

%C%_libocdcaji_la_SOURCES =
%C%_libocdcaji_la_CPPFLAGS =


CAJI_BUILTINS_CPPFLAGS = -DWINDOWS=1 -DUNIX=2 -DLINUX=3
if IS_WIN32
CAJI_BUILTINS_CPPFLAGS += -DPORT=WINDOWS -DSYS=WINDOWS
else
CAJI_BUILTINS_CPPFLAGS += -DPORT=UNIX -DSYS=LINUX
endif

CAJI_GENERAL_CPPFLAGS = -DUSE_STD_NAMESPACE -DUSE_STD_IOSTREAMS -D_CRT_NONSTDC_NO_WARNINGS
CAJI_ALTERA_CPPFLAGS  = -DALTERA_NO_FSTREAM_WRAPPER  -DUSE_QT5 -DMODE_64_BIT \
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

CAJI_MISC_FLAGS = -Wno-error=coverage-mismatch    -fPIC -fvisibility=hidden -ggdb1 -O1 -mmmx -msse \
    -fno-strict-aliasing -finline -fno-inline-functions -fno-builtin -momit-leaf-frame-pointer -fomit-frame-pointer \
    -funwind-tables -Wall -Wno-system-headers -Wno-extra -Wno-switch -Wno-unused-function -Wno-unknown-pragmas \
    -Wno-comment -Wno-parentheses -Wno-strict-overflow -Werror -Wno-error=deprecated-declarations -fopenmp \
    -DBOOST_NO_AUTO_PTR -DBOOST_SYSTEM_NO_DEPRECATED -D_GLIBCXX_USE_CXX11_ABI=0 -Wno-misleading-indentation \
    -Wno-deprecated -Wno-attributes -Wno-unused-but-set-variable -Wno-nonnull-compare -Wno-unused-const-variable \
    -D_GNU_SOURCE -D_REENTRANT -flifetime-dse=1 -frecord-gcc-switches -fstack-protector-strong \
    -Wformat -Wformat-security -fPIE -fPIC -D_FORTIFY_SOURCE=2 -Werror=delete-incomplete


%C%_libocdcaji_la_CPPFLAGS += \
    $(AM_CPPFLAGS) $(MINIJTAGSERV_BUILTINS_CPPFLAGS) \
    $(CAJI_BUILTINS_CPPFLAGS) $(CAJI_SERV_GENERAL_CPPFLAGS) $(CAJI_ALTERA_CPPFLAGS) \
    $(CAJI_MISC_FLAGS) 

%C%_libocdcaji_la_SOURCES += \
    %D%/aji.h %D%/c_aji.h \
    %D%/jtag_client_gnuaji.h \
    %D%/c_jtag_client_gnuaji.h \
    %D%/c_jtag_client_gnuaji.c

if IS_WIN32
%C%_libocdcaji_la_SOURCES  += %D%/c_jtag_client_gnuaji_win64.h %D%/c_jtag_client_gnuaji_win64.c
else
%C%_libocdcaji_la_SOURCES  += %D%/c_jtag_client_gnuaji_lib64.h %D%/c_jtag_client_gnuaji_lib64.c
endif
