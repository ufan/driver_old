/* $ZEL: plxbaseio.h,v 1.2 2001/09/26 21:34:33 wuestner Exp $ */

/*
 * Copyright (c) 2001
 * 	Matthias Drochner.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "linux/ioctl.h"

struct plxreg {
        int offset, val;
};

#define PLX_MAGIC 'x'

#define PLXREADCFG     _IOWR(PLX_MAGIC, 1, struct plxreg)
#define PLXWRITECFG    _IOW(PLX_MAGIC, 2, struct plxreg)
#define PLXREADREG     _IOWR(PLX_MAGIC, 3, struct plxreg)
#define PLXWRITEREG    _IOW(PLX_MAGIC, 4, struct plxreg)
#define PLXREADLOCAL0  _IOWR(PLX_MAGIC, 5, struct plxreg)
#define PLXWRITELOCAL0 _IOW(PLX_MAGIC, 6, struct plxreg)
#define PLXREADLOCAL1  _IOWR(PLX_MAGIC, 7, struct plxreg)
#define PLXWRITELOCAL1 _IOW(PLX_MAGIC, 8, struct plxreg)

