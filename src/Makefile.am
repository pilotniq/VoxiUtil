#util_SOURCES = bag bitFippling bt circularBuffer err file geometry hash \
#	idTable libcCompat license mem memory path shlib sock strbuf textRPC \
#	threading threadpool time vector

AM_CPPFLAGS = -I../include
lib_LTLIBRARIES = libvoxiUtil.la

#voxiUtilIncludedir=$(includedir)/voxi/util

#voxiUtilInclude_HEADERS = bag.h bitFippling.h bt.h circularBuffer.h collection.h err.h file.h geometry.h hash.h idTable.h libcCompat.h license.h mem.h memory.h path.h shlib.h sock.h strbuf.h textRPC.h threading.h threadpool.h time.h vector.h win32_glue.h

libvoxiUtil_la_SOURCES = bag.c bitFippling.c bt.c circularBuffer.c err.c \
                         file.c geometry.c hash.c idTable.c \
                         license.c mem.c memory.c path.c shlib.c sock.c \
                         strbuf.c textRPC.c threading.c threadpool.c time.c \
                         vector.c wordMap.c
#
# Got strange compilation erro when including libcCompat.h
#
# libcCompat.c 

libvoxiUtil_la_LDFLAGS = -version-info 0:0:0 -lpthread -lcrypt

