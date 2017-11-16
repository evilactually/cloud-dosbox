AM_CPPFLAGS = -I$(top_srcdir)/include -D__STDC_CONSTANT_MACROS
AM_CFLAGS = -I$(top_srcdir)/include -std=c99

EXTRA_DIST = streaming.hpp server.h test.h encoder.h

noinst_LIBRARIES = libstreaming.a

libstreaming_a_SOURCES = streaming.cpp server.cpp test.cpp encoder.cpp

# libstreaming_a_LIBADD = -lwebsockets -lavutil -lavcodec -lswscale