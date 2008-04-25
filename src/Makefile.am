#util_SOURCES = bag bitFippling bt circularBuffer err file geometry hash \
#	idTable libcCompat license mem memory path shlib sock strbuf textRPC \
#	threading threadpool time vector


if HAVE_LIBCRYPTO
LIB_OBJS = bag bitFippling bt byteQueueC circularBuffer driver err event \
           file geometry hash idTable \
           libcCompat logging mem memory path queue shlib sock \
           strbuf tanClient tcpip textRPC threading threadpool time \
           vector wordMap libcCompat license
# Hack for automake bug, can't handle src as makefile macro?
LIB_SRC = bag.c bitFippling.c bt.c byteQueueC.c circularBuffer.c driver.c \
           err.c event.c file.c geometry.c hash.c idTable.c \
           libcCompat.c logging.c mem.c memory.c path.c queue.c shlib.c \
           sock.c \
           strbuf.c tanClient.c tcpip.c textRPC.c threading.c threadpool.c \
           time.c vector.c wordMap.c license.c
else
LIB_OBJS = bag bitFippling bt byteQueueC circularBuffer driver err event \
           file geometry hash idTable \
           libcCompat logging mem memory path queue shlib sock \
           strbuf tanClient tcpip textRPC threading threadpool time \
           vector wordMap libcCompat
# Hack for automake bug, can't handle src as makefile macro?
LIB_SRC = bag.c bitFippling.c bt.c byteQueueC.c circularBuffer.c driver.c \
           err.c event.c file.c geometry.c hash.c idTable.c \
           libcCompat.c logging.c mem.c memory.c path.c queue.c shlib.c \
	   sock.c \
           strbuf.c tanClient.c tcpip.c textRPC.c threading.c threadpool.c \
           time.c vector.c wordMap.c
endif

#LIB_SRC = $(addsuffix .c, $(LIB_OBJS))

if USE_LIBTOOL

#
# --------------------------------------------------------------------
#
# Here we implement the default case: Using libtool on linux/unix-like
# systems
#

AM_CPPFLAGS = -I../include -include ../config.h
lib_LTLIBRARIES = libvoxiUtil.la

#voxiUtilIncludedir=$(includedir)/voxi/util

#voxiUtilInclude_HEADERS = bag.h bitFippling.h bt.h circularBuffer.h collection.h err.h file.h geometry.h hash.h idTable.h libcCompat.h license.h mem.h memory.h path.h shlib.h sock.h strbuf.h textRPC.h threading.h threadpool.h time.h vector.h win32_glue.h

libvoxiUtil_la_SOURCES = $(LIB_SRC)

#
# Got strange compilation erro when including libcCompat.h
#
# libcCompat.c 

libvoxiUtil_la_LDFLAGS = -version-info 0:0:0 -lpthread -lcrypto
#libvoxiUtil_la_LDFLAGS = -version-info 0:0:0 -lpthread

else # USE_LIBTOOL == 0

#
# --------------------------------------------------------------------
#
# Not using libtool is currently assumed to imply that we are building
# on Win32.
#

LIB_OBJS_WIN32 = win32_glue

LIB_OBJFILES = $(addsuffix .$(OBJEXT), $(LIB_OBJS) $(LIB_OBJS_WIN32))
#LIB_DEPFILES = $(addprefix $(OBJDIR)/, $(addsuffix .d, $(LIB_OBJS)))
LOADLIBES = wsock32.lib msvcrt.lib $(LIBS)

all: $(PACKAGE_NAME).dll $(PACKAGE_NAME).lib
exports: $(PACKAGE_NAME).lib

# Needed since GNU make doesn't have default rules for .obj files (only .o)
%.obj: %.c
	$(CC) -nologo -c -I$(srcdir) -I$(top_builddir) -I$(top_builddir)/include $(CPPFLAGS) $(CFLAGS) /Fo$@ $<

# Make a shared library
$(PACKAGE_NAME).dll: $(PACKAGE_NAME).def $(LIB_OBJFILES)
	link -dll -def:$< $(LDFLAGS) -out:$@ $(LOADLIBES) $(LIB_OBJFILES)

# Make an import library to be used in dynamic compile-time linking.
$(PACKAGE_NAME).lib: $(PACKAGE_NAME).def
	lib -def:$^ -out:$@

#
# XXXX THIS WILL BE REPLACED BY HARD-CODED DEF FILES SHORTLY!
#
# Generate the export definitions (.def) file used when creating a DLL.
# Note that we have to explicitly remove exporting of MSVC-introduced
# float and double symbols, beginning with _real@4@ and _real@8@ etc.
# We also remove any comment lines, to avoid insignificant CVS diffs.
$(PACKAGE_NAME).def: $(LIB_OBJFILES)
	dlltool --export-all-symbols --output-def temp.def $^
	grep -v '_real@[0-9]' temp.def > temp2.def
	grep -v '^;' temp2.def > temp3.def
	grep -v '??_' temp3.def > $@
	rm -f temp.def temp2.def temp3.def

endif # USE_LIBTOOL

#
# --------------------------------------------------------------------
#

