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

# Compile: For configuring AJI Client Driver
%C%_libocdaji_client_la_CPPFLAGS += \
    -DMODE_64_BIT

# Compile: For optimizing AJI Client Driver
%C%_libocdaji_client_la_CPPFLAGS += \
    -O1 \
    -D_GNU_SOURCE -D_REENTRANT
    
# Compile: Preferred flags used for optimizing AJI Client driver
%C%_libocdaji_client_la_CPPFLAGS += \
    -Wall \
    -Wno-error=coverage-mismatch -fPIC -ggdb1 -mmmx -msse \
    -fno-strict-aliasing -finline -fno-inline-functions -fno-builtin -momit-leaf-frame-pointer -fomit-frame-pointer \
    -funwind-tables -Wno-system-headers -Wno-extra -Wno-switch -Wno-unused-function -Wno-unknown-pragmas \
    -Wno-comment -Wno-parentheses -Wno-strict-overflow -Werror -Wno-error=deprecated-declarations -fopenmp \
    -Wno-misleading-indentation \
    -Wno-deprecated -Wno-attributes -Wno-unused-but-set-variable -Wno-nonnull-compare -Wno-unused-const-variable \
    -flifetime-dse=1 -frecord-gcc-switches \
    -Werror=delete-incomplete

##Compile: Intel security policy - strongly recommended
%C%_libocdaji_client_la_CPPFLAGS  += \
    -Wformat -Wformat-security -Werror=format-security \
    -D_FORTIFY_SOURCE=2 \
    -fstack-protector-strong \
    -fPIE \
    -fPIC 

#Compile: Intel security policy - recommended
%C%_libocdaji_client_la_CPPFLAGS  += \
    -fstack-clash-protection \
    -Wall 

#Compile: Intel security policy - Spectre2 - recommended
%C%_libocdaji_client_la_CPPFLAGS  += \
    -mindirect-branch-register \
    -mindirect-branch=thunk \
    -mfunction-return=thunk

#if !IS_WIN32
#%C%_libocdaji_client_la_CPPFLAGS += -Wformat
#endif



include %D%/aji/Makefile.am
%C%_libocdaji_client_la_LIBADD += $(top_builddir)/%D%/aji/libocdcaji.la

%C%_libocdaji_client_la_CPPFLAGS += -I%D%/h -I%D%/.. \
    $(AM_CPPFLAGS)  

%C%_libocdaji_client_la_LDFLAGS += -Wl,-fuse-ld=gold -z noexecstack -z relro -rdynamic \
    -fopenmp \
    -static-libgcc 

%C%_libocdaji_client_la_SOURCES += \
    %D%/aji/aji.h %D%/aji/c_aji.h \
    %D%/aji/c_jtag_client_gnuaji.h  \
    %D%/jtagservice.h %D%/jtagservice.c \
    %D%/aji_client.c 

#if IS_WIN32
##(windows) Link:  Intel security policy - strongly recommended
%C%_libocdaji_client_la_LDFLAGS += \
     -Wl,--dynamicbase,--export-dynamic \
     -Wl,--nxcompat

%C%_libocdaji_client_la_SOURCES += %D%/aji/jtag_client_gnuaji_win64.h %D%/aji/c_jtag_client_gnuaji_win64.h

#else

##(linux) Link:  Intel security policy - strongly recommended
%C%_libocdaji_client_la_LDFLAGS += \
    -z relro \
    -z now \
    -z noexecstack \
    \
    -pie \
    \
    -rdynamic

%C%_libocdaji_client_la_SOURCES += %D%/aji/jtag_client_gnuaji_lib64.h %D%/aji/c_jtag_client_gnuaji_lib64.h
#endif

#@TODO During installation, copy all the dependent libraries
#      to its final location
