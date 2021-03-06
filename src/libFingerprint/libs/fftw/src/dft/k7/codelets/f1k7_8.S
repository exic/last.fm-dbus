/*
 * Copyright (c) 2003, 2006 Matteo Frigo
 * Copyright (c) 2003, 2006 Massachusetts Institute of Technology
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/* This file was automatically generated --- DO NOT EDIT */
/* Generated on Sat Jul  1 22:50:01 EDT 2006 */

#include "config.h"



/* cheap-mode: VECTGRADE_FULL succeeded. (136 steps) */
/* Generated by: ../../../genfft-k7/gen_twiddle -no-randomized-cse -dif -n 8 -name f1k7_8 */

/*
 * Generator Id's : 
 * $Id: algsimp.ml,v 1.4 2006-01-05 03:04:27 stevenj Exp $
 * $Id: fft.ml,v 1.4 2006-01-05 03:04:27 stevenj Exp $
 * $Id: gen_twiddle.ml,v 1.14 2005-12-24 21:08:49 athena Exp $
 */

/* The following asm code is Copyright (c) 2000-2001 Stefan Kral */
.section .rodata
	.balign 64
chs_lo: .long 0x80000000, 0x00000000
KP707106781KP707106781: .float +0.707106781186547524400844362104849039284835938, +0.707106781186547524400844362104849039284835938
chs_hi: .long 0x00000000, 0x80000000
.text
.text
	.balign 64
f1k7_8:
	subl $60, %esp
	femms 
	movl %ebx, 56(%esp)
	movl 76(%esp), %edx
	movl 84(%esp), %ebx
	movl 64(%esp), %ecx
	movl %esi, 52(%esp)
	movl 72(%esp), %eax
	movl %edi, 48(%esp)
	movl 80(%esp), %esi
	movl %ebp, 44(%esp)
	leal (,%edx,4), %edx
	leal (,%ebx,4), %ebx
	.p2align 4,,7
