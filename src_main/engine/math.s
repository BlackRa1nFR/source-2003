//
// math.s
// x86 assembly-language math routines.

#define GLQUAKE	1	// don't include unneeded defs
#include "asm_i386.h"
#include "quakeasm.h"


#if	id386

	.data

	.text

// TODO: rounding needed?
// stack parameter offset
#define	val	4

.globl C(Invert24To16)
C(Invert24To16):

	movl	val(%esp),%ecx
	movl	$0x100,%edx		// 0x10000000000 as dividend
	cmpl	%edx,%ecx
	jle		LOutOfRange

	subl	%eax,%eax
	divl	%ecx

	ret

LOutOfRange:
	movl	$0xFFFFFFFF,%eax
	ret

#define	in	4
#define out	8

	.align 2
.globl C(TransformVector)
C(TransformVector):
	movl	in(%esp),%eax
	movl	out(%esp),%edx

	flds	(%eax)		// in[0]
	fmuls	C(asmvright)		// in[0]*vright[0]
	flds	(%eax)		// in[0] | in[0]*vright[0]
	fmuls	C(asmvup)		// in[0]*vup[0] | in[0]*vright[0]
	flds	(%eax)		// in[0] | in[0]*vup[0] | in[0]*vright[0]
	fmuls	C(asmvpn)		// in[0]*vpn[0] | in[0]*vup[0] | in[0]*vright[0]

	flds	4(%eax)		// in[1] | ...
	fmuls	C(asmvright)+4	// in[1]*vright[1] | ...
	flds	4(%eax)		// in[1] | in[1]*vright[1] | ...
	fmuls	C(asmvup)+4		// in[1]*vup[1] | in[1]*vright[1] | ...
	flds	4(%eax)		// in[1] | in[1]*vup[1] | in[1]*vright[1] | ...
	fmuls	C(asmvpn)+4		// in[1]*vpn[1] | in[1]*vup[1] | in[1]*vright[1] | ...
	fxch	%st(2)		// in[1]*vright[1] | in[1]*vup[1] | in[1]*vpn[1] | ...

	faddp	%st(0),%st(5)	// in[1]*vup[1] | in[1]*vpn[1] | ...
	faddp	%st(0),%st(3)	// in[1]*vpn[1] | ...
	faddp	%st(0),%st(1)	// vpn_accum | vup_accum | vright_accum

	flds	8(%eax)		// in[2] | ...
	fmuls	C(asmvright)+8	// in[2]*vright[2] | ...
	flds	8(%eax)		// in[2] | in[2]*vright[2] | ...
	fmuls	C(asmvup)+8		// in[2]*vup[2] | in[2]*vright[2] | ...
	flds	8(%eax)		// in[2] | in[2]*vup[2] | in[2]*vright[2] | ...
	fmuls	C(asmvpn)+8		// in[2]*vpn[2] | in[2]*vup[2] | in[2]*vright[2] | ...
	fxch	%st(2)		// in[2]*vright[2] | in[2]*vup[2] | in[2]*vpn[2] | ...

	faddp	%st(0),%st(5)	// in[2]*vup[2] | in[2]*vpn[2] | ...
	faddp	%st(0),%st(3)	// in[2]*vpn[2] | ...
	faddp	%st(0),%st(1)	// vpn_accum | vup_accum | vright_accum

	fstps	8(%edx)		// out[2]
	fstps	4(%edx)		// out[1]
	fstps	(%edx)		// out[0]

	ret



#endif	// id386
