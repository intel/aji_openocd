noinst_LTLIBRARIES += %D%/libocdcaji.la

%C%_libocdcaji_la_SOURCES = 
%C%_libocdcaji_la_CPPFLAGS =
%C%_libocdcaji_la_LDFLAGS = 
%C%_libocdcaji_la_LDLIBS =
%C%_libocdcaji_la_LIBADD =

%C%_libocdcaji_la_CPPFLAGS += -DWINDOWS=1 -DUNIX=2 -DLINUX=3
if IS_WIN32
%C%_libocdcaji_la_CPPFLAGS += -DPORT=WINDOWS -DSYS=WINDOWS
else
%C%_libocdcaji_la_CPPFLAGS += -DPORT=UNIX -DSYS=LINUX
endif

%C%_libocdcaji_la_CPPFLAGS += \
    $(AM_CPPFLAGS)

# Compile: For configuring AJI Client Driver
%C%_libocdcaji_la_CPPFLAGS += \
    -DMODE_64_BIT

# Compile: For optimizing AJI Client Driver
%C%_libocdcaji_la_CPPFLAGS += \
    -O1 \
    -D_GNU_SOURCE -D_REENTRANT
    
# Compile: Preferred flags used for optimizing AJI Client driver
%C%_libocdcaji_la_CPPFLAGS += \
    -Wall \
    -Wno-error=coverage-mismatch -fPIC -ggdb1 -mmmx -msse \
    -fno-strict-aliasing -finline -fno-inline-functions -fno-builtin -momit-leaf-frame-pointer -fomit-frame-pointer \
    -funwind-tables -Wno-system-headers -Wno-extra -Wno-switch -Wno-unused-function -Wno-unknown-pragmas \
    -Wno-comment -Wno-parentheses -Wno-strict-overflow -Werror -Wno-error=deprecated-declarations -fopenmp \
    -Wno-misleading-indentation \
    -Wno-deprecated -Wno-attributes -Wno-unused-but-set-variable -Wno-nonnull-compare -Wno-unused-const-variable \
    -flifetime-dse=1 -frecord-gcc-switches \
    -Werror=delete-incomplete

##Compile: Others
%C%_libocdcaji_la_CPPFLAGS  += \
    -Wformat -Wformat-security -Werror=format-security \
    -D_FORTIFY_SOURCE=2 \
    -fstack-protector-strong \
    -fPIE \
    -fPIC \
    \
    -fstack-clash-protection \
    -Wall \
    \
    -mindirect-branch-register \
    -mindirect-branch=thunk \
    -mfunction-return=thunk



%C%_libocdcaji_la_SOURCES += \
    %D%/aji.h %D%/c_aji.h \
    %D%/c_jtag_client_gnuaji.h \
    %D%/c_jtag_client_gnuaji.c

if IS_WIN32
#(windows) compile: others
%C%_libocdcaji_la_CPPFLAGS  += \
    -fno-stack-protector

#(windows) Link: others
%C%_libocdcaji_la_LDFLAGS += \
     -Wl,--dynamicbase,--export-dynamic \
     -Wl,--nxcompat

%C%_libocdcaji_la_SOURCES  += %D%/c_jtag_client_gnuaji_win64.h %D%/c_jtag_client_gnuaji_win64.c

else
#(linux) compile: others
%C%_libocdcaji_la_CPPFLAGS  += \
    -fstack-protector-strong

#(linux) Link: others
%C%_libocdcaji_la_LDFLAGS += \
    -z relro \
    -z now \
    -z noexecstack \
    \
    -pie \
    \
    -rdynamic

%C%_libocdcaji_la_SOURCES  += %D%/c_jtag_client_gnuaji_lib64.h %D%/c_jtag_client_gnuaji_lib64.c
endif
