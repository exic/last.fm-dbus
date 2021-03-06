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
/* Generated on Sat Jul  1 22:50:51 EDT 2006 */

#include "config.h"



/* cheap-mode: VECTGRADE_FULL succeeded. (48 steps) */
/* Generated by: ../../../genfft-k7/gen_twiddle -no-randomized-cse -dif -n 4 -sign 1 -name f1k7i_4 */

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
.text
.text
	.balign 64
f1k7i_4:
	subl $20, %esp
	femms 
	movl %ebx, 16(%esp)
	movl 36(%esp), %edx
	movl 44(%esp), %ebx
	movl %esi, 12(%esp)
	movl %edi, 8(%esp)
	movl 28(%esp), %ecx
	movl %ebp, 4(%esp)
	movl 32(%esp), %eax
	movl 40(%esp), %esi
	leal (,%edx,4), %edx
	leal (,%ebx,4), %ebx
	.p2align 4,,7
.L0:
	/* promise simd cell size = 8 */ 
	leal (%edx,%edx,2), %edi
	movq (%ecx,%edx), %mm4
	movq (%ecx,%edx,2), %mm0
	movq (%ecx), %mm1
	leal (%edx,%edx,2), %ebp
	movq (%ecx,%edi), %mm3
	movq %mm4, %mm5
	movq %mm1, %mm2
	pfadd %mm0, %mm1
	pfsub %mm3, %mm4
	pfadd %mm3, %mm5
	pfsub %mm0, %mm2
	movq 8(%eax), %mm0
	movq %mm1, %mm6
	pswapd %mm4, %mm4
	pfsub %mm5, %mm1
	pxor chs_lo, %mm4
	movq %mm2, %mm7
	pfadd %mm5, %mm6
	movq (%eax), %mm5
	pswapd %mm1, %mm3
	pfmul %mm0, %mm1
	pfadd %mm4, %mm2
	pfsub %mm4, %mm7
	pfmul %mm0, %mm3
	movq 16(%eax), %mm0
	/* simd data load/store barrier */ 
	movq %mm6, (%ecx)
	addl $24, %eax
	pswapd %mm2, %mm4
	pfmul %mm5, %mm2
	pswapd %mm7, %mm6
	pfmul %mm0, %mm7
	pfmul %mm5, %mm4
	pfpnacc %mm3, %mm1
	pfmul %mm0, %mm6
	pfpnacc %mm4, %mm2
	movq %mm1, (%ecx,%edx,2)
	pfpnacc %mm6, %mm7
	movq %mm2, (%ecx,%edx)
	movq %mm7, (%ecx,%ebp)
	addl %ebx, %ecx
	decl %esi
	jnz .L0
	femms 
	movl 16(%esp), %ebx
	movl 12(%esp), %esi
	movl 8(%esp), %edi
	movl 4(%esp), %ebp
	addl $20, %esp
	ret 

.section .rodata
nam:
	.string "f1k7i_4"
	.align 4
twinstr:
	.byte 4
	.byte 0
	.value 4
	.byte 3
	.byte 1
	.value 0
	.align 4
desc:
	.long 4
	.long nam
	.long twinstr
	.long fftwf_kdft_ct_k7_pgenus
	.double 11
	.double 6
	.double 0
	.double 0
	.long 0
	.long 0
	.long 0

.text
	.align 4
.globl fftwf_codelet_f1k7i_4
fftwf_codelet_f1k7i_4:
	subl $12,%esp
	movl 16(%esp),%eax
	addl $-4,%esp
	pushl $desc
	pushl $f1k7i_4
	pushl %eax
	call fftwf_kdft_dif_register
	addl $16,%esp
	addl $12,%esp
	ret

