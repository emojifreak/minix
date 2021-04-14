#	$NetBSD: Makefile,v 1.3 2019/11/11 22:44:57 joerg Exp $

.include <bsd.own.mk>

.if (${MKLLVM} != "no" || ${MKLLVMRT} != "no")
SUBDIR+= llvm
.endif

.include <bsd.subdir.mk>
