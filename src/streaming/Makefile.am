AM_CPPFLAGS = -I$(top_srcdir)/include
AM_CFLAGS = -I$(top_srcdir)/include -std=c99

EXTRA_DIST = server.h streaming.hpp libwebsocket

noinst_LIBRARIES = libstreaming.a

libstreaming_a_SOURCES = server.c streaming.cpp