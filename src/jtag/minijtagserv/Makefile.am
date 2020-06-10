MINIJTAGSERV_BUILTINS_CPPFLAGS = -DWINDOWS=1 -DUNIX=2 -DPORT=UNIX -DLINUX=3 -DSYS=LINUX
MINIJTAGSERV_BUILTINS_CPPFLAGS += -DLITTLE=1 -DBIG=2 -DENDIAN=LITTLE

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
    -Wformat -Wformat-security -fPIE -fPIC -D_FORTIFY_SOURCE=2 -Werror=delete-incomplete


noinst_LTLIBRARIES += %D%/libocdminijtagserv.la

%C%_libocdminijtagserv_la_SOURCES = $(MINIJTAGSERV_SRC)

%C%_libocdminijtagserv_la_CPPFLAGS = -I%D%/h -I%D%/.. \
    $(AM_CPPFLAGS) \
    $(MINIJTAGSERV_BUILTINS_CPPFLAGS) $(JTAG_SERV_GENERAL_CPPFLAGS) $(MINIJTAGSERV_ALTERA_CPPFLAGS) \
    $(MINIJTAGSERV_MISC_FLAGS) \
    $(LIBUSB1_CFLAGS) $(LIBFTDI_CFLAGS)
    
%C%_libocdminijtagserv_la_LDFLAGS = -Wl,-fuse-ld=gold -z noexecstack -z relro -rdynamic \
    -fopenmp \
    -L%D%/lib64  \
    -static-libgcc 
%C%_libocdminijtagserv_la_LDLIBS = -ljtag_client -lccl_ver -lSafeString

MINIJTAGSERV_SRC = \
    %D%/h/aji.h %D%/h/c_aji.h  %D%/c2_aji.h \
    %D%/h/jtag_client_gnuaji.h %D%/h/c_jtag_client_gnuaji.h  \
<<<<<<< HEAD
=======
    %D%/jtagservice.h %D%/jtagservice.c \
>>>>>>> minijtagserv: Can compile and run
    %D%/minijtagserv.c



