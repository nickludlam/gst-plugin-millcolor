plugin_LTLIBRARIES = libgstmillcolor.la

libgstmillcolor_la_SOURCES = gstalphacolor.c
libgstmillcolor_la_CFLAGS = $(GST_PLUGINS_BASE_CFLAGS) \
	$(GST_BASE_CFLAGS) $(GST_CFLAGS)
libgstmillcolor_la_LIBADD = $(GST_LIBS) $(GST_BASE_LIBS)
libgstmillcolor_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)
libgstmillcolor_la_LIBTOOLFLAGS = --tag=disable-static

noinst_HEADERS = gstmillcolor.h
