AM_CPPFLAGS = -I$(top_srcdir)/kernel
SUBDIRS = codelets simd k7

noinst_LTLIBRARIES = libdft.la

libdft_la_SOURCES = bluestein.c buffered.c conf.c ct.c ctsq.c		\
dftw-direct.c dftw-generic.c dftw-genericbuf.c direct.c generic.c	\
indirect.c indirect-transpose.c kdft-dif.c kdft-difsq.c kdft-dit.c	\
kdft.c nop.c plan.c problem.c rader.c rank-geq2.c solve.c vrank-geq1.c	\
zero.c codelet-dft.h ct.h dft.h