.L0:
	/* promise simd cell size = 8 */ 
	movq (%ecx,%edx,4), %mm0
	movq (%ecx), %mm1
	leal (%edx,%edx,2), %edi
	movq (%ecx,%edx,2), %mm4
	movq (%ecx,%edi,2), %mm3
	leal (%edi,%edx,4), %ebp
	movq (%ecx,%edi), %mm6
	movq %mm1, %mm2
	pfadd %mm0, %mm1
	leal (%edx,%edx,4), %edi
	movq %mm4, %mm5
	movq (%ecx,%ebp), %mm7
	leal (%edx,%edx,2), %ebp
	pfsub %mm0, %mm2
	pfadd %mm3, %mm4
	pfsub %mm3, %mm5
	movq %mm1, %mm3
	movq %mm7, %mm0
	pfadd %mm6, %mm7
	movq %mm2, 8(%esp)
	movq (%ecx,%edi), %mm2
	leal (%ebp,%edx,4), %edi
	pfsub %mm4, %mm1
	pfadd %mm4, %mm3
	movq (%ecx,%edx), %mm4
	pswapd %mm5, %mm5
	pfsub %mm6, %mm0
	pxor chs_lo, %mm5
	movq %mm1, 0(%esp)
	movq %mm4, %mm6
	pfsub %mm2, %mm4
	pfadd %mm2, %mm6
	movq %mm0, %mm2
	pfpnacc %mm4, %mm2
	pswapd %mm4, %mm4
	pfpnacc %mm0, %mm4
	movq %mm6, %mm1
	pfadd %mm7, %mm6
	pfsub %mm7, %mm1
	movq %mm2, %mm0
	movq %mm3, %mm7
	pfacc %mm4, %mm2
	pfadd %mm6, %mm3
	pfsub %mm6, %mm7
	movq 8(%esp), %mm6
	pswapd %mm1, %mm1
	pfnacc %mm0, %mm4
	pfmul KP707106781KP707106781, %mm2
	pxor chs_hi, %mm1
	movq %mm3, 24(%esp)
	movq 0(%esp), %mm3
	movq %mm6, %mm0
	pfsub %mm5, %mm6
	movq %mm7, 16(%esp)
	pfmul KP707106781KP707106781, %mm4
	pfadd %mm5, %mm0
	movq %mm3, %mm7
	pfsub %mm1, %mm3
	movq %mm6, %mm5
	pfsub %mm2, %mm6
	pfadd %mm1, %mm7
	movq 32(%eax), %mm1
	pfadd %mm2, %mm5
	pswapd %mm6, %mm2
	pfmul %mm1, %mm6
	pfmul %mm1, %mm2
	movq %mm0, %mm1
	pfsub %mm4, %mm0
	pfadd %mm4, %mm1
	movq 48(%eax), %mm4
	pfpnacc %mm6, %mm2
	pswapd %mm0, %mm6
	pfmul %mm4, %mm0
	pfmul %mm4, %mm6
	pswapd %mm1, %mm4
	movq %mm2, 32(%esp)
	movq 16(%eax), %mm2
	pfpnacc %mm0, %mm6
	movq (%eax), %mm0
	pfmul %mm2, %mm4
	pfmul %mm2, %mm1
	pswapd %mm5, %mm2
	pfmul %mm0, %mm2
	pswapd %mm6, %mm6
	pfmul %mm0, %mm5
	pswapd %mm7, %mm0
	pfpnacc %mm1, %mm4
	movq 8(%eax), %mm1
	pfpnacc %mm5, %mm2
	movq 40(%eax), %mm5
	pfmul %mm1, %mm0
	pswapd %mm4, %mm4
	pfmul %mm1, %mm7
	pswapd %mm3, %mm1
	pfmul %mm5, %mm1
	pswapd %mm2, %mm2
	pfmul %mm5, %mm3
	movq 24(%eax), %mm5
	/* simd data load/store barrier */ 
	movq %mm4, (%ecx,%ebp)
	movq 16(%esp), %mm4
	pfpnacc %mm7, %mm0
	movq 24(%esp), %mm7
	movq %mm6, (%ecx,%edi)
	addl $56, %eax
	movq %mm2, (%ecx,%edx)
	pfpnacc %mm3, %mm1
	movq 32(%esp), %mm3
	pswapd %mm4, %mm6
	pfmul %mm5, %mm4
	pswapd %mm0, %mm0
	movq %mm7, (%ecx)
	pfmul %mm5, %mm6
	pswapd %mm1, %mm1
	pswapd %mm3, %mm3
	movq %mm0, (%ecx,%edx,2)
	movq %mm1, (%ecx,%ebp,2)
	leal (%edx,%edx,4), %ebp
	pfpnacc %mm4, %mm6
	movq %mm3, (%ecx,%ebp)
	pswapd %mm6, %mm6
	movq %mm6, (%ecx,%edx,4)
	addl %ebx, %ecx
	decl %esi
	jnz .L0
	femms 
	movl 56(%esp), %ebx
	movl 52(%esp), %esi
	movl 48(%esp), %edi
	movl 44(%esp), %ebp
	addl $60, %esp
	ret 

.section .rodata
nam:
	.string "f1k7_8"
	.align 4
twinstr:
	.byte 4
	.byte 0
	.value 8
	.byte 3
	.byte 1
	.value 0
	.align 4
desc:
	.long 8
	.long nam
	.long twinstr
	.long fftwf_kdft_ct_k7_mgenus
	.double 33
	.double 16
	.double 0
	.double 0
	.long 0
	.long 0
	.long 0

.text
	.align 4
.globl fftwf_codelet_f1k7_8
fftwf_codelet_f1k7_8:
	subl $12,%esp
	movl 16(%esp),%eax
	addl $-4,%esp
	pushl $desc
	pushl $f1k7_8
	pushl %eax
	call fftwf_kdft_dif_register
	addl $16,%esp
	addl $12,%esp
	ret

