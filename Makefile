#	$OpenBSD: Makefile,v 1.47 2018/07/03 01:34:43 mortimer Exp $

PROG=	filter-eventlog

#BINMODE?=0550

BINDIR=	/usr/libexec
NOMAN=

CFLAGS+=	-fstack-protector-all
CFLAGS+=	-I${.CURDIR}/..
CFLAGS+=	-Wall -Wstrict-prototypes -Wmissing-prototypes
CFLAGS+=	-Wmissing-declarations
CFLAGS+=	-Wshadow -Wpointer-arith -Wcast-qual
CFLAGS+=	-Wsign-compare
CFLAGS+=	-Werror-implicit-function-declaration
YFLAGS=

SRCS=	eventlog.c

.include <bsd.prog.mk>
