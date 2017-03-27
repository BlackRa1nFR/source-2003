//
// snd_mixa.s
// x86 assembly-language sound code
//

#include "snd_asm_i386.h"
#include "../../quakeasm.h"


#if	id386

	.text

//----------------------------------------------------------------------
// 8-bit sound-mixing code
//----------------------------------------------------------------------

#define volume	4+16
#define pdata	8+16
#define count	12+16

.globl C(SND_PaintChannelFrom8)
C(SND_PaintChannelFrom8):
	pushl	%esi				// preserve register variables
	pushl	%edi
	pushl	%ebx
	pushl	%ebp

//	int 	data;
//	short	*lscale, *rscale;
//	unsigned char *sfx;
//	int		i;

	movl	volume(%esp),%ebx
	movl	pdata(%esp),%esi

	movl	0(%ebx),%eax
	movl	4(%ebx),%edx

//	lscale = snd_scaletable[volume[0] >> SND_SCALE_SHIFT];
//	rscale = snd_scaletable[volume[1] >> SND_SCALE_SHIFT];
	
	// NOTE: These values depend on the setting of SND_SCALE_BITS
	// The mask must mask off all the lower bits you aren't using in the multiply
	// so for 7 bits, the mask is 0xFE, 6 bits 0xFC, etc.
	// The shift must multiply by the table size.  There are 256 4-byte values in the table at each level.
	// So each index must be shifted left by 10, but since the bits we use are in the MSB rather than LSB
	// they must be shifted right by 8 - SND_SCALE_BITS.  e.g., for a 7 bit number the left shift is:
	// 10 - (8-7) = 9.  For a 5 bit number it's 10 - (8-5) = 7.

	// eax is lscale, edx is rscale
	andl	$0xFE,%eax
	andl	$0xFE,%edx
	movl	count(%esp),%ecx

	// shift up by 10 to index table, down by 1 to make the 7 MSB of the bytes an index
	shll	$9,%eax
	shll	$9,%edx
	addl	$(C(snd_scaletable)),%eax
	addl	$(C(snd_scaletable)),%edx
	subl	%ebx,%ebx
	movb	-1(%esi,%ecx,1),%bl

	testl	$1,%ecx
	jz		LMix8Loop

	movl	(%eax,%ebx,4),%edi
	movl	(%edx,%ebx,4),%ebp
	addl	C(paintbuffer)+psp_left-psp_size(,%ecx,psp_size),%edi
	addl	C(paintbuffer)+psp_right-psp_size(,%ecx,psp_size),%ebp
	movl	%edi,C(paintbuffer)+psp_left-psp_size(,%ecx,psp_size)
	movl	%ebp,C(paintbuffer)+psp_right-psp_size(,%ecx,psp_size)
	movb	-2(%esi,%ecx,1),%bl

	decl	%ecx
	jz		LDone

//	for (i=0 ; i<count ; i++)
//	{
LMix8Loop:

//		data = sfx[i];
//		paintbuffer[i].left += lscale[data];
//		paintbuffer[i].right += rscale[data];
	movl	(%eax,%ebx,4),%edi
	movl	(%edx,%ebx,4),%ebp
	addl	C(paintbuffer)+psp_left-psp_size(,%ecx,psp_size),%edi
	addl	C(paintbuffer)+psp_right-psp_size(,%ecx,psp_size),%ebp
	movb	-2(%esi,%ecx,1),%bl
	movl	%edi,C(paintbuffer)+psp_left-psp_size(,%ecx,psp_size)
	movl	%ebp,C(paintbuffer)+psp_right-psp_size(,%ecx,psp_size)

	movl	(%eax,%ebx,4),%edi
	movl	(%edx,%ebx,4),%ebp
	movb	-3(%esi,%ecx,1),%bl
	addl	C(paintbuffer)+psp_left-psp_size*2(,%ecx,psp_size),%edi
	addl	C(paintbuffer)+psp_right-psp_size*2(,%ecx,psp_size),%ebp
	movl	%edi,C(paintbuffer)+psp_left-psp_size*2(,%ecx,psp_size)
	movl	%ebp,C(paintbuffer)+psp_right-psp_size*2(,%ecx,psp_size)

//	}
	subl	$2,%ecx
	jnz		LMix8Loop

LDone:
	popl	%ebp
	popl	%ebx
	popl	%edi
	popl	%esi

	ret


//----------------------------------------------------------------------
// Transfer of stereo buffer to 16-bit DMA buffer code
//----------------------------------------------------------------------

.globl C(Snd_WriteLinearBlastStereo16)
C(Snd_WriteLinearBlastStereo16):
	pushl	%esi				// preserve register variables
	pushl	%edi
	pushl	%ebx

//	int		i;
//	int		val;
	movl	C(snd_linear_count),%ecx
	movl	C(snd_p),%ebx
	movl	C(snd_vol),%esi
	movl	C(snd_out),%edi

//	for (i=0 ; i<snd_linear_count ; i+=2)
//	{
LWLBLoopTop:

//		val = (snd_p[i]*snd_vol)>>8;
//		if (val > 0x7fff)
//			snd_out[i] = 0x7fff;
//		else if (val < (short)0x8000)
//			snd_out[i] = (short)0x8000;
//		else
//			snd_out[i] = val;
	movl	-8(%ebx,%ecx,4),%eax
	imull	%esi,%eax
	sarl	$8,%eax
	cmpl	$0x7FFF,%eax
	jg		LClampHigh
	cmpl	$0xFFFF8000,%eax
	jnl		LClampDone
	movl	$0xFFFF8000,%eax
	jmp		LClampDone
LClampHigh:
	movl	$0x7FFF,%eax
LClampDone:

//		val = (snd_p[i+1]*snd_vol)>>8;
//		if (val > 0x7fff)
//			snd_out[i+1] = 0x7fff;
//		else if (val < (short)0x8000)
//			snd_out[i+1] = (short)0x8000;
//		else
//			snd_out[i+1] = val;
	movl	-4(%ebx,%ecx,4),%edx
	imull	%esi,%edx
	sarl	$8,%edx
	cmpl	$0x7FFF,%edx
	jg		LClampHigh2
	cmpl	$0xFFFF8000,%edx
	jnl		LClampDone2
	movl	$0xFFFF8000,%edx
	jmp		LClampDone2
LClampHigh2:
	movl	$0x7FFF,%edx
LClampDone2:
	shll	$16,%edx
	andl	$0xFFFF,%eax
	orl		%eax,%edx
	movl	%edx,-4(%edi,%ecx,2)

//	}
	subl	$2,%ecx
	jnz		LWLBLoopTop

//	snd_p += snd_linear_count;

	popl	%ebx
	popl	%edi
	popl	%esi

	ret




#endif	// id386

