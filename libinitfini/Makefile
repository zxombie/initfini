.PATH: ../initfini
SHLIB=initfini
SHLIB_MAJOR=0
CFLAGS+=	-DSHLIB

FILES=initfini
CLEANFILES=initfini

initfini: initfini.sh
	cp ${.CURDIR}/initfini.sh ${.OBJDIR}/initfini

SUBDIR=		test

.include "../initfini/Makefile.srcs"
.include <bsd.lib.mk>
