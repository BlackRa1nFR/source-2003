// snd_dsp.c -- audio processing routines

#define WIN32_LEAN_AND_MEAN
#pragma warning(push, 1)
#include <windows.h>
#include <mmsystem.h>
#pragma warning(pop)

#include "tier0/dbg.h"
#include "sound.h"
#include "sound_private.h"
#include "soundflags.h"
#include "snd_device.h"
#include "measure_section.h"
#include "iprediction.h"
#include "snd_mix_buf.h"
#include "snd_env_fx.h"
#include "snd_channels.h"
#include "snd_audio_source.h"
#include "snd_convars.h"
#include "SoundService.h"
#include "commonmacros.h"

#include "mathlib.h"


// hard clip input value to -32767 <= y <= 32767

// #define CLIP(x) ((x) > 32767 ? 32767 : ((x) < -32767 ? -32767 : (x)))

#define SIGN(d)				((d)<0?-1:1)

#define ABS(a)	abs(a)

#define MSEC_TO_SAMPS(a)	(((a)*SOUND_DMA_SPEED) / 1000)		// convert milliseconds to # samples in equivalent time
#define SEC_TO_SAMPS(a)		((a)*SOUND_DMA_SPEED)				// conver seconds to # samples in equivalent time

//#define CLIP_DSP(x) ((x) > 32767 ? 32767 : ((x) < -32767 ? -32767 : (x)))

#define CLIP_DSP(x) (x)

extern bool SURROUND_ON;

#define SOUND_MS_PER_FT	1			// sound travels approx 1 foot per millisecond
#define ROOM_MAX_SIZE	1000		// max size in feet of room simulation for dsp

//===============================================================================
//
// Digital Signal Processing algorithms for audio FX.
//
// KellyB 2/18/03
//===============================================================================

// Performance notes:

// DSP processing should take no more than 3ms total time per frame to remain on par with hl1
// Assume a min frame rate of 24fps = 42ms per frame
// at 24fps, to maintain 44.1khz output rate, we must process about 1840 mono samples per frame.
// So we must process 1840 samples in 3ms.

// on a 1Ghz CPU (mid-low end CPU) 3ms provides roughly 3,000,000 cycles.
// Thus we have 3e6 / 1840 = 1630 cycles per sample.  

#define PBITS			12					// parameter bits
#define PMAX			((1 << PBITS)-1)	// parameter max size

// crossfade from y2 to y1 at point r (0 < r < PMAX)

#define XFADE(y1,y2,r)	(((y1) * (r)) >> PBITS) + (((y2) * (PMAX - (r))) >> PBITS);

#define XFADEF(y1,y2,r)	(((y1) * (r)) / (float)(PMAX)) + (((y2) * (PMAX - (r))) / (float)(PMAX));

/////////////////////
// dsp helpers
/////////////////////

// dot two integer vectors of length M+1
// M is filter order, h is filter vector, w is filter state vector

inline int dot ( int M, int *h, int *w )
{
	int i;
	int y;

	for (y = 0, i = 0; i <= M; i++)
		y += ( h[i] * w[i] ) >> PBITS;
	return y;
}

// delay array w[] by D samples
// w[0] = input, w[D] = output
// practical for filters, but not for large values of D

inline void delay ( int D, int *w )
{
	int i;
	for (i = D; i >= 1; i--)		// reverse order updating
		w[i] = w[i-1];
}

// circular wrap of pointer p, relative to array w
// D delay line size in samples w[0...D]
// w delay line buffer pointer, dimension D+1
// p circular pointer

inline void wrap (int D, int *w, int **p)
{
	if ( *p > w + D )
		*p -= D + 1;		// when *p = w + D + 1, it wraps around to *p = w
	
	if ( *p < w )
		*p += D + 1;		// when *p = w - 1, it wraps around to *p = w + D
}

// simple averaging filter for performance - a[] is 0, b[] is 1, L is # of samples to average

inline int avg_filter ( int  M, int *a, int L, int *b, int *w, int x )
{
	int i;
	int y = 0;
	
	
	w[0] = x;

	// output adder

	switch (L)
	{
	default:
	case 12: y += w[12];
	case 11: y += w[11];
	case 10: y += w[10];
	case 9:  y += w[9];
	case 8:  y += w[8];
	case 7:  y += w[7];
	case 6:  y += w[6];
	case 5:  y += w[5];
	case 4:  y += w[4];
	case 3:  y += w[3];
	case 2:  y += w[2];
	case 1:  y += w[1];
	case 0:  y += w[0];
	}
	
	for (i = L; i >= 1; i--)		// reverse update internal state
		w[i] = w[i-1];

	switch (L)
	{
	default:
	case 12: return y / 13;
	case 11: return y / 12;
	case 10: return y / 11;
	case 9:  return y / 10;
	case 8:  return y / 9;
	case 7:  return y >> 3;
	case 6:  return y / 7;
	case 5:  return y / 6;
	case 4:  return y / 5;
	case 3:  return y >> 2;
	case 2:  return y / 3;
	case 1:  return y >> 1;
	case 0:  return y;
	}

	return y;						// current output sample
}

// IIR filter, cannonical form
//  returns single sample y for current input value x
//  x is input sample
//	w = internal state vector, dimension max(M,L) + 1
//  L, M numerator and denominator filter orders
//  a,b are M+1 dimensional arrays of filter params
// 
//  for M = 4:
//
//                1     w0(n)   b0	 
//  x(n)--->(+)--(*)-----.------(*)->(+)---> y(n)
//           ^         |            ^
//           |     [Delay d]        |
//           |         |            |
//           |    -a1  |W1     b1   |
//			 ----(*)---.------(*)----
//           ^         |            ^
//           |     [Delay d]        |
//           |         |            |
//           |    -a2  |W2     b2   |
//			 ----(*)---.------(*)----
//           ^         |            ^
//           |     [Delay d]        |
//           |         |            |
//           |    -a3  |W3     b3   |
//			 ----(*)---.------(*)----
//           ^         |            ^
//           |     [Delay d]        |
//           |         |            |
//           |    -a4  |W4     b4   |
//			 ----(*)---.------(*)----
//
//	for each input sample x, do:
//			w0 = x - a1*w1 - a2*w2 - ... aMwM
//			y = b0*w0 + b1*w1 + ...bL*wL
//			wi = wi-1, i = K, K-1, ..., 1


inline int iir_filter ( int  M, int *a, int L, int *b, int *w, int x )
{
	int K, i;
	int y;
	int x0;					
	
	if (M == 0)
		return (avg_filter( M, a, L, b, w, x));
	
	y = 0;
	x0 = x;

	K = max ( M, L );				
	
	// for (i = 1; i <= M; i++)		// input adder
	//	w[0] -= ( a[i] * w[i] ) >> PBITS;

	// M is clamped between 1 and FLT_M
	// change this switch statement if FLT_M changes!

	switch (M)
	{
	case 12: x0 -= ( a[12] * w[12] ) >> PBITS;
	case 11: x0 -= ( a[11] * w[11] ) >> PBITS;
	case 10: x0 -= ( a[10] * w[10] ) >> PBITS;
	case 9:  x0 -= ( a[9] * w[9] ) >> PBITS;
	case 8:  x0 -= ( a[8] * w[8] ) >> PBITS;
	case 7:  x0 -= ( a[7] * w[7] ) >> PBITS;
	case 6:  x0 -= ( a[6] * w[6] ) >> PBITS;
	case 5:  x0 -= ( a[5] * w[5] ) >> PBITS;
	case 4:  x0 -= ( a[4] * w[4] ) >> PBITS;
	case 3:  x0 -= ( a[3] * w[3] ) >> PBITS;
	case 2:  x0 -= ( a[2] * w[2] ) >> PBITS;
	default:
	case 1:  x0 -= ( a[1] * w[1] ) >> PBITS;
	}

	w[0] = x0;

	//for (i = 0; i <= L; i++)		// output adder
	//	y += ( b[i] * w[i] ) >> PBITS;

	switch (L)
	{
	case 12: y += ( b[12] * w[12] ) >> PBITS;
	case 11: y += ( b[11] * w[11] ) >> PBITS;
	case 10: y += ( b[10] * w[10] ) >> PBITS;
	case 9:  y += ( b[9] * w[9] ) >> PBITS;
	case 8:  y += ( b[8] * w[8] ) >> PBITS;
	case 7:  y += ( b[7] * w[7] ) >> PBITS;
	case 6:  y += ( b[6] * w[6] ) >> PBITS;
	case 5:  y += ( b[5] * w[5] ) >> PBITS;
	case 4:  y += ( b[4] * w[4] ) >> PBITS;
	case 3:  y += ( b[3] * w[3] ) >> PBITS;
	case 2:  y += ( b[2] * w[2] ) >> PBITS;
	default:
	case 1:  y += ( b[1] * w[1] ) >> PBITS;
	case 0:  y += ( b[0] * w[0] ) >> PBITS;
	}
	
	for (i = K; i >= 1; i--)		// reverse update internal state
		w[i] = w[i-1];

	return y;						// current output sample
}

// IIR filter, cannonical form, using dot product and delay implementation
// (may be easier to optimize this routine.)

inline int iir_filter2 ( int  M, int *a, int L, int *b, int *w, int x )
{
	int K;
	int y;

	K = max ( M, L );				// K = max (M, L)
	w[0] = 0;						// needed for dot (M, a, w)
	
	w[0] = x - dot ( M, a, w );		// input adder
	y = dot ( L, b, w );			// output adder
	
	delay ( K, w );					// update delay line
	
	return y;						// current output sample
}


// fir filter - no feedback = high stability but also may be more expensive computationally

inline int fir_filter ( int M, int *h, int *w, int x )
{
	int i;
	int y;

	w[0] = x;

	for ( y = 0, i = 0; i <= M; i++ )
		y += h[i] * w[i];

	for ( i = M; i >= -1; i-- )
		w[i] = w[i-1];

	return y;
}

// fir filter, using dot product and delay implementation

inline int fir_filter2 ( int M, int *h, int *w, int x )
{
	int y;

	w[0] = x;

	y = dot ( M, h, w );

	delay ( M, w );

	return y;
}


// tap - i-th tap of circular delay line buffer
// D delay line size in samples
// w delay line buffer pointer, of dimension D+1
// p circular pointer
// t = 0...D

int tap ( int D, int *w, int *p, int t )
{
	return w[(p - w + t) % (D + 1)];
}

// tapi - interpolated tap output of a delay line
//        interpolates sample between adjacent samples in delay line for 'frac' part of delay
// D delay line size in samples
// w delay line buffer pointer, of dimension D+1
// p circular pointer
// t - delay tap integer value 0...D.  (complete delay is t.frac )
// frac - varying 16 bit fractional delay value 0...32767 (normalized to 0.0 - 1.0)

inline int tapi ( int D, int *w, int *p, int t, int frac )
{
	int i, j;
	int si, sj;

	i = t;					// tap value, interpolate between adjacent samples si and sj
	j = (i + 1) % (D+1);	// if i = D, then j = 0; otherwise, j = i + 1
	
	si = tap( D, w, p, i );	// si(n) = x(n - i)
	sj = tap( D, w, p, j );	// sj(n) = x(n - j)

	return si + (((frac) * (sj - si) ) >> 16);
}

// circular delay line, D-fold delay
// D delay line size in samples w[0..D]
// w delay line buffer pointer, dimension D+1
// p circular pointer

inline void cdelay ( int D, int *w, int **p )
{
	(*p)--;					// decrement pointer and wrap modulo (D+1)
	wrap ( D, w, p );		// when *p = w-1, it wraps around to *p = w+D
}

// plain reverberator with circular delay line
// D delay line size in samples
// t tap from this location - <= D
// w delay line buffer pointer of dimension D+1
// p circular pointer, must be init to &w[0] before first call
// a feedback value, 0-PMAX (normalized to 0.0-1.0)
// b gain
// x input sample

//                    w0(n)   b	 
//  x(n)--->(+)--------.-----(*)-> y(n)
//           ^         |            
//           |     [Delay d]        
//           |         |            
//           |    a    |Wd(n)       
//			 ----(*)---.

inline int dly_plain ( int D, int t, int *w, int **p, int a, int b, int x )
{
	int y, sD;

	sD = tap ( D, w, *p, t );		// Tth tap delay output
	y = x + (( a * sD ) >> PBITS);		// filter output
	**p = y;						// delay input
	cdelay ( D, w, p );				// update delay line

	return ( (y * b) >> PBITS );
}

// straight delay line
//
// D delay line size in samples
// t tap from this location - <= D
// w delay line buffer pointer of dimension D+1
// p circular pointer, must be init to &w[0] before first call
// x input sample
//                    
//  x(n)--->[Delay d]---> y(n)
//
 
inline int dly_linear ( int D, int t, int *w, int **p, int x )
{
	int y;

	y = tap ( D, w, *p, t );		// Tth tap delay output
	**p = x;						// delay input
	cdelay ( D, w, p );				// update delay line

	return ( y );
}

// lowpass reverberator, replace feedback multiplier 'a' in 
// plain reverberator with a low pass filter
// D delay line size in samples
// t tap from this location - <= D
// w delay line buffer pointer of dimension D+1
// p circular pointer, must be init to &w[0] before first call
// a feedback gain
// b output gain
// M filter order
// bf filter numerator, 0-PMAX (normalized to 0.0-1.0), M+1 dimensional
// af filter denominator, 0-PMAX (normalized to 0.0-1.0), M+1 dimensional
// vf filter state, M+1 dimensional
// x input sample
//                    w0(n)    	   b
//  x(n)--->(+)--------------.----(*)--> y(n)
//           ^               |            
//           |           [Delay d]        
//           |               |            
//           |  a            |Wd(n)       
//			 --(*)--[Filter])-

int dly_lowpass ( int D, int t, int *w, int **p, int a, int b, int M, int *af, int L, int *bf, int *vf, int x )
{
	int y, sD;

	sD = tap ( D, w, *p, t );										// delay output is filter input
	y = x + ((iir_filter ( M, af, L, bf, vf, sD ) * a) >> PBITS);	// filter output with gain
	**p = y;														// delay input
	cdelay ( D, w, p );												// update delay line

	return ( (y * b) >> PBITS ); // output with gain
}

// allpass reverberator with circular delay line
// D delay line size in samples
// t tap from this location - <= D
// w delay line buffer pointer of dimension D+1
// p circular pointer, must be init to &w[0] before first call
// a feedback value, 0-PMAX (normalized to 0.0-1.0)
// b gain

//                    w0(n)   -a	     b
//  x(n)--->(+)--------.-----(*)-->(+)--(*)-> y(n)
//           ^         |            ^
//           |     [Delay d]        |
//           |         |            |
//           |    a    |Wd(n)       |
//			 ----(*)---.-------------
//
//	for each input sample x, do:
//		w0 = x + a*Wd
//		y = -a*w0 + Wd
//		delay (d, W) - w is the delay buffer array
//
// or, using circular delay, for each input sample x do:
//
//		Sd = tap (D,w,p,D)
//		S0 = x + a*Sd
//		y = -a*S0 + Sd
//		*p = S0
//		cdelay(D, w, &p)		

inline int dly_allpass ( int D, int t, int *w, int **p, int a, int b, int x ) 
{
	int y, s0, sD;
	
	sD = tap ( D, w, *p, t );			// Dth tap delay output
	s0 = x + (( a * sD ) >> PBITS);

	y = ( ( -a * s0 ) >> PBITS ) + sD;	// filter output
	**p = s0;							// delay input
	cdelay ( D, w, p );					// update delay line

	return ( (y * b) >> PBITS );
}


///////////////////////////////////////////////////////////////////////////////////
// fixed point math for real-time wave table traversing, pitch shifting, resampling
///////////////////////////////////////////////////////////////////////////////////

#define FIX20_BITS			20									// 20 bits of fractional part
#define FIX20_SCALE			(1 << FIX20_BITS)

#define FIX20_INTMAX		((1 << (32 - FIX20_BITS))-1)		// maximum step integer

#define FLOAT_TO_FIX20(a)	((int)((a) * (float)FIX20_SCALE))		// convert float to fixed point
#define INT_TO_FIX20(a)		(((int)(a)) << FIX20_BITS)			// convert int to fixed point
#define FIX20_TO_FLOAT(a)	((float)(a) / (float)FIX20_SCALE)	// convert fix20 to float
#define FIX20_INTPART(a)	(((int)(a)) >> FIX20_BITS)			// get integer part of fixed point
#define FIX20_FRACPART(a)	((a) - (((a) >> FIX20_BITS) << FIX20_BITS))	// get fractional part of fixed point

#define FIX20_FRACTION(a,b)	(FIX(a)/(b))						// convert int a to fixed point, divide by b

typedef int fix20int;

/////////////////////////////////
// DSP processor parameter block
/////////////////////////////////

// NOTE: these prototypes must match the XXX_Params ( prc_t *pprc ) and XXX_GetNext ( XXX_t *p, int x ) functions

typedef void * (*prc_Param_t)( void *pprc );					// individual processor allocation functions
typedef int (*prc_GetNext_t) ( void *pdata, int x );			// get next function for processor
typedef int (*prc_GetNextN_t) ( void *pdata,  portable_samplepair_t *pbuffer, int SampleCount, int op);	// batch version of getnext
typedef void (*prc_Free_t) ( void *pdata );						// free function for processor
typedef void (*prc_Mod_t) (void *pdata, float v);				// modulation function for processor	

#define	OP_LEFT				0		// batch process left channel in place
#define OP_RIGHT			1		// batch process right channel in place
#define OP_LEFT_DUPLICATE	2		// batch process left channel in place, duplicate to right channel

#define PRC_NULL			0		// pass through - must be 0
#define PRC_DLY				1		// simple feedback reverb
#define PRC_RVA				2		// parallel reverbs
#define PRC_FLT				3		// lowpass or highpass filter
#define PRC_CRS				4		// chorus
#define	PRC_PTC				5		// pitch shifter
#define PRC_ENV				6		// adsr envelope
#define PRC_LFO				7		// lfo
#define PRC_EFO				8		// envelope follower
#define PRC_MDY				9		// mod delay
#define PRC_DFR				10		// diffusor - n series allpass delays
#define PRC_AMP				11		// amplifier with distortion

#define QUA_LO				0		// quality of filter or reverb.  Must be 0,1,2,3.
#define QUA_MED				1
#define QUA_HI				2
#define QUA_VHI				3
#define QUA_MAX				QUA_VHI

#define CPRCPARAMS			16		// up to 16 floating point params for each processor type

// processor definition - one for each running instance of a dsp processor

struct prc_t
{
	int type;					// PRC type

	float prm[CPRCPARAMS];		// dsp processor parameters - array of floats

	prc_Param_t pfnParam;		// allocation function - takes ptr to prc, returns ptr to specialized data struct for proc type
	prc_GetNext_t pfnGetNext;	// get next function
	prc_GetNextN_t pfnGetNextN;	// batch version of get next
	prc_Free_t pfnFree;			// free function
	prc_Mod_t pfnMod;			// modulation function

	void *pdata;				// processor state data - ie: pdly, pflt etc.
};

// processor parameter ranges - for validating parameters during allocation of new processor

typedef struct prm_rng_t
{
	int iprm;		// parameter index
	float lo;		// min value of parameter
	float hi;		// max value of parameter
} prm_rng_s;

void PRC_CheckParams ( prc_t *pprc, prm_rng_t *prng );

///////////
// Filters
///////////

#define CFLTS	64			// max number of filters simultaneously active
#define FLT_M	12			// max order of any filter

#define FLT_LP	0			// lowpass filter
#define FLT_HP	1			// highpass filter
#define FTR_MAX	FLT_HP

// flt parameters

struct flt_t
{
	bool fused;				// true if slot in use

	int b[FLT_M+1];			// filter numerator parameters  (convert 0.0-1.0 to 0-PMAX representation)
	int a[FLT_M+1];			// filter denominator parameters (convert 0.0-1.0 to 0-PMAX representation)
	int w[FLT_M+1];			// filter state - samples (dimension of max (M, L))
	int L;					// filter order numerator (dimension of a[M+1])
	int M;					// filter order denominator (dimension of b[L+1])
};

// flt flts

flt_t flts[CFLTS];

void FLT_Init ( flt_t *pf ) { if ( pf ) Q_memset ( pf, 0, sizeof (flt_t) ); }
void FLT_InitAll ( void ) {	for ( int i = 0 ; i < CFLTS; i++ ) FLT_Init ( &flts[i] ); }
void FLT_Free ( flt_t *pf ) { if ( pf )	Q_memset ( pf, 0, sizeof (flt_t) );	}
void FLT_FreeAll ( void ) {	for (int i = 0 ; i < CFLTS; i++) FLT_Free ( &flts[i] ); }


// find a free filter from the filter pool
// initialize filter numerator, denominator b[0..M], a[0..L]

flt_t * FLT_Alloc ( int M, int L, int *a, int *b )
{
	int i, j;
	flt_t *pf = NULL;
	
	for (i = 0; i < CFLTS; i++)
	{
		if ( !flts[i].fused )
			{
			pf = &flts[i];

			// transfer filter params into filter struct
			pf->M = M;
			pf->L = L;
			for (j = 0; j <= M; j++)
				pf->a[j] = a[j];

			for (j = 0; j <= L; j++)
				pf->b[j] = b[j];

			pf->fused = true;
			break;
			}
	}

	Assert(pf);	// make sure we're not trying to alloc more than CFLTS flts

	return pf;
}

// convert filter params cutoff and type into
// iir transfer function params M, L, a[], b[]

// iir filter, 1st order, transfer function is H(z) = b0 + b1 Z^-1  /  a0 + a1 Z^-1
// or H(z) = b0 - b1 Z^-1 / a0 + a1 Z^-1 for lowpass

// design cutoff filter at 3db (.5 gain) p579

void FLT_Design_3db_IIR ( float cutoff, float ftype, int *pM, int *pL, int *a, int *b )
{
	// ftype: FLT_LP, FLT_HP, FLT_BP
	
	double Wc = 2.0 * M_PI * cutoff / SOUND_DMA_SPEED;			// radians per sample
	double Oc;
	double fa;
	double fb; 

	// calculations:
	// Wc = 2pi * fc/44100								convert to radians
	// Oc = tan (Wc/2) * Gc / sqt ( 1 - Gc^2)			get analog version, low pass
	// Oc = tan (Wc/2) * (sqt (1 - Gc^2)) / Gc			analog version, high pass
	// Gc = 10 ^ (-Ac/20)								gain at cutoff.  Ac = 3db, so Gc^2 = 0.5
	// a = ( 1 - Oc ) / ( 1 + Oc )
	// b = ( 1 - a ) / 2

	Oc = tan ( Wc / 2.0 );

	fa = ( 1.0 - Oc ) / ( 1.0 + Oc );

	fb = ( 1.0 - fa ) / 2.0;

	if ( ftype == FLT_HP )
		fb = ( 1.0 + fa ) / 2.0;

	a[0] = 0;						// a0 always ignored
	a[1] = (int)( -fa * PMAX );		// quantize params down to 0-PMAX >> PBITS
	b[0] = (int)( fb * PMAX );
	b[1] = b[0];
	
	if ( ftype == FLT_HP )
		b[1] = -b[1];

	*pM = *pL = 1;

	return;
}


// convolution of x[n] with h[n], resulting in y[n]
// h, x, y filter, input and output arrays (double precision)
// M = filter order, L = input length
// h is M+1 dimensional
// x is L dimensional
// y is L+M dimensional

void conv ( int M, double *h, int L, double *x, double *y )
{
	int n, m;

	for ( n = 0; n < L+M; n++ )
	{
		for (y[n] = 0, m = max(0, n-L+1); m <= min(n, M); m++ )
		{
			y[n] += h[m] * x[n-m];
		}
	}
}

// cas2can - convert cascaded, second order section parameter arrays to 
// canonical numerator/denominator arrays.  Canonical implementations
// have half as many multiplies as cascaded implementations.

// K is number of cascaded sections
// A is Kx3 matrix of sos params A[K] = A[0]..A[K-1]
// a is (2K + 1) -dimensional output of canonical params

#define KMAX	32			// max # of sos sections - 8 is the most we should ever see at runtime

void cas2can ( int K, double A[KMAX+1][3], int *aout )
{
	int i, j;
	double d[2*KMAX + 1];
	double a[2*KMAX + 1];

	Assert ( K <= KMAX );
	
	Q_memset(d, 0, sizeof (double) * (2 * KMAX + 1));
	Q_memset(a, 0, sizeof (double) * (2 * KMAX + 1));

	a[0] = 1;

	for (i = 0; i < K; i++)
	{
		conv( 2, A[i], 2*i + 1, a, d );

		for ( j = 0; j < 2*i + 3; j++ )
			a[j] = d[j];
	}

	for (i = 0; i < (2*K + 1); i++)
		aout[i] = a[i] * PMAX;
}


// chebyshev IIR design, type 2, Lowpass or Highpass

#define lnf(e) (2.303 * log10 (e))

#define acosh(e)  ( lnf( (e) + sqrt((e)*(e) - 1) ) )
#define asinh(e)  ( lnf( (e) + sqrt((e)*(e) + 1) ) )


// returns a[], b[] which are Kx3 matrices of cascaded second-order sections
// these matrices may be passed directly to the iir_cas() routine for evaluation
// Nmax - maximum order of filter
// cutoff, ftype, qwidth - filter cutoff in hz, filter type FLT_LOWPASS/HIGHPASS, qwidth in hz
// pM - denominator order
// pL - numerator order
// a - array of canonical filter params
// b - array of canonical filter params

void FLT_Design_Cheb ( int Nmax, float cutoff, float ftype, float qwidth, int *pM, int *pL, int *a, int *b )
{
// p769 - converted from MATLAB

	double s =		(ftype == FLT_LP ? 1 : -1 );		// 1 for LP, -1 for HP
	double fs =		SOUND_DMA_SPEED;					// sampling frequency
	double fpass =	cutoff;								// cutoff frequency
	double fstop =	fpass + max (2000, qwidth);	// stop frequency
	double Apass =	0.5;								// max attenuation of pass band UNDONE: use Quality to select this
	double Astop =	10;									// max amplitude of stop band	UNDONE: use Quality to select this

	double Wpass, Wstop, epass, estop, Nex, aa, W3, f3, W0, G, Wi2, W02, a1, a2, th, Wi, D, b1;
	int K, r, N;
	double A[KMAX+1][3];								// denominator output matrices, second order sections
	double B[KMAX+1][3];								// numerator output matrices, second order sections

	Wpass = tan( M_PI * fpass / fs ); Wpass = powf (Wpass, s);
	Wstop = tan( M_PI * fstop / fs ); Wstop = powf (Wstop, s);

	epass = sqrt( pow( 10, Apass/10 ) - 1 );
	estop = sqrt( pow( 10, Astop/10 ) - 1 );

	// calculate filter order N

	Nex = acosh( estop/epass ) / acosh ( Wstop/Wpass );
	N = min ( ceil(Nex), Nmax );	// don't exceed Nmax for filter order
	r = ( (int)N & 1);				// r == 1 if N is odd
	K = (N - r ) / 2;

	aa = asinh ( estop ) / N;
	W3 = Wstop / cosh( acosh(estop)/N );
	f3 = (fs / M_PI) * atan( pow( W3, s ) );
	
	W0 = sinh( aa ) / Wstop;
	W02 = W0 * W0;

	// 1st order section for N odd

	if ( r == 1 )
	{
		G = 1 / (1 + W0);
		A[0][0] = 1; A[0][1] = s * (2*G-1); A[0][2] = 0;
		B[0][0] = G; B[0][1] = G*s;			B[0][2] = 0;
	}
	else
	{
		A[0][0] = 1; A[0][1] = 0; A[0][2] = 0;
		B[0][0] = 1; B[0][1] = 0; B[0][2] = 0;
	}

	for (int i = 1; i <= K ; i++ )
	{
		th = M_PI * (N - 1 + 2 * i) / (2 * N);
		Wi = sin(th) / Wstop;
		Wi2 = Wi * Wi;

		D = 1 - 2 * W0 * cos(th) + W02 + Wi2;
		G = ( 1 + Wi2 ) / D;

		b1 = 2 * ( 1 - Wi2 ) / ( 1 + Wi2 );
		a1 = 2 * ( 1 - W02 - Wi2) / D;
		a2 = ( 1 + 2 * W0 * cos(th) + W02 + Wi2) / D;

		A[i][0] = 1;
		A[i][1] = s * a1;
		A[i][2] = a2;
		
		B[i][0] = G;
		B[i][1] = G* s* b1;
		B[i][2] = G;
	}

	// convert cascade parameters to canonical parameters

	cas2can ( K, A, a );
	*pM = 2*K + 1;

	cas2can ( K, B, b );
	*pL = 2*K + 1;
}

// filter parameter order
	
typedef enum
{
	flt_iftype,				
	flt_icutoff,			
	flt_iqwidth,			
	flt_iquality,			

	flt_cparam				// # of params
} flt_e;

// filter parameter ranges

prm_rng_t flt_rng[] = {

	{flt_cparam,	0, 0},			// first entry is # of parameters

	{flt_iftype,	0, FTR_MAX},	// filter type FLT_LP, FLT_HP, FLT_BP (UNDONE: FLT_BP currently ignored)			
	{flt_icutoff,	10, 22050},		// cutoff frequency in hz at -3db gain
	{flt_iqwidth,	100, 11025},	// width of BP, or steepness of LP/HP (ie: fcutoff + qwidth = -60db gain point)
	{flt_iquality,	0, QUA_MAX},	// QUA_LO, _MED, _HI 0,1,2,3 
};


// convert prc float params to iir filter params, alloc filter and return ptr to it
// filter quality set by prc quality - 0,1,2

flt_t * FLT_Params ( prc_t *pprc )
{
	float qual		= pprc->prm[flt_iquality];
	float cutoff	= pprc->prm[flt_icutoff];
	float ftype		= pprc->prm[flt_iftype];
	float qwidth	= pprc->prm[flt_iqwidth];
	
	int L = 0;					// numerator order
	int M = 0;					// denominator order
	int b[FLT_M+1];				// numerator params	 0..PMAX
	int a[FLT_M+1];				// denominator params 0..PMAX
	
	// low pass and highpass filter design 
	
	if ( (int) qual == QUA_LO) qual = QUA_MED;	// disable lowest quality filter - check perf on lowend KDB

	switch ( (int)qual ) 
	{
	case QUA_LO:
		
		// lowpass averaging filter: perf KDB
		
		Assert ( ftype == FLT_LP );
		Assert ( cutoff <= SOUND_DMA_SPEED );
		M = 0;
		
		// L is # of samples to average

		L = 0;
		if ( cutoff <= SOUND_DMA_SPEED / 4) L = 1;		// 11k
		if ( cutoff <= SOUND_DMA_SPEED / 8) L = 2;		// 5.5k
		if ( cutoff <= SOUND_DMA_SPEED / 16) L = 4;		// 2.75k
		if ( cutoff <= SOUND_DMA_SPEED / 32) L = 8;		// 1.35k
		if ( cutoff <= SOUND_DMA_SPEED / 64) L = 12;	// 750hz
		
		break;
	case QUA_MED:
		//	1st order IIR filter, 3db cutoff at fc
		FLT_Design_3db_IIR ( cutoff, ftype, &M, &L, a, b );
			
		M = clamp (M, 1, FLT_M);
		L = clamp (L, 1, FLT_M);

		break;
	case QUA_HI:
		// type 2 chebyshev N = 4 IIR 
		FLT_Design_Cheb ( 4, cutoff, ftype, qwidth, &M, &L, a, b );
			
		M = clamp (M, 1, FLT_M);
		L = clamp (L, 1, FLT_M);

		break;
	case QUA_VHI:
	// type 2 chebyshev N = 7 IIR
		FLT_Design_Cheb ( 8, cutoff, ftype, qwidth, &M, &L, a, b );

			
		M = clamp (M, 1, FLT_M);
		L = clamp (L, 1, FLT_M);

		break;
	}

	return FLT_Alloc ( M, L, a, b );
}

inline void * FLT_VParams ( void *p ) 
{
	PRC_CheckParams( (prc_t *)p, flt_rng);
	return (void *) FLT_Params ((prc_t *)p); 
}

inline void FLT_Mod ( void *p, float v ) { return; }

// get next filter value for filter pf and input x

inline int FLT_GetNext ( flt_t *pf, int  x )
{
	return iir_filter (pf->M, pf->a, pf->L, pf->b, pf->w, x);
	// return iir_filter2 (pf->M, pf->a, pf->L, pf->b, pf->w, x);
}

// batch version for performance

inline void FLT_GetNextN( flt_t *pflt, portable_samplepair_t *pbuffer, int SampleCount, int op )
{
	int count = SampleCount;
	portable_samplepair_t *pb = pbuffer;
	
	switch (op)
	{
	default:
	case OP_LEFT:
		while (count--)
		{
			pb->left = FLT_GetNext( pflt, pb->left );
			pb++;
		}
		return;
	case OP_RIGHT:
		while (count--)
		{
			pb->right = FLT_GetNext( pflt, pb->right );
			pb++;
		}
		return;
	case OP_LEFT_DUPLICATE:
		while (count--)
		{
			pb->left = pb->right = FLT_GetNext( pflt, pb->left );
			pb++;
		}
		return;
	}
}

///////////////////////////////////////////////////////////////////////////
// Positional updaters for pitch shift etc
///////////////////////////////////////////////////////////////////////////

// looping position within a wav, with integer and fractional parts
// used for pitch shifting, upsampling/downsampling
// 20 bits of fraction, 8+ bits of integer

struct pos_t
{

	fix20int step;	// wave table whole and fractional step value
	fix20int cstep;	// current cummulative step value
	int pos;		// current position within wav table
	
	int D;			// max dimension of array w[0...D] ie: # of samples = D+1
};

// circular wrap of pointer p, relative to array w
// D max buffer index w[0...D] (count of samples in buffer is D+1)
// i circular index

inline void POS_Wrap ( int D, int *i )
{
	if ( *i > D )
		*i -= D + 1;		// when *pi = D + 1, it wraps around to *pi = 0
	
	if ( *i < 0 )
		*i += D + 1;		// when *pi = - 1, it wraps around to *pi = D
}

// set initial update value - fstep can have no more than 8 bits of integer and 20 bits of fract
// D is array max dimension w[0...D] (ie: size D+1)
// w is ptr to array
// p is ptr to pos_t to initialize

inline void POS_Init( pos_t *p, int D, float fstep )
{
	float step = fstep;

	// make sure int part of step is capped at fix20_intmax

	if ((int)step > FIX20_INTMAX)
		step = (step - (int)step) + FIX20_INTMAX;

	p->step = FLOAT_TO_FIX20(step);	// convert fstep to fixed point
	p->cstep = 0;			
	p->pos = 0;							// current update value

	p->D = D;							// always init to end value, in case we're stepping backwards
}

// change step value - this is an instantaneous change, not smoothed.

inline void POS_ChangeVal( pos_t *p, float fstepnew )
{
	p->step = FLOAT_TO_FIX20( fstepnew );	// convert fstep to fixed point
}

// return current integer position, then update internal position value

inline int POS_GetNext ( pos_t *p )
{
	
	//float f = FIX20_TO_FLOAT(p->cstep);
	//int i1 = FIX20_INTPART(p->cstep);
	//float f1 = FIX20_TO_FLOAT(FIX20_FRACPART(p->cstep));
	//float f2 = FIX20_TO_FLOAT(p->step);

	p->cstep += p->step;						// update accumulated fraction step value (fixed point)
	p->pos += FIX20_INTPART( p->cstep );		// update pos with integer part of accumulated step
	p->cstep = FIX20_FRACPART( p->cstep );		// throw away the integer part of accumulated step

	// wrap pos around either end of buffer if needed

	POS_Wrap(p->D, &(p->pos));

	// make sure returned position is within array bounds

	Assert (p->pos <= p->D);

	return p->pos;
}

// oneshot position within wav 
struct pos_one_t
{
	pos_t p;				// pos_t
	bool fhitend;			// flag indicating we hit end of oneshot wav
};

// set initial update value - fstep can have no more than 8 bits of integer and 20 bits of fract
// one shot position - play only once, don't wrap, when hit end of buffer, return last position

inline void POS_ONE_Init( pos_one_t *p1, int D, float fstep )
{
	POS_Init( &p1->p, D, fstep ) ;
	
	p1->fhitend = false;
}

// return current integer position, then update internal position value

inline int POS_ONE_GetNext ( pos_one_t *p1 )
{
	int pos;
	pos_t *p0;

	pos = p1->p.pos;							// return current position
	
	if (p1->fhitend)
		return pos;

	p0 = &(p1->p);
	p0->cstep += p0->step;						// update accumulated fraction step value (fixed point)
	p0->pos += FIX20_INTPART( p0->cstep );		// update pos with integer part of accumulated step
	//p0->cstep = SIGN(p0->cstep) * FIX20_FRACPART( p0->cstep );
	p0->cstep = FIX20_FRACPART( p0->cstep );		// throw away the integer part of accumulated step

	// if we wrapped, stop updating, always return last position
	// if step value is 0, return hit end

	if (!p0->step || p0->pos < 0 || p0->pos >= p0->D )
		p1->fhitend = true;
	else
		pos = p0->pos;

	// make sure returned value is within array bounds

	Assert ( pos <= p0->D );

	return pos;
}


/////////////////////
// Reverbs and delays
/////////////////////

#define CDLYS			128				// max delay lines active. Also used for lfos.

#define DLY_PLAIN		0				// single feedback loop
#define DLY_ALLPASS		1				// feedback and feedforward loop - flat frequency response (diffusor)
#define DLY_LOWPASS		2				// lowpass filter in feedback loop
#define DLY_LINEAR		3				// linear delay, no feedback, unity gain
#define DLY_MAX			DLY_LINEAR

// delay line 

struct dly_t
{
	
	bool fused;						// true if dly is in use
	int type;						// delay type

	int D;							// delay size, in samples
	int t;							// current tap, <= D
	int D0;							// original delay size (only relevant if calling DLY_ChangeVal)
	int *p;							// circular buffer pointer
	int *w;							// array of samples

	int a;							// feedback value 0..PMAX,normalized to 0-1.0
	int b;							// gain value 0..PMAX, normalized to 0-1.0

	flt_t *pflt;					// pointer to filter, if type DLY_LOWPASS

	HANDLE h;						// memory handle for sample array	
};

dly_t dlys[CDLYS];					// delay lines

void DLY_Init ( dly_t *pdly ) {	if ( pdly )	Q_memset( pdly, 0, sizeof (dly_t)); }
void DLY_InitAll ( void ) {	for (int i = 0 ; i < CDLYS; i++) DLY_Init ( &dlys[i] ); }
void DLY_Free ( dly_t *pdly )
{
	// free memory buffer

	if ( pdly )
	{
		FLT_Free ( pdly->pflt );

		if ( pdly->w )
		{
			GlobalUnlock( pdly->h );
			GlobalFree( pdly->h );
		}
		
		// free dly slot

		Q_memset ( pdly, 0, sizeof (dly_t) );
	}
}


void DLY_FreeAll ( void ) {	for (int i = 0; i < CDLYS; i++ ) DLY_Free ( &dlys[i] ); }

// set up 'b' gain parameter of feedback delay to
// compensate for gain caused by feedback.  

void DLY_SetNormalizingGain ( dly_t *pdly )
{
	// compute normalized gain, set as output gain

	// calculate gain of delay line with feedback, and use it to
	// reduce output.  ie: force delay line with feedback to unity gain

	// for constant input x with feedback fb:

	// out = x + x*fb + x * fb^2 + x * fb^3...
	// gain = out/x
	// so gain = 1 + fb + fb^2 + fb^3...
	// which, by the miracle of geometric series, equates to 1/1-fb
	// thus, gain = 1/(1-fb)
	
	float fgain = 0;
	float gain;
	int b;

	// if b is 0, set b to PMAX (1)

	b = pdly->b ? pdly->b : PMAX;

	// fgain = b * (1.0 / (1.0 - (float)pdly->a / (float)PMAX)) / (float)PMAX;

	fgain = (1.0 / (1.0 - (float)pdly->a / (float)PMAX));

	// compensating gain -  multiply rva output by gain then >> PBITS

	gain = (int)((1.0 / fgain) * PMAX);	

	gain = gain * 4;	// compensate for fact that gain calculation is for +/- 32767 amplitude wavs
						// ie: ok to allow a bit more gain because most wavs are not at theoretical peak amplitude at all times

	gain = min (gain, PMAX);	// cap at PMAX
	
	gain = ((float)b/(float)PMAX) * gain;	// scale final gain by pdly->b.

	pdly->b = (int)gain;
}

// allocate a new delay line
// D number of samples to delay
// a feedback value (0-PMAX normalized to 0.0-1.0)
// b gain value (0-PMAX normalized to 0.0-1.0)
// if DLY_LOWPASS:
//		L - numerator order of filter
//		M - denominator order of filter
//		fb - numerator params, M+1 
//		fa - denominator params, L+1

dly_t * DLY_AllocLP ( int D, int a, int b, int type, int M, int L, int *fa, int *fb )
{
	HANDLE h;
	int cb;
	int *w;
	int i;
	dly_t *pdly = NULL;

	// find open slot

	for (i = 0; i < CDLYS; i++)
	{
		if (!dlys[i].fused)
		{
			pdly = &dlys[i];
			DLY_Init( pdly );
			break;
		}
	}
	
	if ( i == CDLYS )
	{
		DevMsg ("DSP: Warning, failed to allocate delay line.\n" );
		return NULL;					// all delay lines in use
	}

	cb = (D + 1) * sizeof ( int );		// assume all samples are signed integers

	if (type == DLY_LOWPASS)
	{
		// alloc lowpass fir_filter
	
		pdly->pflt = FLT_Alloc( M, L, fa, fb );
		if ( !pdly->pflt )
		{
			DevMsg ("DSP: Warning, failed to allocate filter for delay line.\n" );
			return NULL;
		}
	}

	// alloc delay memory
	
	h = GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE, cb); 
	if (!h) 
	{ 
		g_pSoundServices->ConSafePrint ("Sound DSP: Out of memory.\n");
		FLT_Free ( pdly->pflt );
		return NULL; 
	}
	
	// lock delay memory

	w = (int *)GlobalLock(h);

	if ( !w )
	{ 
		g_pSoundServices->ConSafePrint ("Sound DSP: Failed to lock.\n");
		GlobalFree(h);
		FLT_Free ( pdly->pflt );
		return NULL; 
	}
	
	// clear delay array

	Q_memset (w, 0, cb);
	
	// init values

	pdly->type = type;
	pdly->D = D;
	pdly->t = D;		// set delay tap to full delay
	pdly->D0 = D;
	pdly->p = w;		// init circular pointer to head of buffer
	pdly->w = w;
	pdly->h = h;
	pdly->a = min( a, PMAX );		// do not allow 100% feedback
	pdly->b = b;
	pdly->fused = true;

	if ( type == DLY_LINEAR ) 
	{
		// linear delay has no feedback and unity gain

		pdly->a = 0;
		pdly->b = PMAX;
	}
	else
	{
		// adjust b to compensate for feedback gain

		DLY_SetNormalizingGain( pdly );	
	}

	return (pdly);
}

// allocate lowpass or allpass delay

dly_t * DLY_Alloc( int D, int a, int b, int type )
{
	return DLY_AllocLP( D, a, b, type, 0, 0, 0, 0 );
}


// Allocate new delay, convert from float params in prc preset to internal parameters
// Uses filter params in prc if delay is type lowpass

// delay parameter order

typedef enum {

 dly_idtype,		// NOTE: first 8 params must match those in mdy_e
 dly_idelay,		
 dly_ifeedback,		
 dly_igain,		

 dly_iftype,		
 dly_icutoff,	
 dly_iqwidth,		
 dly_iquality, 
	
 dly_cparam

} dly_e;


// delay parameter ranges

prm_rng_t dly_rng[] = {

	{dly_cparam,	0, 0},			// first entry is # of parameters
		
	// delay params

	{dly_idtype,	0, DLY_MAX},		// delay type DLY_PLAIN, DLY_LOWPASS, DLY_ALLPASS	
	{dly_idelay,	0.0, 1000.0},		// delay in milliseconds
	{dly_ifeedback,	0.0, 0.99},			// feedback 0-1.0
	{dly_igain,	    0.0, 1.0},			// final gain of output stage, 0-1.0

	// filter params if dly type DLY_LOWPASS

	{dly_iftype,	0, FTR_MAX},			
	{dly_icutoff,	10.0, 22050.0},
	{dly_iqwidth,	100.0, 11025.0},
	{dly_iquality,	0, QUA_MAX},
};

dly_t * DLY_Params ( prc_t *pprc )
{
	dly_t *pdly = NULL;
	int D, a, b;
	
	float delay		= pprc->prm[dly_idelay];
	float feedback	= pprc->prm[dly_ifeedback];
	float gain		= pprc->prm[dly_igain];
	int type		= pprc->prm[dly_idtype];

	float ftype 	= pprc->prm[dly_iftype];
	float cutoff	= pprc->prm[dly_icutoff];
	float qwidth	= pprc->prm[dly_iqwidth];
	float qual		= pprc->prm[dly_iquality];

	D = MSEC_TO_SAMPS(delay);					// delay samples
	a = feedback * PMAX;						// feedback
	b = gain * PMAX;							// gain
	
	switch ( (int) type )
	{
	case DLY_PLAIN:
	case DLY_ALLPASS:
	case DLY_LINEAR:
		pdly = DLY_Alloc( D, a, b, type );
		break;

	case DLY_LOWPASS: 
		{
		// set up dummy lowpass filter to convert params

		prc_t prcf;

		prcf.prm[flt_iquality]	= qual;	// 0,1,2 - high, medium, low (low quality implies faster execution time)
		prcf.prm[flt_icutoff]	= cutoff;
		prcf.prm[flt_iftype]	= ftype;
		prcf.prm[flt_iqwidth]	= qwidth;
	
		flt_t *pflt = (flt_t *)FLT_Params ( &prcf );
		
		if ( !pflt )
		{
			DevMsg ("DSP: Warning, failed to allocate filter.\n" );
			return NULL;
		}

		pdly = DLY_AllocLP ( D, a, b, type, pflt->M, pflt->L, pflt->a, pflt->b );

		FLT_Free ( pflt );
		break;
		}
	}

	return pdly;
}

inline void * DLY_VParams ( void *p ) 
{
	PRC_CheckParams( (prc_t *)p, dly_rng );
	return (void *) DLY_Params ((prc_t *)p); 
}

// get next value from delay line, move x into delay line

int DLY_GetNext ( dly_t *pdly, int x )
{
	switch (pdly->type)
	{
	default:
	case DLY_PLAIN:
		return dly_plain( pdly->D, pdly->t, pdly->w, &pdly->p, pdly->a, pdly->b, x );
	case DLY_ALLPASS:
		return dly_allpass( pdly->D, pdly->t, pdly->w, &pdly->p, pdly->a, pdly->b, x );
	case DLY_LOWPASS:
		return dly_lowpass( pdly->D, pdly->t, pdly->w, &(pdly->p), pdly->a, pdly->b, pdly->pflt->M, pdly->pflt->a, pdly->pflt->L, pdly->pflt->b, pdly->pflt->w, x );
	case DLY_LINEAR:
		return dly_linear( pdly->D, pdly->t, pdly->w, &pdly->p, x );
	}		
}

// batch version for performance

void DLY_GetNextN( dly_t *pdly, portable_samplepair_t *pbuffer, int SampleCount, int op )
{
	int count = SampleCount;
	portable_samplepair_t *pb = pbuffer;
	
	switch (op)
	{
	default:
	case OP_LEFT:
		while (count--)
		{
			pb->left = DLY_GetNext( pdly, pb->left );
			pb++;
		}
		return;
	case OP_RIGHT:
		while (count--)
		{
			pb->right = DLY_GetNext( pdly, pb->right );
			pb++;
		}
		return;
	case OP_LEFT_DUPLICATE:
		while (count--)
		{
			pb->left = pb->right = DLY_GetNext( pdly, pb->left );
			pb++;
		}
		return;
	}
}

// get tap on t'th sample in delay - don't update buffer pointers, this is done via DLY_GetNext

inline int DLY_GetTap ( dly_t *pdly, int t )
{
	return tap (pdly->D, pdly->w, pdly->p, t );
}


// make instantaneous change to new delay value D.
// t tap value must be <= original D (ie: we don't do any reallocation here)

void DLY_ChangeVal ( dly_t *pdly, int t )
{
	// never set delay > original delay

	pdly->t = min ( t, pdly->D0 );
}

// ignored - use MDY_ for modulatable delay

inline void DLY_Mod ( void *p, float v ) { return; }


///////////////////
// Parallel reverbs
///////////////////

// Reverb A
// M parallel reverbs, mixed to mono output

#define CRVAS				64				// max number of parallel series reverbs active

#define CRVA_DLYS			12				// max number of delays making up reverb_a

struct rva_t
{
	bool fused;
	int m;						// number of parallel plain or lowpass delays
	int fparallel;				// true if filters in parallel with delays, otherwise single output filter	
	flt_t *pflt;

	dly_t *pdlys[CRVA_DLYS];	// array of pointers to delays
};

rva_t rvas[CRVAS];

void RVA_Init ( rva_t *prva ) {	if ( prva )	Q_memset (prva, 0, sizeof (rva_t)); }
void RVA_InitAll( void ) { for (int i = 0; i < CRVAS; i++) RVA_Init ( &rvas[i] ); }

// free parallel series reverb

void RVA_Free( rva_t *prva )
{
	if ( prva )
	{
	// free all delays
	for (int i = 0; i < CRVA_DLYS; i++)
		DLY_Free ( prva->pdlys[i] );
	
	FLT_Free( prva->pflt );

	Q_memset( prva, 0, sizeof (rva_t) );
	}
}


void RVA_FreeAll( void ) { for (int i = 0; i < CRVAS; i++) RVA_Free( &rvas[i] ); }

// create parallel reverb - m parallel reverbs summed 

// D array of CRVB_DLYS reverb delay sizes max sample index w[0...D] (ie: D+1 samples)
// a array of reverb feedback parms for parallel reverbs (CRVB_P_DLYS)
// b array of CRVB_P_DLYS - mix params for parallel reverbs
// m - number of parallel delays
// pflt - filter template, to be used by all parallel delays
// fparallel - true if filter operates in parallel with delays, otherwise filter output only

rva_t * RVA_Alloc ( int *D, int *a, int *b, int m, flt_t *pflt, int fparallel )
{
	
	int i;
	rva_t *prva;
	flt_t *pflt2 = NULL;

	// find open slot

	for ( i = 0; i < CRVAS; i++ )
	{
		if ( !rvas[i].fused )
			break;
	}

	// return null if no free slots

	if (i == CRVAS)
	{
		DevMsg ("DSP: Warning, failed to allocate reverb.\n" );
		return NULL;
	}
	
	prva = &rvas[i];

	// if series filter specified, alloc

	if ( pflt && !fparallel)
	{
		// use filter data as template for a filter on output

		pflt2 = FLT_Alloc (pflt->M, pflt->L, pflt->a, pflt->b );

		if (!pflt2)
		{
			DevMsg ("DSP: Warning, failed to allocate flt for reverb.\n" );
			return NULL;
		}
	}
	
	// alloc parallel reverbs

	if ( pflt && fparallel )
	{

		// use this filter data as a template to alloc a filter for each parallel delay

		for (i = 0; i < m; i++)
			prva->pdlys[i] = DLY_AllocLP( D[i], a[i], b[i], DLY_LOWPASS, pflt->M, pflt->L, pflt->a, pflt->b );
	}	
	else 
	{
		// no filter specified, use plain delays in parallel sections

		for (i = 0; i < m; i++)
			prva->pdlys[i] = DLY_Alloc( D[i], a[i], b[i], DLY_PLAIN );
	}
	
	
	// if we failed to alloc any reverb, free all, return NULL

	for (i = 0; i < m; i++)
	{
		if ( !prva->pdlys[i])
		{
			FLT_Free( pflt2 );
			RVA_Free( prva );
			DevMsg ("DSP: Warning, failed to allocate delay for reverb.\n" );
			return NULL;
		}
	}

	prva->fused = true;
	prva->m = m;
	prva->fparallel = fparallel;
	prva->pflt = pflt2;

	return prva;
}


// parallel reverberator
//
// for each input sample x do:
//		x0 = plain(D0,w0,&p0,a0,x)
//		x1 = plain(D1,w1,&p1,a1,x)
//		x2 = plain(D2,w2,&p2,a2,x)
//		x3 = plain(D3,w3,&p3,a3,x)
//		y = b0*x0 + b1*x1 + b2*x2 + b3*x3
//
//		rgdly - array of 6 delays:
//		D - Delay values (typical - 29, 37, 44, 50, 27, 31)
//		w - array of delayed values
//		p - array of pointers to circular delay line pointers
//		a - array of 6 feedback values (typical - all equal, like 0.75 * PMAX)
//		b - array of 6 gain values for plain reverb outputs (1, .9, .8, .7)
//		xin - input value
// if fparallel, filters are built into delays,
// otherwise, filter output

inline int RVA_GetNext( rva_t *prva, int x )
{
	int m = prva->m;			
	int sum;

	int y;

	sum = 0;

		for (int i = 0; i < m; i++ )
			sum += DLY_GetNext( prva->pdlys[i], x );

	// m is clamped between RVA_BASEM & CRVA_DLYS
	
	if (m)
		y = sum/m;
	else
		y = x;
#if 0
	// PERFORMANCE:
	// UNDONE: build as array
	int mm;

	switch (m)
	{
	case 12: mm = (PMAX/12); break;
	case 11: mm = (PMAX/11); break;
	case 10: mm = (PMAX/10); break;
	case 9:  mm = (PMAX/9); break;
	case 8:  mm = (PMAX/8); break;
	case 7:  mm = (PMAX/7); break;
	case 6:  mm = (PMAX/6); break;
	case 5:  mm = (PMAX/5); break;
	case 4:  mm = (PMAX/4); break;
	case 3:  mm = (PMAX/3); break;
	case 2:  mm = (PMAX/2); break;
	default:
	case 1:  mm = (PMAX/1); break;
	}

	y = (sum * mm) >> PBITS;

#endif // 0

	// run series filter if present

	if ( prva->pflt && !prva->fparallel )
		y = FLT_GetNext( prva->pflt, y);
	
	return y;
}

// batch version for performance

inline void RVA_GetNextN( rva_t *prva, portable_samplepair_t *pbuffer, int SampleCount, int op )
{
	int count = SampleCount;
	portable_samplepair_t *pb = pbuffer;
	
	switch (op)
	{
	default:
	case OP_LEFT:
		while (count--)
		{
			pb->left = RVA_GetNext( prva, pb->left );
			pb++;
		}
		return;
	case OP_RIGHT:
		while (count--)
		{
			pb->right = RVA_GetNext( prva, pb->right );
			pb++;
		}
		return;
	case OP_LEFT_DUPLICATE:
		while (count--)
		{
			pb->left = pb->right = RVA_GetNext( prva, pb->left );
			pb++;
		}
		return;
	}
}

#define RVA_BASEM		3				// base number of parallel delays

// nominal delay and feedback values

//float rvadlys[] = {29,  37,  44,  50,  62,  75, 96, 118, 127, 143, 164, 175};
float rvadlys[] =   {18,  23,  28,  36,  47,  21, 26, 33,  40,  49,  45,  38};
float rvafbs[] = {0.7, 0.7, 0.7, 0.8, 0.8, 0.9, 0.9, 0.9, 0.9, 0.9, 0.9, 0.9};

// reverb parameter order
	
typedef enum
{

// parameter order

	rva_isize,		
	rva_idensity,	
	rva_idecay,	
	
	rva_iftype,		
	rva_icutoff,		
	rva_iqwidth,		
					
	rva_ifparallel,

	rva_cparam		// # of params
} rva_e;

// filter parameter ranges

prm_rng_t rva_rng[] = {

	{rva_cparam,	0, 0},			// first entry is # of parameters
	
		// reverb params

	{rva_isize,		0.0, 2.0},	// 0-2.0 scales nominal delay parameters (starting at approx 20ms)
	{rva_idensity,	0.0, 2.0},	// 0-2.0 density of reverbs (room shape) - controls # of parallel or series delays
	{rva_idecay,	0.0, 2.0},	// 0-2.0 scales feedback parameters (starting at approx 0.15)
	
	// filter params for each parallel reverb (quality set to 0 for max execution speed)

	{rva_iftype,	0, FTR_MAX},			
	{rva_icutoff,	10, 22050},
	{rva_iqwidth,	100, 11025},

	{rva_ifparallel, 0,	1}		// if 1, then all filters operate in parallel with delays. otherwise filter output only
};

rva_t * RVA_Params ( prc_t *pprc )
{
	rva_t *prva;
	int i;
	float size		= pprc->prm[rva_isize];			// 0-2.0 controls scale of delay parameters
	float density	= pprc->prm[rva_idensity];		// 0-2.0 density of reverbs (room shape) - controls # of parallel delays
	float decay		= pprc->prm[rva_idecay];		// 0-1.0 controls feedback parameters

	float ftype 	= pprc->prm[rva_iftype];
	float cutoff	= pprc->prm[rva_icutoff];
	float qwidth	= pprc->prm[rva_iqwidth];

	float fparallel = pprc->prm[rva_ifparallel];

	// D array of CRVB_DLYS reverb delay sizes max sample index w[0...D] (ie: D+1 samples)
	// a array of reverb feedback parms for parallel delays 
	// b array of CRVB_P_DLYS - mix params for parallel reverbs
	// m - number of parallel delays
	
	int D[CRVA_DLYS];
	int a[CRVA_DLYS];
	int b[CRVA_DLYS];
	int m = RVA_BASEM;
	
	m = density * CRVA_DLYS / 2;

	// limit # delays 3-12

	m = clamp (m, RVA_BASEM, CRVA_DLYS);

	
	// average time sound takes to travel from most distant wall
	// (cap at 1000 ft room)

	for ( i = 0; i < m; i++ )
	{
		// delays of parallel reverb
		
		D[i] = MSEC_TO_SAMPS( rvadlys[i] * size );
		
		// feedback and gain of parallel reverb

		a[i] = (int) min (0.9 * PMAX, rvafbs[i] * (float)PMAX * decay);
		b[i] = PMAX;
	}

	// add filter

	flt_t *pflt = NULL;

	if ( cutoff )
	{

		// set up dummy lowpass filter to convert params

		prc_t prcf;

		prcf.prm[flt_iquality]	= QUA_LO;	// force filter to low quality for faster execution time
		prcf.prm[flt_icutoff]	= cutoff;
		prcf.prm[flt_iftype]	= ftype;
		prcf.prm[flt_iqwidth]	= qwidth;
	
		pflt = (flt_t *)FLT_Params ( &prcf );	
	}
	
	prva = RVA_Alloc ( D, a, b, m, pflt, fparallel );

	FLT_Free( pflt );

	return prva;
}

inline void * RVA_VParams ( void *p ) 
{
	PRC_CheckParams ( (prc_t *)p, rva_rng ); 
	return (void *) RVA_Params ((prc_t *)p); 
}

inline void RVA_Mod ( void *p, float v ) { return; }



////////////
// Diffusor
///////////

// (N series allpass reverbs)

#define CDFRS				64				// max number of series reverbs active

#define CDFR_DLYS			16				// max number of delays making up diffusor

struct dfr_t
{
	bool fused;
	int n;						// series allpass delays
	int w[CDFR_DLYS];			// internal state array for series allpass filters

	dly_t *pdlys[CDFR_DLYS];	// array of pointers to delays
};

dfr_t dfrs[CDFRS];

void DFR_Init ( dfr_t *pdfr ) {	if ( pdfr )	Q_memset (pdfr, 0, sizeof (dfr_t)); }
void DFR_InitAll( void ) { for (int i = 0; i < CDFRS; i++) DFR_Init ( &dfrs[i] ); }

// free parallel series reverb

void DFR_Free( dfr_t *pdfr )
{
	if ( pdfr )
	{
	// free all delays

	for (int i = 0; i < CDFR_DLYS; i++)
		DLY_Free ( pdfr->pdlys[i] );
	
	Q_memset( pdfr, 0, sizeof (dfr_t) );
	}
}


void DFR_FreeAll( void ) { for (int i = 0; i < CDFRS; i++) DFR_Free( &dfrs[i] ); }

// create n series allpass reverbs

// D array of CRVB_DLYS reverb delay sizes max sample index w[0...D] (ie: D+1 samples)
// a array of reverb feedback parms for series delays
// b array of gain params for parallel reverbs
// n - number of series delays

dfr_t * DFR_Alloc ( int *D, int *a, int *b, int n )
{
	
	int i;
	dfr_t *pdfr;

	// find open slot

	for (i = 0; i < CDFRS; i++)
	{
		if (!dfrs[i].fused)
			break;
	}
	
	// return null if no free slots

	if (i == CDFRS)
	{
		DevMsg ("DSP: Warning, failed to allocate diffusor.\n" );
		return NULL;
	}
	
	pdfr = &dfrs[i];

	DFR_Init( pdfr );

	// alloc reverbs

	for (i = 0; i < n; i++)
		pdfr->pdlys[i] = DLY_Alloc( D[i], a[i], b[i], DLY_ALLPASS );
		
	// if we failed to alloc any reverb, free all, return NULL

	for (i = 0; i < n; i++)
	{
		if ( !pdfr->pdlys[i])
		{
			DFR_Free( pdfr );
			DevMsg ("DSP: Warning, failed to allocate delay for diffusor.\n" );
			return NULL;
		}
	}
	
	pdfr->fused = true;
	pdfr->n = n;

	return pdfr;
}


// series reverberator

inline int DFR_GetNext( dfr_t *pdfr, int x )
{
	int i;
	int n = pdfr->n;			
	int y;

	y = x;
	for (i = 0; i < n; i++)
		y = DLY_GetNext(pdfr->pdlys[i], y);
	return y;

#if 0
	// alternate method, using internal state - causes PREDELAY = sum of delay times

	int *v = pdfr->w;			// intermediate results

	v[0] = x;

	// reverse evaluate series delays

	//    w[0]   w[1]   w[2]      w[n-1]      w[n]
	// x---->D[0]--->D[1]--->D[2]...-->D[n-1]--->out
	//

	for (i = n; i > 0; i--)
		v[i] = DLY_GetNext(pdfr->pdlys[i-1], v[i-1]);
	
	return v[n];
#endif 
}

// batch version for performance

inline void DFR_GetNextN( dfr_t *pdfr, portable_samplepair_t *pbuffer, int SampleCount, int op )
{
	int count = SampleCount;
	portable_samplepair_t *pb = pbuffer;
	
	switch (op)
	{
	default:
	case OP_LEFT:
		while (count--)
		{
			pb->left = DFR_GetNext( pdfr, pb->left );
			pb++;
		}
		return;
	case OP_RIGHT:
		while (count--)
		{
			pb->right = DFR_GetNext( pdfr, pb->right );
			pb++;
		}
		return;
	case OP_LEFT_DUPLICATE:
		while (count--)
		{
			pb->left = pb->right = DFR_GetNext( pdfr, pb->left );
			pb++;
		}
		return;
	}
}

#define DFR_BASEN		2				// base number of series allpass delays

// nominal diffusor delay and feedback values

//float dfrdlys[] = {20,   25,   30,   35,   40,   45,   50,   55,   60,   65,   70,   75,   80,   85,   90,   95};
float dfrdlys[] =   {13,   19,   26,   21,   32,   36,   38,   16,   24,   28,   41,   35,   10,   46,   50,   27};
float dfrfbs[] =  {0.15, 0.15, 0.15, 0.15, 0.15, 0.15, 0.15, 0.15, 0.15, 0.15, 0.15, 0.15, 0.15, 0.15, 0.15, 0.15};


// diffusor parameter order
	
typedef enum
{

// parameter order

	dfr_isize,		
	dfr_idensity,	
	dfr_idecay,		
		
	dfr_cparam				// # of params

} dfr_e;

// diffusor parameter ranges

prm_rng_t dfr_rng[] = {

	{dfr_cparam,	0, 0},			// first entry is # of parameters

	{dfr_isize,		0.0, 1.0},	// 0-1.0 scales all delays
	{dfr_idensity,	0.0, 1.0},	// 0-1.0 controls # of series delays
	{dfr_idecay,	0.0, 1.0},	// 0-1.0 scales all feedback parameters 
};


dfr_t * DFR_Params ( prc_t *pprc )
{
	dfr_t *pdfr;
	int i;
	int s;
	float size =		pprc->prm[dfr_isize];			// 0-1.0 scales all delays
	float density =		pprc->prm[dfr_idensity];		// 0-1.0 controls # of series delays
	float diffusion =	pprc->prm[dfr_idecay];		// 0-1.0 scales all feedback parameters 

	// D array of CRVB_DLYS reverb delay sizes max sample index w[0...D] (ie: D+1 samples)
	// a array of reverb feedback parms for series delays (CRVB_S_DLYS)
	// b gain of each reverb section
	// n - number of series delays

	int D[CDFR_DLYS];
	int a[CDFR_DLYS];
	int b[CDFR_DLYS];
	int n = DFR_BASEN;

	// increase # of series diffusors with increased density

	n += density * 2;
	
	// limit m, n to half max number of delays

	n = min (CDFR_DLYS/2, n);

	// compute delays for diffusors

	for (i = 0; i < n; i++)
	{
		s = (int)( dfrdlys[i] * size );

		// delay of diffusor

		D[i] = MSEC_TO_SAMPS(s);

		// feedback and gain of diffusor

		a[i] = min (0.9 * PMAX, dfrfbs[i] * PMAX * diffusion);
		b[i] = PMAX;
	}

	
	pdfr = DFR_Alloc ( D, a, b, n );

	return pdfr;
}

inline void * DFR_VParams ( void *p ) 
{
	PRC_CheckParams ((prc_t *)p, dfr_rng);
	return (void *) DFR_Params ((prc_t *)p); 
}

inline void DFR_Mod ( void *p, float v ) { return; }


//////////////////////
// LFO wav definitions
//////////////////////

#define CLFOSAMPS		512					// samples per wav table - single cycle only
#define LFOBITS			14					// bits of peak amplitude of lfo wav
#define LFOAMP			((1<<LFOBITS)-1)	// peak amplitude of lfo wav

//types of lfo wavs

#define LFO_SIN			0	// sine wav
#define LFO_TRI			1	// triangle wav
#define LFO_SQR			2	// square wave, 50% duty cycle
#define LFO_SAW			3	// forward saw wav
#define LFO_RND			4	// random wav
#define LFO_LOG_IN		5	// logarithmic fade in
#define LFO_LOG_OUT		6	// logarithmic fade out
#define LFO_LIN_IN		7	// linear fade in 
#define LFO_LIN_OUT		8	// linear fade out
#define LFO_MAX			LFO_LIN_OUT

#define CLFOWAV	9			// number of LFO wav tables

struct lfowav_t		// lfo or envelope wave table
{
	int	type;				// lfo type
	dly_t *pdly;			// delay holds wav values and step pointers
};

lfowav_t lfowavs[CLFOWAV];

// deallocate lfo wave table. Called only when sound engine exits.

void LFOWAV_Free( lfowav_t *plw )
{
	// free delay

	if ( plw )
		DLY_Free( plw->pdly );

	Q_memset( plw, 0, sizeof (lfowav_t) );
}

// deallocate all lfo wave tables. Called only when sound engine exits.

void LFOWAV_FreeAll( void )
{
	for ( int i = 0; i < CLFOWAV; i++ )
		LFOWAV_Free( &lfowavs[i] );
}

// fill lfo array w with count samples of lfo type 'type'
// all lfo wavs except fade out, rnd, and log_out should start with 0 output

void LFOWAV_Fill( int *w, int count, int type )
{
	int i,x;
	switch (type)
	{
	default:
	case LFO_SIN:			// sine wav, all values 0 <= x <= LFOAMP, initial value = 0
			for (i = 0; i < count; i++ )
			{
				x = ( int )( (float)(LFOAMP) * sinf( (2.0 * M_PI_F * (float)i / (float)count ) + (M_PI_F * 1.5) ) );
				w[i] = (x + LFOAMP)/2;
			}
			break;
	case LFO_TRI:			// triangle wav, all values 0 <= x <= LFOAMP, initial value = 0
			for (i = 0; i < count; i++)
				{
				w[i] = ( int ) ( (float)(2 * LFOAMP * i ) / (float)(count) );
				
				if ( i > count / 2 )
					w[i] = ( int ) ( (float) (2 * LFOAMP) - (float)( 2 * LFOAMP * i ) / (float)(count) );
				}
			break;
	case LFO_SQR:			// square wave, 50% duty cycle, all values 0 <= x <= LFOAMP, initial value = 0
			for (i = 0; i < count; i++)
				w[i] = i > count / 2 ? 0 : LFOAMP;
			break;
	case LFO_SAW:			// forward saw wav, aall values 0 <= x <= LFOAMP, initial value = 0
			for (i = 0; i < count; i++)
				w[i] = ( int ) ( (float)(LFOAMP) * (float)i / (float)(count) );
			break;
	case LFO_RND:			// random wav, all values 0 <= x <= LFOAMP
			for (i = 0; i < count; i++)
				w[i] = ( int ) ( g_pSoundServices->RandomLong(0, LFOAMP) );
			break;
	case LFO_LOG_IN:		// logarithmic fade in, all values 0 <= x <= LFOAMP, initial value = 0
			for (i = 0; i < count; i++)
				w[i] = ( int ) ( (float)(LFOAMP) * powf( (float)i / (float)count, 2));
			break;
	case LFO_LOG_OUT:		// logarithmic fade out, all values 0 <= x <= LFOAMP, initial value = LFOAMP
			for (i = 0; i < count; i++)
				w[i] = ( int ) ( (float)(LFOAMP) * powf( 1.0 - ((float)i / (float)count), 2 ));
			break;
	case LFO_LIN_IN:		// linear fade in, all values 0 <= x <= LFOAMP, initial value = 0
			for (i = 0; i < count; i++)
				w[i] = ( int ) ( (float)(LFOAMP) * (float)i / (float)(count) );
			break;
	case LFO_LIN_OUT:		// linear fade out, all values 0 <= x <= LFOAMP, initial value = LFOAMP
			for (i = 0; i < count; i++)
				w[i] = LFOAMP - ( int ) ( (float)(LFOAMP) * (float)i / (float)(count) );
			break;
	}
}

// allocate all lfo wave tables.  Called only when sound engine loads.

void LFOWAV_InitAll()
{
	int i;
	dly_t *pdly;

	Q_memset( lfowavs, 0, sizeof( lfowavs ) );

	// alloc space for each lfo wav type
	
	for (i = 0; i < CLFOWAV; i++)
	{
		pdly = DLY_Alloc( CLFOSAMPS, 0, 0 , DLY_PLAIN);
		
		lfowavs[i].pdly = pdly;
		lfowavs[i].type = i;

		LFOWAV_Fill( pdly->w, CLFOSAMPS, i );
	}
	
	// if any dlys fail to alloc, free all

	for (i = 0; i < CLFOWAV; i++)
	{
		if ( !lfowavs[i].pdly )
			LFOWAV_FreeAll();
	}
}


////////////////////////////////////////
// LFO iterators - one shot and looping
////////////////////////////////////////

#define CLFO	16	// max active lfos (this steals from active delays)

struct lfo_t
{
	bool fused;		// true if slot take

	dly_t *pdly;	// delay points to lfo wav within lfowav_t (don't free this)

	
	float f;		// playback frequency in hz

	pos_t pos;		// current position within wav table, looping
	pos_one_t pos1;	// current position within wav table, one shot

	int foneshot;	// true - one shot only, don't repeat
};

lfo_t lfos[CLFO];

void LFO_Init( lfo_t *plfo ) { if ( plfo ) Q_memset( plfo, 0, sizeof (lfo_t) ); }
void LFO_InitAll( void ) { for (int i = 0; i < CLFO; i++) LFO_Init(&lfos[i]); }
void LFO_Free( lfo_t *plfo ) { if ( plfo ) Q_memset( plfo, 0, sizeof (lfo_t) ); }
void LFO_FreeAll( void ) { for (int i = 0; i < CLFO; i++) LFO_Free(&lfos[i]); }


// get step value given desired playback frequency

inline float LFO_HzToStep ( float freqHz )
{
	float lfoHz;

	// calculate integer and fractional step values,
	// assume an update rate of SOUND_DMA_SPEED samples/sec

	// 1 cycle/CLFOSAMPS * SOUND_DMA_SPEED samps/sec = cycles/sec = current lfo rate
	//
	// lforate * X = freqHz  so X = freqHz/lforate = update rate

	lfoHz = (float)(SOUND_DMA_SPEED) / (float)(CLFOSAMPS);

	return freqHz / lfoHz;
}

// return pointer to new lfo

lfo_t * LFO_Alloc( int wtype, float freqHz, bool foneshot )
{
	int i;
	int type = min ( CLFOWAV - 1, wtype );
	float lfostep;
	
	for (i = 0; i < CLFO; i++)
		if (!lfos[i].fused)
		{
			lfo_t *plfo = &lfos[i];

			LFO_Init( plfo );

			plfo->fused = true;
			plfo->pdly = lfowavs[type].pdly;		// pdly in lfo points to wav table data in lfowavs
			plfo->f = freqHz;
			plfo->foneshot = foneshot;

			lfostep = LFO_HzToStep( freqHz );

			// init positional pointer (ie: fixed point updater for controlling pitch of lfo)

			if ( !foneshot )
				POS_Init(&(plfo->pos), plfo->pdly->D, lfostep );
			else
				POS_ONE_Init(&(plfo->pos1), plfo->pdly->D,lfostep );

			return plfo;
		}
		DevMsg ("DSP: Warning, failed to allocate LFO.\n" );
		return NULL;
}

// get next lfo value
// Value returned is 0..LFOAMP.  can be normalized by shifting right by LFOBITS
// To play back at correct passed in frequency, routien should be
// called once for every output sample (ie: at SOUND_DMA_SPEED)
// x is dummy param

inline int LFO_GetNext( lfo_t *plfo, int x )
{
	int i;

	// get current position

	if ( !plfo->foneshot )
		i = POS_GetNext( &plfo->pos );
	else
		i = POS_ONE_GetNext( &plfo->pos1 );

	// return current sample

	return plfo->pdly->w[i];
}

// batch version for performance

inline void LFO_GetNextN( lfo_t *plfo, portable_samplepair_t *pbuffer, int SampleCount, int op )
{
	int count = SampleCount;
	portable_samplepair_t *pb = pbuffer;
	
	switch (op)
	{
	default:
	case OP_LEFT:
		while (count--)
		{
			pb->left = LFO_GetNext( plfo, pb->left );
			pb++;
		}
		return;
	case OP_RIGHT:
		while (count--)
		{
			pb->right = LFO_GetNext( plfo, pb->right );
			pb++;
		}
		return;
	case OP_LEFT_DUPLICATE:
		while (count--)
		{
			pb->left = pb->right = LFO_GetNext( plfo, pb->left );
			pb++;
		}
		return;
	}
}

// uses lfowav, rate, foneshot
	
typedef enum
{

// parameter order

	lfo_iwav,		
	lfo_irate,		
	lfo_ifoneshot,	
		
	lfo_cparam			// # of params

} lfo_e;

// parameter ranges

prm_rng_t lfo_rng[] = {

	{lfo_cparam,	0, 0},			// first entry is # of parameters

	{lfo_iwav,		0.0, LFO_MAX},	// lfo type to use (LFO_SIN, LFO_RND...)
	{lfo_irate,		0.0, 16000.0},	// modulation rate in hz. for MDY, 1/rate = 'glide' time in seconds
	{lfo_ifoneshot,	0.0, 1.0},		// 1.0 if lfo is oneshot
};


lfo_t * LFO_Params ( prc_t *pprc )
{
	lfo_t *plfo;
	bool foneshot = pprc->prm[lfo_ifoneshot] > 0 ? true : false;

	plfo = LFO_Alloc ( pprc->prm[lfo_iwav], pprc->prm[lfo_irate], foneshot );

	return plfo;
}

void LFO_ChangeVal ( lfo_t *plfo, float fhz )
{
	float fstep = LFO_HzToStep( fhz );

	// change lfo playback rate to new frequency fhz

	if ( plfo->foneshot )
		POS_ChangeVal( &plfo->pos, fstep );
	else
		POS_ChangeVal( &plfo->pos1.p, fstep );
}

inline void * LFO_VParams ( void *p ) 
{
	PRC_CheckParams ( (prc_t *)p, lfo_rng ); 
	return (void *) LFO_Params ((prc_t *)p); 
}

// v is +/- 0-1.0
// v changes current lfo frequency up/down by +/- v%

inline void LFO_Mod ( lfo_t *plfo, float v ) 
{ 
	float fhz;
	float fhznew;

	fhz = plfo->f;
	fhznew = fhz * (1.0 + v);

	LFO_ChangeVal ( plfo, fhznew );

	return; 
}


/////////////////////////////////////////////////////////////////////////////
// Ramp - used for varying smoothly between int parameters ie: modulation delays
/////////////////////////////////////////////////////////////////////////////


struct rmp_t
{
	int initval;					// initial ramp value
	int target;						// final ramp value
	int sign;						// increasing (1) or decreasing (-1) ramp
	
	int yprev;						// previous output value
	bool fhitend;					// true if hit end of ramp

	pos_one_t ps;					// current ramp output
};

// ramp smoothly between initial value and target value in approx 'ramptime' seconds.
// (initial value may be greater or less than target value)
// never changes output by more than +1 or -1 (which can cause the ramp to take longer to complete than ramptime)
// called once per sample while ramping
// ramptime - duration of ramp in seconds
// initval - initial ramp value
// targetval - target ramp value

void RMP_Init( rmp_t *prmp, float ramptime, int initval, int targetval ) 
{
	int rise;
	int run;

	if (prmp)
		Q_memset( prmp, 0, sizeof (rmp_t) ); 

			
	run = (int) (ramptime * SOUND_DMA_SPEED);		// 'samples' in ramp
	rise = (targetval - initval);					// height of ramp

	// init fixed point iterator to iterate along the height of the ramp 'rise'
	// always iterates from 0..'rise', increasing in value

	POS_ONE_Init( &prmp->ps, ABS( rise ), ABS((float) rise) / ((float) run) );
	
	prmp->yprev = initval;
	prmp->initval = initval;
	prmp->target = targetval;
	prmp->sign = SIGN( rise );

}

// continues from current position to new target position

void RMP_SetNext( rmp_t *prmp, float ramptime, int targetval )
{
	RMP_Init ( prmp, ramptime, prmp->yprev, targetval );
}

inline bool RMP_HitEnd ( rmp_t *prmp )
{
	return prmp->fhitend;
}

inline void RMP_SetEnd ( rmp_t *prmp )
{
	prmp->fhitend = true;
}

// get next ramp value & update ramp, never varies by more than +1 or -1 between calls
// when ramp hits target value, it thereafter always returns last value

inline int RMP_GetNext( rmp_t *prmp ) 
{
	int y;
	int d;
	
	// if we hit ramp end, return last value

	if (prmp->fhitend)
		return prmp->yprev;
	
	// get next integer position in ramp height. 

	d = POS_ONE_GetNext( &prmp->ps );
	
	if ( prmp->ps.fhitend )
		prmp->fhitend = true;

	// increase or decrease from initval, depending on ramp sign

	if ( prmp->sign > 0 )
		y = prmp->initval + d;
	else
		y = prmp->initval - d;

	// only update current height by a max of +1 or -1
	// this means that for short ramp times, we may not hit target

	if ( ABS( y - prmp->yprev ) >= 1 )
		prmp->yprev += prmp->sign;

	return prmp->yprev;
}

// get current ramp value, don't update ramp

inline int RMP_GetCurrent( rmp_t *prmp )
{
	return prmp->yprev;
}

////////////////////////////////////////
// Time Compress/expand with pitch shift
////////////////////////////////////////

// realtime pitch shift - ie: pitch shift without change to playback rate

#define CPTCS		64

struct ptc_t
{
	bool fused;
	
	dly_t *pdly_in;			// input buffer space
	dly_t *pdly_out;		// output buffer space

	int *pin;				// input buffer (pdly_in->w)
	int *pout;				// output buffer (pdly_out->w)

	int cin;				// # samples in input buffer
	int cout;				// # samples in output buffer

	int cxfade;				// # samples in crossfade segment
	int ccut;				// # samples to cut
	int cduplicate;			// # samples to duplicate (redundant - same as ccut)

	int iin;				// current index into input buffer (reading)
	
	pos_one_t psn;			// stepping index through output buffer

	bool fdup;				// true if duplicating, false if cutting

	float fstep;			// pitch shift & time compress/expand
};

ptc_t ptcs[CPTCS];

void PTC_Init( ptc_t *pptc ) { if (pptc) Q_memset( pptc, 0, sizeof (ptc_t) ); };
void PTC_Free( ptc_t *pptc ) 
{
	if (pptc)
	{
		DLY_Free (pptc->pdly_in);
		DLY_Free (pptc->pdly_out);

		Q_memset( pptc, 0, sizeof (ptc_t) ); 
	}
};
void PTC_InitAll() { for (int i = 0; i < CPTCS; i++) PTC_Init( &ptcs[i] ); };
void PTC_FreeAll() { for (int i = 0; i < CPTCS; i++) PTC_Free( &ptcs[i] ); };



// Time compressor/expander with pitch shift (ie: pitch changes, playback rate does not)
//
// Algorithm:

// 1) Duplicate or discard chunks of sound to provide tslice * fstep seconds of sound.
//    (The user-selectable size of the buffer to process is tslice milliseconds in length)
// 2) Resample this compressed/expanded buffer at fstep to produce a pitch shifted
//    output with the same duration as the input (ie: #samples out = # samples in, an
//    obvious requirement for realtime inline processing).

// timeslice is size in milliseconds of full buffer to process.
// timeslice * fstep is the size of the expanded/compressed buffer
// timexfade is length in milliseconds of crossfade region between duplicated or cut sections
// fstep is % expanded/compressed sound normalized to 0.01-2.0 (1% - 200%)

// input buffer: 

// iin-->

// [0...      tslice              ...D]						input samples 0...D (D is NEWEST sample)
// [0...          ...n][m... tseg ...D]						region to be cut or duplicated m...D
					
// [0...   [p..txf1..n][m... tseg ...D]						fade in  region 1 txf1 p...n
// [0...          ...n][m..[q..txf2..D]						fade out region 2 txf2 q...D


// pitch up: duplicate into output buffer:	tdup = tseg

// [0...          ...n][m... tdup ...D][m... tdup ...D]		output buffer size with duplicate region	
// [0...          ...n][m..[p...xf1..n][m... tdup ...D]		fade in p...n while fading out q...D
// [0...          ...n][m..[q...xf2..D][m... tdup ...D]		
// [0...          ...n][m..[.XFADE...n][m... tdup ...D]		final duplicated output buffer - resample at fstep

// pitch down: cut into output buffer: tcut = tseg

// [0...         ...n][m... tcut  ...D]				input samples with cut region delineated m...D
// [0...         ...n]								output buffer size after cut
// [0... [q..txf2...D]								fade in txf1 q...D while fade out txf2 p...n
// [0... [.XFADE ...D]								final cut output buffer - resample at fstep


ptc_t * PTC_Alloc( float timeslice, float timexfade, float fstep ) 
{
	
	int i;
	ptc_t *pptc;
	float tout;
	int cin, cout;
	float tslice = timeslice;
	float txfade = timexfade;
	float tcutdup;

	// find time compressor slot

	for ( i = 0; i < CPTCS; i++ )
	{
		if ( !ptcs[i].fused )
			break;
	}
	
	if ( i == CPTCS ) 
	{
		DevMsg ("DSP: Warning, failed to allocate pitch shifter.\n" );
		return NULL;
	}

	pptc = &ptcs[i];
	
	PTC_Init ( pptc );

	// get size of region to cut or duplicate

	tcutdup = abs((fstep - 1.0) * timeslice);

	// to prevent buffer overruns:

	// make sure timeslice is greater than cut/dup time
	
	tslice = max ( tslice, 1.1 * tcutdup);

	// make sure xfade time smaller than cut/dup time, and smaller than (timeslice-cutdup) time

	txfade = min ( txfade, 0.9 * tcutdup );
	txfade = min ( txfade, 0.9 * (tslice - tcutdup));

	pptc->cxfade =		MSEC_TO_SAMPS( txfade );
	pptc->ccut =		MSEC_TO_SAMPS( tcutdup );
	pptc->cduplicate =  MSEC_TO_SAMPS( tcutdup );
	
	// alloc delay lines (buffers)

	tout = tslice * fstep;

	cin = MSEC_TO_SAMPS( tslice );
	cout = MSEC_TO_SAMPS( tout );
	
	pptc->pdly_in = DLY_Alloc( cin, 0, 1, DLY_LINEAR );			// alloc input buffer
	pptc->pdly_out = DLY_Alloc( cout, 0, 1, DLY_LINEAR);		// alloc output buffer
	
	if ( !pptc->pdly_in || !pptc->pdly_out )
	{
		PTC_Free( pptc );
		DevMsg ("DSP: Warning, failed to allocate delay for pitch shifter.\n" );
		return NULL;
	}

	// buffer pointers

	pptc->pin = pptc->pdly_in->w;
	pptc->pout = pptc->pdly_out->w;

	// input buffer index

	pptc->iin = 0;

	// output buffer index

	POS_ONE_Init ( &pptc->psn, cout, fstep );

	// if fstep > 1.0 we're pitching shifting up, so fdup = true

	pptc->fdup = fstep > 1.0 ? true : false;
	
	pptc->cin = cin;
	pptc->cout = cout;

	pptc->fstep = fstep;
	pptc->fused = true;

	return pptc;
}

// linear crossfader
// yfadein - instantaneous value fading in
// ydafeout -instantaneous value fading out
// nsamples - duration in #samples of fade
// isample - index in to fade 0...nsamples-1

inline int xfade ( int yfadein, int yfadeout, int nsamples, int isample )
{
	int yout;
	int m = (isample << PBITS ) / nsamples;

	yout = ((yfadein * m) >> PBITS) + ((yfadeout * (PMAX - m)) >> PBITS);

	return yout;
}

// w - pointer to start of input buffer samples
// v - pointer to start of output buffer samples
// cin - # of input buffer samples
// cout = # of output buffer samples
// cxfade = # of crossfade samples
// cduplicate = # of samples in duplicate/cut segment

void TimeExpand( int *w, int *v, int cin, int cout, int cxfade, int cduplicate )
{
	int i,j;
	int m;	
	int p;
	int q;
	int D;

	// input buffer
	//               xfade source   duplicate
	// [0...........][p.......n][m...........D]
	
	// output buffer
	//								 xfade region   duplicate
	// [0.....................n][m..[q.......D][m...........D]

	// D - index of last sample in input buffer
	// m - index of 1st sample in duplication region
	// p - index of 1st sample of crossfade source
	// q - index of 1st sample in crossfade region
	
	D = cin - 1;
	m = cin - cduplicate;			
	p = m - cxfade;	
	q = cin - cxfade;

	// copy up to crossfade region

	for (i = 0; i < q; i++)
		v[i] = w[i];
	
	// crossfade region

	j = p;	

	for (i = q; i <= D; i++)
		v[i] = xfade (w[j++], w[i], cxfade, i-q);	// fade out p..n, fade in q..D
	
	// duplicate region

	j = D+1;

	for (i = m; i <= D; i++)
		v[j++] = w[i];

}

// cut ccut samples from end of input buffer, crossfade end of cut section
// with end of remaining section

// w - pointer to start of input buffer samples
// v - pointer to start of output buffer samples
// cin - # of input buffer samples
// cout = # of output buffer samples
// cxfade = # of crossfade samples
// ccut = # of samples in cut segment

void TimeCompress( int *w, int *v, int cin, int cout, int cxfade, int ccut )
{
	int i,j;
	int m;	
	int p;
	int q;
	int D;

	// input buffer
	//								  xfade source 
	// [0.....................n][m..[p.......D]

	//              xfade region     cut
	// [0...........][q.......n][m...........D]
	
	// output buffer
	//               xfade to source 
	// [0...........][p.......D]
	
	// D - index of last sample in input buffer
	// m - index of 1st sample in cut region
	// p - index of 1st sample of crossfade source
	// q - index of 1st sample in crossfade region
	
	D = cin - 1;
	m = cin - ccut;			
	p = cin - cxfade;	
	q = m - cxfade;

	// copy up to crossfade region

	for (i = 0; i < q; i++)
		v[i] = w[i];
	
	// crossfade region

	j = p;	

	for (i = q; i < m; i++)
		v[i] = xfade (w[j++], w[i], cxfade, i-q);	// fade out p..n, fade in q..D
	
	// skip rest of input buffer
}

// get next sample

// put input sample into input (delay) buffer
// get output sample from output buffer, step by fstep %
// output buffer is time expanded or compressed version of previous input buffer

inline int PTC_GetNext( ptc_t *pptc, int x ) 
{
	int iout, xout;
	bool fhitend = false;

	// write x into input buffer
	Assert (pptc->iin < pptc->cin);

	pptc->pin[pptc->iin] = x;

	pptc->iin++;
	
	// check for end of input buffer

	if ( pptc->iin >= pptc->cin )
		fhitend = true;

	// read sample from output buffer, resampling at fstep

	iout = POS_ONE_GetNext( &pptc->psn );
	Assert (iout < pptc->cout);
	xout = pptc->pout[iout];
	
	if ( fhitend )
	{
		// if hit end of input buffer (ie: input buffer is full)
		//		reset input buffer pointer
		//		reset output buffer pointer
		//		rebuild entire output buffer (TimeCompress/TimeExpand)

		pptc->iin = 0;

		POS_ONE_Init( &pptc->psn, pptc->cout, pptc->fstep );

		if ( pptc->fdup )
			TimeExpand ( pptc->pin, pptc->pout, pptc->cin, pptc->cout, pptc->cxfade, pptc->cduplicate );
		else
			TimeCompress ( pptc->pin, pptc->pout, pptc->cin, pptc->cout, pptc->cxfade, pptc->ccut );
	}

	return xout;
}

// batch version for performance

inline void PTC_GetNextN( ptc_t *pptc, portable_samplepair_t *pbuffer, int SampleCount, int op )
{
	int count = SampleCount;
	portable_samplepair_t *pb = pbuffer;
	
	switch (op)
	{
	default:
	case OP_LEFT:
		while (count--)
		{
			pb->left = PTC_GetNext( pptc, pb->left );
			pb++;
		}
		return;
	case OP_RIGHT:
		while (count--)
		{
			pb->right = PTC_GetNext( pptc, pb->right );
			pb++;
		}
		return;
	case OP_LEFT_DUPLICATE:
		while (count--)
		{
			pb->left = pb->right = PTC_GetNext( pptc, pb->left );
			pb++;
		}
		return;
	}
}

// change time compression to new value
// fstep is new value
// ramptime is how long change takes in seconds (ramps smoothly), 0 for no ramp

void PTC_ChangeVal( ptc_t *pptc, float fstep, float ramptime )
{
// UNDONE: ignored
// UNDONE: just realloc time compressor with new fstep
}

// uses pitch: 
// 1.0 = playback normal rate
// 0.5 = cut 50% of sound (2x playback)
// 1.5 = add 50% sound (0.5x playback)

typedef enum
{

// parameter order

	ptc_ipitch,			
	ptc_itimeslice,		
	ptc_ixfade,			
		
	ptc_cparam			// # of params

} ptc_e;

// diffusor parameter ranges

prm_rng_t ptc_rng[] = {

	{ptc_cparam,	0, 0},				// first entry is # of parameters

	{ptc_ipitch,		0.1, 4.0},		// 0-n.0 where 1.0 = 1 octave up and 0.5 is one octave down	
	{ptc_itimeslice,	20.0, 300.0},	// in milliseconds - size of sound chunk to analyze and cut/duplicate - 100ms nominal
	{ptc_ixfade,		1.0, 200.0},	// in milliseconds - size of crossfade region between spliced chunks - 20ms nominal	
};


ptc_t * PTC_Params ( prc_t *pprc )
{
	ptc_t *pptc;

	float pitch = pprc->prm[ptc_ipitch];
	float timeslice = pprc->prm[ptc_itimeslice];
	float txfade = pprc->prm[ptc_ixfade];

	pptc = PTC_Alloc( timeslice, txfade, pitch );
	
	return pptc;
}

inline void * PTC_VParams ( void *p ) 
{
	PRC_CheckParams ( (prc_t *)p, ptc_rng ); 
	return (void *) PTC_Params ((prc_t *)p); 
}

// change to new pitch value
// v is +/- 0-1.0
// v changes current pitch up/down by +/- v%

void PTC_Mod ( ptc_t *pptc, float v ) 
{ 
	float fstep;
	float fstepnew;

	fstep = pptc->fstep;
	fstepnew = fstep * (1.0 + v);

	PTC_ChangeVal( pptc, fstepnew, 0.01 );
}


////////////////////
// ADSR envelope
////////////////////

#define CENVS		64		// max # of envelopes active
#define CENVRMPS	4		// A, D, S, R

#define ENV_LIN		0		// linear a,d,s,r
#define ENV_EXP		1		// exponential a,d,s,r
#define ENV_MAX		ENV_EXP	

#define ENV_BITS	14		// bits of resolution of ramp

struct env_t
{
	bool fused;

	bool fhitend;			// true if done

	int ienv;				// current ramp
	rmp_t rmps[CENVRMPS];	// ramps
};

env_t envs[CENVS];

void ENV_Init( env_t *penv ) { if (penv) Q_memset( penv, 0, sizeof (env_t) ); };
void ENV_Free( env_t *penv ) { if (penv) Q_memset( penv, 0, sizeof (env_t) ); };
void ENV_InitAll() { for (int i = 0; i < CENVS; i++) ENV_Init( &envs[i] ); };
void ENV_FreeAll() { for (int i = 0; i < CENVS; i++) ENV_Free( &envs[i] ); };


// allocate ADSR envelope
// all times are in seconds
// amp1 - attack amplitude multiplier 0-1.0
// amp2 - sustain amplitude multiplier 0-1.0
// amp3 - end of sustain amplitude multiplier 0-1.0

env_t *ENV_Alloc ( int type, float famp1, float famp2, float famp3, float attack, float decay, float sustain, float release )
{
	int i;
	env_t *penv;

	for (i = 0; i < CENVS; i++)
	{
		if ( !envs[i].fused )
		{

			int amp1 = famp1 * (1 << ENV_BITS);	// ramp resolution
			int amp2 = famp2 * (1 << ENV_BITS);	
			int amp3 = famp3 * (1 << ENV_BITS);

			penv = &envs[i];
			
			ENV_Init (penv);

			// UNDONE: ignoring type = ENV_EXP - use oneshot LFOS instead with sawtooth/exponential

			// set up ramps

			RMP_Init( &penv->rmps[0], attack, 0, amp1 );
			RMP_Init( &penv->rmps[1], decay, amp1, amp2 );
			RMP_Init( &penv->rmps[2], sustain, amp2, amp3 );
			RMP_Init( &penv->rmps[3], release, amp3, 0 );

			penv->ienv = 0;
			penv->fused = true;
			penv->fhitend = false;

			return penv;
		}
	}
	DevMsg ("DSP: Warning, failed to allocate envelope.\n" );
	return NULL;
}


inline int ENV_GetNext( env_t *penv, int x )
{
	if ( !penv->fhitend )
	{
		int i;
		int y;

		i = penv->ienv;
		y = RMP_GetNext ( &penv->rmps[i] );
		
		// check for next ramp

		if ( penv->rmps[i].fhitend )
			i++;

		penv->ienv = i;

		// check for end of all ramps

		if ( i > 3)
			penv->fhitend = true;

		// multiply input signal by ramp

		return (x * y) >> ENV_BITS;
	}

	return 0;
}

// batch version for performance

inline void ENV_GetNextN( env_t *penv, portable_samplepair_t *pbuffer, int SampleCount, int op )
{
	int count = SampleCount;
	portable_samplepair_t *pb = pbuffer;
	
	switch (op)
	{
	default:
	case OP_LEFT:
		while (count--)
		{
			pb->left = ENV_GetNext( penv, pb->left );
			pb++;
		}
		return;
	case OP_RIGHT:
		while (count--)
		{
			pb->right = ENV_GetNext( penv, pb->right );
			pb++;
		}
		return;
	case OP_LEFT_DUPLICATE:
		while (count--)
		{
			pb->left = pb->right = ENV_GetNext( penv, pb->left );
			pb++;
		}
		return;
	}
}

// uses lfowav, amp1, amp2, amp3, attack, decay, sustain, release
// lfowav is type, currently ignored - ie: LFO_LIN_IN, LFO_LOG_IN

// parameter order

typedef enum
{
	env_itype,		
	env_iamp1,		
	env_iamp2,		
	env_iamp3,		
	env_iattack,	
	env_idecay,		
	env_isustain,		
	env_irelease,	
	
	env_cparam			// # of params

} env_e;

// parameter ranges

prm_rng_t env_rng[] = {

	{env_cparam,	0, 0},			// first entry is # of parameters

	{env_itype,		0.0,ENV_MAX},	// ENV_LINEAR, ENV_LOG - currently ignored
	{env_iamp1,		0.0, 1.0},		// attack peak amplitude 0-1.0
	{env_iamp2,		0.0, 1.0},		// decay target amplitued 0-1.0
	{env_iamp3,		0.0, 1.0},		// sustain target amplitude 0-1.0
	{env_iattack,	0.0, 20000.0},	// attack time in milliseconds
	{env_idecay,	0.0, 20000.0},	// envelope decay time in milliseconds
	{env_isustain,	0.0, 20000.0},	// sustain time in milliseconds	
	{env_irelease,	0.0, 20000.0},	// release time in milliseconds	
};

env_t * ENV_Params ( prc_t *pprc )
{
	env_t *penv;

	float type		= pprc->prm[env_itype];
	float amp1		= pprc->prm[env_iamp1];
	float amp2		= pprc->prm[env_iamp2];
	float amp3		= pprc->prm[env_iamp3];
	float attack	= pprc->prm[env_iattack]/1000.0;
	float decay		= pprc->prm[env_idecay]/1000.0;
	float sustain	= pprc->prm[env_isustain]/1000.0;
	float release	= pprc->prm[env_irelease]/1000.0;

	penv = ENV_Alloc ( type, amp1, amp2, amp3, attack, decay, sustain, release );
	return penv;
}

inline void * ENV_VParams ( void *p ) 
{
	PRC_CheckParams( (prc_t *)p, env_rng ); 
	return (void *) ENV_Params ((prc_t *)p); 
}

inline void ENV_Mod ( void *p, float v ) { return; }

////////////////////
// envelope follower
////////////////////

#define CEFOS		64		// max # of envelope followers active

#define CEFOBITS	6					// size 2^6 = 64
#define CEFOWINDOW	(1 << (CEFOBITS))	// size of sample window

struct efo_t
{
	bool fused;

	int avg;				// accumulating average over sample window
	int cavg;				// count down

	int xout;				// current output value

};

efo_t efos[CEFOS];

void EFO_Init( efo_t *pefo ) { if (pefo) Q_memset( pefo, 0, sizeof (efo_t) ); };
void EFO_Free( efo_t *pefo ) { if (pefo) Q_memset( pefo, 0, sizeof (efo_t) ); };
void EFO_InitAll() { for (int i = 0; i < CEFOS; i++) EFO_Init( &efos[i] ); };
void EFO_FreeAll() { for (int i = 0; i < CEFOS; i++) EFO_Free( &efos[i] ); };


// allocate enveloper follower

efo_t *EFO_Alloc ( void )
{
	int i;
	efo_t *pefo;

	for (i = 0; i < CEFOS; i++)
	{
		if ( !efos[i].fused )
		{
			pefo = &efos[i];

			EFO_Init ( pefo );

			pefo->xout = 0;
			pefo->cavg = CEFOWINDOW;
			pefo->fused = true;

			return pefo;
		}
	}

	DevMsg ("DSP: Warning, failed to allocate envelope follower.\n" );
	return NULL;
}


inline int EFO_GetNext( efo_t *pefo, int x )
{
	int xa = ABS( x );			// rectify input wav

	// get running sum / 2

	pefo->avg += xa >> 1;		// divide by 2 to prevent overflow

	pefo->cavg--;
	
	if ( !pefo->cavg )
	{
		// new output value - end of window

		// get average over window

		pefo->xout = pefo->avg >> (CEFOBITS - 1);	// divide by window size / 2
		pefo->cavg = CEFOWINDOW;
		pefo->avg = 0;
	}

	return pefo->xout;
}

// batch version for performance

inline void EFO_GetNextN( efo_t *pefo, portable_samplepair_t *pbuffer, int SampleCount, int op )
{
	int count = SampleCount;
	portable_samplepair_t *pb = pbuffer;
	
	switch (op)
	{
	default:
	case OP_LEFT:
		while (count--)
		{
			pb->left = EFO_GetNext( pefo, pb->left );
			pb++;
		}
		return;
	case OP_RIGHT:
		while (count--)
		{
			pb->right = EFO_GetNext( pefo, pb->right );
			pb++;
		}
		return;
	case OP_LEFT_DUPLICATE:
		while (count--)
		{
			pb->left = pb->right = EFO_GetNext( pefo, pb->left );
			pb++;
		}
		return;
	}
}


efo_t * EFO_Params ( prc_t *pprc )
{
	return EFO_Alloc();
}

inline void * EFO_VParams ( void *p ) 
{
	// PRC_CheckParams ( (prc_t *)p, efo_rng );  - efo has no params
	return (void *) EFO_Params ((prc_t *)p); 
}

inline void EFO_Mod ( void *p, float v ) { return; }

//////////////
// mod delay
//////////////

// modulate delay time anywhere from 0..D using MDY_ChangeVal. no output glitches (uses RMP)

#define CMDYS				64				// max # of mod delays active (steals from delays)

struct mdy_t
{
	bool fused;

	bool fchanging;			// true if modulating to new delay value

	dly_t *pdly;			// delay
	
	int Dcur;				// current delay value
	float ramptime;			// ramp 'glide' time - time in seconds to change between values

	int mtime;				// time in samples between delay changes. 0 implies no self-modulating
	int mtimecur;			// current time in samples until next delay change
	float depth;			// modulate delay from D to D - (D*depth)  depth 0-1.0

	int xprev;				// previous delay output, used to smooth transitions between delays

	rmp_t rmp;				// ramp
};

mdy_t mdys[CMDYS];

void MDY_Init( mdy_t *pmdy ) { if (pmdy) Q_memset( pmdy, 0, sizeof (mdy_t) ); };
void MDY_Free( mdy_t *pmdy ) { if (pmdy) { DLY_Free (pmdy->pdly); Q_memset( pmdy, 0, sizeof (mdy_t) ); } };
void MDY_InitAll() { for (int i = 0; i < CMDYS; i++) MDY_Init( &mdys[i] ); };
void MDY_FreeAll() { for (int i = 0; i < CMDYS; i++) MDY_Free( &mdys[i] ); };


// allocate mod delay, given previously allocated dly
// ramptime is time in seconds for delay to change from dcur to dnew
// modtime is time in seconds between modulations. 0 if no self-modulation
// depth is 0-1.0 multiplier, new delay values when modulating are Dnew = randomlong (D - D*depth, D)

mdy_t *MDY_Alloc ( dly_t *pdly, float ramptime, float modtime, float depth )
{
	int i;
	mdy_t *pmdy;

	if ( !pdly )
		return NULL;

	for (i = 0; i < CMDYS; i++)
	{
		if ( !mdys[i].fused )
		{
			pmdy = &mdys[i];

			MDY_Init ( pmdy );

			pmdy->pdly = pdly;

			if ( !pmdy->pdly )
			{
				DevMsg ("DSP: Warning, failed to allocate delay for mod delay.\n" );
				return NULL;
			}

			pmdy->Dcur = pdly->D0;
			pmdy->fused = true;
			pmdy->ramptime = ramptime;
			pmdy->mtime = SEC_TO_SAMPS(modtime);
			pmdy->mtimecur = pmdy->mtime;
			pmdy->depth = depth;

			return pmdy;
		}
	}

	DevMsg ("DSP: Warning, failed to allocate mod delay.\n" );
	return NULL;
}

// change to new delay tap value t samples, ramp linearly over ramptime seconds

void MDY_ChangeVal ( mdy_t *pmdy, int t )
{
	// if D > original delay value, cap at original value

	t = min (pmdy->pdly->D0, t);
	
	pmdy->fchanging = true;
	
	RMP_Init ( &pmdy->rmp, pmdy->ramptime, pmdy->Dcur, t );
}

// get next value from modulating delay

int MDY_GetNext( mdy_t *pmdy, int x )
{

	int xout;
	int xcur;

	// get current delay output

	xcur = DLY_GetNext( pmdy->pdly, x );

	// return right away if not modulating (not changing and not self modulating)

	if ( !pmdy->fchanging && !pmdy->mtime )
	{	
		pmdy->xprev = xcur;
		return xcur;
	}
	
	xout = xcur;

	// if currently changing to new delay target, get next delay value
	
	if ( pmdy->fchanging )
	{
		// get next ramp value, test for done

		int r = RMP_GetNext ( &pmdy->rmp );
		
		if ( RMP_HitEnd( &pmdy->rmp ) )
			pmdy->fchanging = false;
		
		// if new delay different from current delay, change delay

		if ( r != pmdy->Dcur )
		{
			// ramp never changes by more than + or - 1
			
			// change delay tap value to r

			DLY_ChangeVal( pmdy->pdly, r );

			pmdy->Dcur = r;	
			
			// filter delay output within transitions. 
			// note: xprev = xcur = 0 if changing delay on 1st sample

			xout = ( xcur + pmdy->xprev ) >> 1;
		}
	}

	// if self-modulating and timer has expired, get next change

	if ( pmdy->mtime && !pmdy->mtimecur-- )
	{
		pmdy->mtimecur = pmdy->mtime;

		int D0 = pmdy->pdly->D0;
		int Dnew;
		float D1;

		// modulate between 0 and 100% of d0

		D1 = (float)D0 * (1.0 - pmdy->depth);

		Dnew = g_pSoundServices->RandomLong( (int)D1, D0 );

		MDY_ChangeVal ( pmdy, Dnew );
	}

	pmdy->xprev = xcur;

	return xout;
}

// batch version for performance

inline void MDY_GetNextN( mdy_t *pmdy, portable_samplepair_t *pbuffer, int SampleCount, int op )
{
	int count = SampleCount;
	portable_samplepair_t *pb = pbuffer;
	
	switch (op)
	{
	default:
	case OP_LEFT:
		while (count--)
		{
			pb->left = MDY_GetNext( pmdy, pb->left );
			pb++;
		}
		return;
	case OP_RIGHT:
		while (count--)
		{
			pb->right = MDY_GetNext( pmdy, pb->right );
			pb++;
		}
		return;
	case OP_LEFT_DUPLICATE:
		while (count--)
		{
			pb->left = pb->right = MDY_GetNext( pmdy, pb->left );
			pb++;
		}
		return;
	}
}

// parameter order

typedef enum {

	 mdy_idtype,			// NOTE: first 8 params must match params in dly_e
	 mdy_idelay,		
	 mdy_ifeedback,		
	 mdy_igain,		

	 mdy_iftype,
	 mdy_icutoff,	
	 mdy_iqwidth,		
	 mdy_iquality, 
		
	 mdy_imodrate,
	 mdy_imoddepth,
	 mdy_imodglide,

	 mdy_cparam

} mdy_e;


// parameter ranges

prm_rng_t mdy_rng[] = {

	{mdy_cparam,	0, 0},				// first entry is # of parameters
		
	// delay params

	{mdy_idtype,	0, DLY_MAX},		// delay type DLY_PLAIN, DLY_LOWPASS, DLY_ALLPASS	
	{mdy_idelay,	0.0, 1000.0},		// delay in milliseconds
	{mdy_ifeedback,	0.0, 0.99},			// feedback 0-1.0
	{mdy_igain,	    0.0, 1.0},			// final gain of output stage, 0-1.0

	// filter params if mdy type DLY_LOWPASS

	{mdy_iftype,	0, FTR_MAX},		
	{mdy_icutoff,	10.0, 22050.0},
	{mdy_iqwidth,	100.0, 11025.0},
	{mdy_iquality,	0, QUA_MAX},
	
	{mdy_imodrate,	0.01, 200.0},		// frequency at which delay values change to new random value. 0 is no self-modulation
	{mdy_imoddepth,	0.0, 1.0},			// how much delay changes (decreases) from current value (0-1.0) 
	{mdy_imodglide,	0.01, 100.0},		// glide time between dcur and dnew in milliseconds

};


// convert user parameters to internal parameters, allocate and return

mdy_t * MDY_Params ( prc_t *pprc )
{
	mdy_t *pmdy;
	dly_t *pdly;	

	float ramptime = pprc->prm[mdy_imodglide] / 1000.0;			// get ramp time in seconds
	float modtime = 1.0 / pprc->prm[mdy_imodrate];				// time between modulations in seconds
	float depth = pprc->prm[mdy_imoddepth];						// depth of modulations 0-1.0

	// alloc plain, allpass or lowpass delay

	pdly = DLY_Params( pprc );
	
	if ( !pdly )
		return NULL;

	pmdy = MDY_Alloc ( pdly, ramptime, modtime, depth );
	
	return pmdy;
}

inline void * MDY_VParams ( void *p ) 
{
	PRC_CheckParams ( (prc_t *)p, mdy_rng ); 
	return (void *) MDY_Params ((prc_t *)p); 
}

// v is +/- 0-1.0
// change current delay value 0..D

void MDY_Mod ( mdy_t *pmdy, float v ) 
{

	int D = pmdy->Dcur;
	float v2 = -(v + 1.0)/2.0;				// v2 varies -1.0-0.0

	// D varies 0..D

	D = D + (int)((float)D * v2);

	// change delay

	MDY_ChangeVal( pmdy, D );

	return; 
}


///////////////////////////////////////////
// Chorus - lfo modulated delay
///////////////////////////////////////////


#define CCRSS		64				// max number chorus' active

struct crs_t
{
	bool fused;

	mdy_t *pmdy;						// modulatable delay
	lfo_t *plfo;						// modulating lfo

	int lfoprev;						// previous modulator value from lfo
	int mix;							// mix of clean & chorus signal - 0..PMAX

};

crs_t crss[CCRSS];

void CRS_Init( crs_t *pcrs ) { if (pcrs) Q_memset( pcrs, 0, sizeof (crs_t) ); };
void CRS_Free( crs_t *pcrs ) 
{
	if (pcrs)
	{
		MDY_Free ( pcrs->pmdy );
		LFO_Free ( pcrs->plfo );
		Q_memset( pcrs, 0, sizeof (crs_t) ); 
	}
}


void CRS_InitAll() { for (int i = 0; i < CCRSS; i++) CRS_Init( &crss[i] ); }
void CRS_FreeAll() { for (int i = 0; i < CCRSS; i++) CRS_Free( &crss[i] ); }

// fstep is base pitch shift, ie: floating point step value, where 1.0 = +1 octave, 0.5 = -1 octave 
// lfotype is LFO_SIN, LFO_RND, LFO_TRI etc (LFO_RND for chorus, LFO_SIN for flange)
// fHz is modulation frequency in Hz
// depth is modulation depth, 0-1.0
// mix is mix of chorus and clean signal

#define CRS_DELAYMAX			100		// max milliseconds of sweepable delay
#define CRS_RAMPTIME			5		// milliseconds to ramp between new delay values

crs_t * CRS_Alloc( int lfotype, float fHz, float fdepth, float mix ) 
{
	
	int i;
	crs_t *pcrs;
	dly_t *pdly;
	mdy_t *pmdy;
	lfo_t *plfo;
	float ramptime;
	int D;

	// find free chorus slot

	for ( i = 0; i < CCRSS; i++ )
	{
		if ( !crss[i].fused )
			break;
	}

	if ( i == CCRSS ) 
	{
		DevMsg ("DSP: Warning, failed to allocate chorus.\n" );
		return NULL;
	}

	pcrs = &crss[i];

	CRS_Init ( pcrs );

	D = fdepth * MSEC_TO_SAMPS(CRS_DELAYMAX);		// sweep from 0 - n milliseconds

	ramptime = (float) CRS_RAMPTIME / 1000.0;				// # milliseconds to ramp between new values
	
	pdly = DLY_Alloc ( D, 0, 1, DLY_LINEAR );

	pmdy = MDY_Alloc ( pdly, ramptime, 0.0, 0.0 );

	plfo = LFO_Alloc ( lfotype, fHz, false );
	
	if ( !plfo || !pmdy )
	{
		LFO_Free ( plfo );
		MDY_Free ( pmdy );
		DevMsg ("DSP: Warning, failed to allocate lfo or mdy for chorus.\n" );
		return NULL;
	}

	pcrs->pmdy = pmdy;
	pcrs->plfo = plfo;
	pcrs->mix = (int) ( PMAX * mix );
	pcrs->fused = true;

	return pcrs;
}

// return next chorused sample (modulated delay) mixed with input sample

inline int CRS_GetNext( crs_t *pcrs, int x ) 
{
	int l;
	int y;
	
	// get current mod delay value

	y = MDY_GetNext ( pcrs->pmdy, x );

	// get next lfo value for modulation
	// note: lfo must return 0 as first value

	l = LFO_GetNext ( pcrs->plfo, x );

	// if modulator has changed, change mdy

	if ( l != pcrs->lfoprev )
	{
		// calculate new tap (starts at D)

		int D = pcrs->pmdy->pdly->D0;
		int tap;

		// lfo should always output values 0 <= l <= LFOMAX

		if (l < 0)
			l = 0;

		tap = D - ((l * D) >> LFOBITS);

		MDY_ChangeVal ( pcrs->pmdy, tap );

		pcrs->lfoprev = l;
	}

	return ((y * pcrs->mix) >> PBITS) + x;
}

// batch version for performance

inline void CRS_GetNextN( crs_t *pcrs, portable_samplepair_t *pbuffer, int SampleCount, int op )
{
	int count = SampleCount;
	portable_samplepair_t *pb = pbuffer;
	
	switch (op)
	{
	default:
	case OP_LEFT:
		while (count--)
		{
			pb->left = CRS_GetNext( pcrs, pb->left );
			pb++;
		}
		return;
	case OP_RIGHT:
		while (count--)
		{
			pb->right = CRS_GetNext( pcrs, pb->right );
			pb++;
		}
		return;
	case OP_LEFT_DUPLICATE:
		while (count--)
		{
			pb->left = pb->right = CRS_GetNext( pcrs, pb->left );
			pb++;
		}
		return;
	}
}

// parameter order

typedef enum {

	crs_ilfotype,
	crs_irate,
	crs_idepth,
	crs_imix,
	
	crs_cparam

} crs_e;


// parameter ranges

prm_rng_t crs_rng[] = {

	{crs_cparam,	0, 0},				// first entry is # of parameters
		
	{crs_ilfotype,	0, LFO_MAX},		// lfotype is LFO_SIN, LFO_RND, LFO_TRI etc (LFO_RND for chorus, LFO_SIN for flange)
	{crs_irate,		0.0, 1000.0},		// rate is modulation frequency in Hz
	{crs_idepth,	0.0, 1.0},			// depth is modulation depth, 0-1.0
	{crs_imix,	    0.0, 1.0},			// mix is mix of chorus and clean signal

};

// uses pitch, lfowav, rate, depth

crs_t * CRS_Params ( prc_t *pprc )
{
	crs_t *pcrs;
	
	pcrs = CRS_Alloc ( pprc->prm[crs_ilfotype], pprc->prm[crs_irate], pprc->prm[crs_idepth], pprc->prm[crs_imix] );

	return pcrs;
}

inline void * CRS_VParams ( void *p ) 
{
	PRC_CheckParams ( (prc_t *)p, crs_rng ); 
	return (void *) CRS_Params ((prc_t *)p); 
}

inline void CRS_Mod ( void *p, float v ) { return; }


////////////////////////////////////////////////////
// amplifier - modulatable gain, distortion
////////////////////////////////////////////////////

#define CAMPS		64				// max number amps active

#define AMPSLEW		10				// milliseconds of slew time between gain changes

struct amp_t
{
	bool fused;

	float gain;						// amplification 0-6.0
	float vthresh;					// clip distortion threshold 0-1.0
	float distmix;					// 0-1.0 mix of distortion with clean
	float vfeed;					// 0-1.0 feedback with distortion;

	float gaintarget;				// new gain
	float gaindif;					// incrementer
};

amp_t amps[CAMPS];

void AMP_Init( amp_t *pamp ) { if (pamp) Q_memset( pamp, 0, sizeof (amp_t) ); };
void AMP_Free( amp_t *pamp ) 
{
	if (pamp)
	{
		Q_memset( pamp, 0, sizeof (amp_t) ); 
	}
}


void AMP_InitAll() { for (int i = 0; i < CAMPS; i++) AMP_Init( &amps[i] ); }
void AMP_FreeAll() { for (int i = 0; i < CAMPS; i++) AMP_Free( &amps[i] ); }

amp_t * AMP_Alloc( float gain, float vthresh, float distmix, float vfeed ) 
{
	int i;
	amp_t *pamp;

	// find free amp slot

	for ( i = 0; i < CAMPS; i++ )
	{
		if ( !amps[i].fused )
			break;
	}

	if ( i == CAMPS ) 
	{
		DevMsg ("DSP: Warning, failed to allocate amp.\n" );
		return NULL;
	}

	pamp = &amps[i];

	AMP_Init ( pamp );

	pamp->gain = gain;
	pamp->vthresh = vthresh;
	pamp->distmix = distmix;
	pamp->vfeed = vfeed;

	return pamp;
}

// return next amplified sample

inline int AMP_GetNext( amp_t *pamp, int x ) 
{
	float y = (float)x;
	float yin;
	float gain = pamp->gain;

	yin = y;

	// slew between gains

	if ( gain != pamp->gaintarget )
	{
		float gaintarget = pamp->gaintarget;
		float gaindif = pamp->gaindif;

		if (gain > gaintarget)
		{
			gain -= gaindif;
			if (gain <= gaintarget)
				pamp->gaintarget = gain;
		}
		else
		{
			gain += gaindif;
			if (gain >= gaintarget)
				pamp->gaintarget = gain;
		}

		pamp->gain = gain;
	}

	// if distortion is on, add distortion, feedback

	if ( pamp->vthresh < 1.0 )
	{
		float fclip = pamp->vthresh * 32767.0;

		if ( pamp->vfeed > 0.0 )
		{
			// UNDONE: feedback 
		}

		// clip distort

		y = ( y > fclip ? fclip : ( y < -fclip ? -fclip : y));

		// mix distorted with clean (1.0 = full distortion)

		if ( pamp->distmix > 0.0 )
			y = y * pamp->distmix + yin * (1.0 - pamp->distmix);
	}

	// amplify

	y *= gain;

	return (int)y;
}

// batch version for performance

inline void AMP_GetNextN( amp_t *pamp, portable_samplepair_t *pbuffer, int SampleCount, int op )
{
	int count = SampleCount;
	portable_samplepair_t *pb = pbuffer;
	
	switch (op)
	{
	default:
	case OP_LEFT:
		while (count--)
		{
			pb->left = AMP_GetNext( pamp, pb->left );
			pb++;
		}
		return;
	case OP_RIGHT:
		while (count--)
		{
			pb->right = AMP_GetNext( pamp, pb->right );
			pb++;
		}
		return;
	case OP_LEFT_DUPLICATE:
		while (count--)
		{
			pb->left = pb->right = AMP_GetNext( pamp, pb->left );
			pb++;
		}
		return;
	}
}

inline void AMP_Mod( amp_t *pamp, float v )
{
	float vmod = clamp (v, 0.0, 1.0);
	float samps = MSEC_TO_SAMPS(AMPSLEW);		// # samples to slew between amp values

	// ramp to new amplification value

	pamp->gaintarget = pamp->gain * vmod;

	pamp->gaindif = fabs (pamp->gain - pamp->gaintarget) / samps;

	if (pamp->gaindif == 0.0)
		pamp->gaindif = fabs(pamp->gain - pamp->gaintarget)/100;
}


// parameter order

typedef enum {

	amp_gain,
	amp_vthresh,
	amp_distmix,
	amp_vfeed,
	
	amp_cparam

} amp_e;


// parameter ranges

prm_rng_t amp_rng[] = {

	{amp_cparam,	0, 0},				// first entry is # of parameters
		
	{amp_gain,		0.0, 10.0},			// amplification		
	{amp_vthresh,	0.0, 1.0},			// threshold for distortion (1.0 = no distortion)
	{amp_distmix,	0.0, 1.0},			// mix of clean and distortion (1.0 = full distortion, 0.0 = full clean)
	{amp_vfeed,	    0.0, 1.0},			// distortion feedback
};

amp_t * AMP_Params ( prc_t *pprc )
{
	amp_t *pamp;
	
	pamp = AMP_Alloc ( pprc->prm[amp_gain], pprc->prm[amp_vthresh], pprc->prm[amp_distmix], pprc->prm[amp_vfeed] );

	return pamp;
}

inline void * AMP_VParams ( void *p ) 
{
	PRC_CheckParams ( (prc_t *)p, amp_rng ); 
	return (void *) AMP_Params ((prc_t *)p); 
}


/////////////////
// NULL processor
/////////////////

struct nul_t
{
	int type;
};

nul_t nuls[] = {0};

void NULL_Init ( nul_t *pnul ) { }
void NULL_InitAll( ) { }
void NULL_Free ( nul_t *pnul ) { }
void NULL_FreeAll ( ) { }
nul_t *NULL_Alloc ( ) { return &nuls[0]; }

inline int NULL_GetNext ( void *p, int x) { return x; }

inline void NULL_GetNextN( nul_t *pnul, portable_samplepair_t *pbuffer, int SampleCount, int op ) { return; }

inline void NULL_Mod ( void *p, float v ) { return; }

inline void * NULL_VParams ( void *p ) { return (void *) (&nuls[0]); }

//////////////////////////
// DSP processors presets
//////////////////////////

// A dsp processor (prc) performs a single-sample function, such as pitch shift, delay, reverb, filter

// note, this array must have CPRCPARMS entries

#define PRMZERO 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0
#define PFNZERO	NULL,NULL,NULL,NULL,NULL	// zero pointers for pfnparam...pdata within prc_t

//////////////////
// NULL processor
/////////////////

#define PRC_NULL1 {PRC_NULL, PRMZERO, PFNZERO}

#define PRC0	PRC_NULL1

//////////////
// Amplifiers
//////////////
	// {amp_gain,		0.0, 10.0},			// amplification		
	// {amp_vthresh,	0.0, 1.0},			// threshold for distortion (1.0 = no distortion)
	// {amp_distmix,	0.0, 1.0},			// mix of clean and distortion (1.0 = full distortion, 0.0 = full clean)
	// {amp_vfeed,	    0.0, 1.0},			// distortion feedback
	
//					prctype		gain	vthresh	distmix	vfeed 
#define PRC_AMP1	{PRC_AMP,	{1.0,	1.0,	0.0,	0.0, },	PFNZERO}		// modulatable unity gain amp
#define PRC_AMP2	{PRC_AMP,	{1.5,	0.75,	1.0,	0.0, },	PFNZERO}		// amp with light distortion
#define PRC_AMP3	{PRC_AMP,	{2.0,	0.5,	1.0,	0.0, },	PFNZERO}		// amp with medium distortion
#define PRC_AMP4	{PRC_AMP,	{4.0,	0.25,	1.0,	0.0, },	PFNZERO}		// amp with heavy distortion
#define PRC_AMP5	{PRC_AMP,	{10.0,	0.10,	1.0,	0.0, },	PFNZERO}		// mega distortion

#define PRC_AMP6	{PRC_AMP,	{0.1,	1.0,	0.0,	0.0,},	PFNZERO}		// fade out
#define PRC_AMP7	{PRC_AMP,	{0.2,	1.0,	0.0,	0.0,},	PFNZERO}		// fade out
#define PRC_AMP8	{PRC_AMP,	{0.3,	1.0,	0.0,	0.0,},	PFNZERO}		// fade out

#define PRC_AMP9	{PRC_AMP,	{0.75,	1.0,	0.0,	0.0,},	PFNZERO}		// duck out


///////////
// Filters			
///////////

		// ftype: filter type FLT_LP, FLT_HP, FLT_BP (UNDONE: FLT_BP currently ignored)			
		// cutoff: cutoff frequency in hz at -3db gain
		// qwidth: width of BP, or steepness of LP/HP (ie: fcutoff + qwidth = -60db gain point)
		// quality: QUA_LO, _MED, _HI 0,1,2 

//					prctype		ftype	cutoff	qwidth	quality
#define PRC_FLT1	{PRC_FLT, {FLT_LP,	3000,	1000,	QUA_MED,},		PFNZERO} 
#define PRC_FLT2	{PRC_FLT, {FLT_LP,	2000,	2000,	QUA_MED,},		PFNZERO}	// lowpass for facing away
#define PRC_FLT3	{PRC_FLT, {FLT_LP,	1000,	1000,	QUA_MED,},		PFNZERO} 
#define PRC_FLT4	{PRC_FLT, {FLT_LP,	700,	700,	QUA_LO,},		PFNZERO}	// muffle filter

#define PRC_FLT5	{PRC_FLT, {FLT_HP,	700,	200,	QUA_MED,},		PFNZERO}	// highpass (bandpass pair)
#define PRC_FLT6	{PRC_FLT, {FLT_HP,	2000,	1000,	QUA_MED,},		PFNZERO}	// lowpass  (bandpass pair)

//////////
// Delays	
//////////

		// dtype: delay type DLY_PLAIN, DLY_LOWPASS, DLY_ALLPASS	
		// delay: delay in milliseconds
		// feedback: feedback 0-1.0
		// gain: final gain of output stage, 0-1.0

//					prctype		dtype		delay	feedbk	gain	ftype	cutoff	qwidth	quality
#define	PRC_DLY1	{PRC_DLY, {DLY_PLAIN,	500.0,	0.5,	0.6,	0.0,	0.0,	0.0,	0.0,},		PFNZERO}
#define PRC_DLY2	{PRC_DLY, {DLY_LOWPASS,	45.0,	0.8,	0.6,	FLT_LP,	3000,	3000,	QUA_LO,},	PFNZERO}

#define PRC_DLY3	{PRC_DLY, {DLY_LOWPASS,	300.0,	0.5,	0.6,	FLT_LP,	2000,	2000,	QUA_LO,},	PFNZERO} // outside S
#define PRC_DLY4	{PRC_DLY, {DLY_LOWPASS,	400.0,	0.5,	0.6,	FLT_LP,	1500,	1500,	QUA_LO,},	PFNZERO} // outside M
#define PRC_DLY5	{PRC_DLY, {DLY_LOWPASS,	750.0,	0.5,	0.6,	FLT_LP,	1000,	1000,	QUA_LO,},	PFNZERO} // outside L
#define PRC_DLY6	{PRC_DLY, {DLY_LOWPASS,	1000.0,	0.5,	0.6,	FLT_LP,	800,	400,	QUA_LO,},	PFNZERO} // outside VL

#define PRC_DLY7	{PRC_DLY, {DLY_LOWPASS,	45.0,	0.4,	0.5,	FLT_LP,	3000,	3000,	QUA_LO,},	PFNZERO} // tunnel S
#define PRC_DLY8	{PRC_DLY, {DLY_LOWPASS,	55.0,	0.4,	0.5,	FLT_LP,	3000,	3000,	QUA_LO,},	PFNZERO} // tunnel M
#define PRC_DLY9	{PRC_DLY, {DLY_LOWPASS,	65.0,	0.4,	0.5,	FLT_LP,	3000,	3000,	QUA_LO,},	PFNZERO} // tunnel L

#define PRC_DLY10	{PRC_DLY, {DLY_LOWPASS,	150.0,	0.5,	0.6,	FLT_LP,	3000,	3000,	QUA_LO,},	PFNZERO} // cavern S
#define PRC_DLY11	{PRC_DLY, {DLY_LOWPASS,	200.0,	0.7,	0.6,	FLT_LP,	3000,	3000,	QUA_LO,},	PFNZERO} // cavern M
#define PRC_DLY12	{PRC_DLY, {DLY_LOWPASS,	300.0,	0.7,	0.6,	FLT_LP,	3000,	3000,	QUA_LO,},	PFNZERO} // cavern L

#define PRC_DLY13	{PRC_DLY, {DLY_LINEAR,	300.0,	0.0,	1.0,	0.0,	0.0,	0.0,	0.0,},		PFNZERO} // straight delay 300ms
#define PRC_DLY14	{PRC_DLY, {DLY_LINEAR,	80.0,	0.0,	1.0,	0.0,	0.0,	0.0,	0.0,},		PFNZERO} // straight delay 80ms

///////////
// Reverbs
///////////

		// size: 0-2.0 scales nominal delay parameters (starting at approx 20ms)
		// density: 0-2.0 density of reverbs (room shape) - controls # of parallel or series delays
		// decay: 0-2.0 scales feedback parameters (starting at approx 0.15)

//					prctype		size	density	decay	ftype	cutoff	qwidth	fparallel

#define PRC_RVA1	{PRC_RVA,	{2.0,	0.5,	1.5,	FLT_LP,	6000,	2000,	1},		PFNZERO}
#define	PRC_RVA2	{PRC_RVA,	{1.0,	0.2,	1.5,	0,		0,		0,		0},		PFNZERO}

#define PRC_RVA3	{PRC_RVA,	{0.8,	0.5,	1.5,	FLT_LP,	2500,	2000,	0},		PFNZERO}	// metallic S
#define PRC_RVA4	{PRC_RVA,	{1.0,	0.5,	1.5,	FLT_LP,	2500,	2000,	0},		PFNZERO}	// metallic M
#define PRC_RVA5	{PRC_RVA,	{1.2,	0.5,	1.5,	FLT_LP,	2500,	2000,	0},		PFNZERO}	// metallic L

#define PRC_RVA6	{PRC_RVA,	{0.8,	0.3,	1.5,	FLT_LP,	4000,	2000,	0},		PFNZERO}	// tunnel S
#define PRC_RVA7	{PRC_RVA,	{0.9,	0.3,	1.5,	FLT_LP,	4000,	2000,	0},		PFNZERO}	// tunnel M
#define PRC_RVA8	{PRC_RVA,	{1.0,	0.3,	1.5,	FLT_LP,	4000,	2000,	0},		PFNZERO}	// tunnel L

#define PRC_RVA9	{PRC_RVA,	{2.0,	1.5,	2.0,	FLT_LP,	1500,	1500,	1},		PFNZERO}	// cavern S
#define PRC_RVA10	{PRC_RVA,	{2.0,	1.5,	2.0,	FLT_LP,	1500,	1500,	1},		PFNZERO}	// cavern M
#define PRC_RVA11	{PRC_RVA,	{2.0,	1.5,	2.0,	FLT_LP,	1500,	1500,	1},		PFNZERO}	// cavern L

#define PRC_RVA12	{PRC_RVA,	{2.0,	0.5,	1.5,	FLT_LP,	6000,	2000,	1},		PFNZERO}	// chamber S
#define PRC_RVA13	{PRC_RVA,	{2.0,	1.0,	1.5,	FLT_LP,	6000,	2000,	1},		PFNZERO}	// chamber M
#define PRC_RVA14	{PRC_RVA,	{2.0,	2.0,	1.5,	FLT_LP,	6000,	2000,	1},		PFNZERO}	// chamber L

#define PRC_RVA15	{PRC_RVA,	{1.7,	1.0,	1.2,	FLT_LP,	5000,	4000,	1},		PFNZERO}	// brite S
#define PRC_RVA16	{PRC_RVA,	{1.75,	1.0,	1.5,	FLT_LP,	5000,	4000,	1},		PFNZERO}	// brite M
#define PRC_RVA17	{PRC_RVA,	{1.85,	1.0,	2.0,	FLT_LP,	6000,	4000,	1},		PFNZERO}	// brite L

#define PRC_RVA18	{PRC_RVA,	{1.0,	1.5,	1.0,	FLT_LP,	1000,	1000,	0},		PFNZERO}	// generic

#define PRC_RVA19	{PRC_RVA,	{1.9,	1.8,	1.25,	FLT_LP,	4000,	2000,	1},		PFNZERO}	// concrete S
#define PRC_RVA20	{PRC_RVA,	{2.0,	1.8,	1.5,	FLT_LP,	3500,	2000,	1},		PFNZERO}	// concrete M
#define PRC_RVA21	{PRC_RVA,	{2.0,	1.8,	1.75,	FLT_LP,	3000,	2000,	1},		PFNZERO}	// concrete L

#define PRC_RVA22	{PRC_RVA,	{1.8,	1.5,	1.5,	FLT_LP,	1000,	1000,	0},		PFNZERO}	// water S
#define PRC_RVA23	{PRC_RVA,	{1.9,	1.75,	1.5,	FLT_LP,	1000,	1000,	0},		PFNZERO}	// water M
#define PRC_RVA24	{PRC_RVA,	{2.0,	2.0,	1.5,	FLT_LP,	1000,	1000,	0},		PFNZERO}	// water L


/////////////
// Diffusors	
/////////////

		// size: 0-1.0 scales all delays
		// density: 0-1.0 controls # of series delays
		// decay: 0-1.0 scales all feedback parameters 

//					prctype		size	density	decay

#define	PRC_DFR1	{PRC_DFR,	{1.0,	0.5,	1.0},	PFNZERO}

#define PRC_DFR2	{PRC_DFR,	{0.5,	0.3,	0.5},	PFNZERO}	// S
#define PRC_DFR3	{PRC_DFR,	{0.75,	0.5,	0.75},	PFNZERO}	// M
#define PRC_DFR4	{PRC_DFR,	{1.0,	0.5,	1.0},	PFNZERO}	// L
#define PRC_DFR5	{PRC_DFR,	{1.0,	1.0,	1.0},	PFNZERO}	// VL

////////
// LFOs		
////////
		// wavtype: lfo type to use (LFO_SIN, LFO_RND...)
		// rate: modulation rate in hz. for MDY, 1/rate = 'glide' time in seconds
		// foneshot: 1.0 if lfo is oneshot

//					prctype		wavtype		rate	foneshot

#define	PRC_LFO1	{PRC_LFO, {LFO_SIN,		440.0,	0.0,},		PFNZERO}
#define	PRC_LFO2	{PRC_LFO, {LFO_SIN,		3000.0,	0.0,},		PFNZERO}	// ear noise ring
#define	PRC_LFO3	{PRC_LFO, {LFO_SIN,		4500.0,	0.0,},		PFNZERO}	// ear noise ring
#define	PRC_LFO4	{PRC_LFO, {LFO_SIN,		6000.0,	0.0,},		PFNZERO}	// ear noise ring
#define PRC_LFO5	{PRC_LFO, {LFO_SAW,		100.0,	0.0,},		PFNZERO}	// sub bass

/////////
// Pitch	
/////////

		// pitch: 0-n.0 where 1.0 = 1 octave up and 0.5 is one octave down	
		// timeslice: in milliseconds - size of sound chunk to analyze and cut/duplicate - 100ms nominal
		// xfade: in milliseconds - size of crossfade region between spliced chunks - 20ms nominal
	
//					prctype		pitch	timeslice	xfade

#define	PRC_PTC1	{PRC_PTC,	{1.1,	100.0,		20.0},		PFNZERO}	// pitch up 10%
#define PRC_PTC2	{PRC_PTC,	{0.9,	100.0,		20.0},		PFNZERO}	// pitch down 10%
#define PRC_PTC3	{PRC_PTC,	{0.95,	100.0,		20.0},		PFNZERO}	// pitch down 5%
#define PRC_PTC4	{PRC_PTC,	{1.01,	100.0,		20.0},		PFNZERO}	// pitch up 1%
#define PRC_PTC5	{PRC_PTC,	{0.5,	100.0,		20.0},		PFNZERO}	// pitch down 50%

/////////////
// Envelopes	
/////////////

		// etype: ENV_LINEAR, ENV_LOG - currently ignored
		// amp1: attack peak amplitude 0-1.0
		// amp2: decay target amplitued 0-1.0
		// amp3: sustain target amplitude 0-1.0
		// attack time in milliseconds
		// envelope decay time in milliseconds
		// sustain time in milliseconds	
		// release time in milliseconds	

//					prctype		etype	amp1	amp2	amp3	attack	decay	sustain	release

#define	PRC_ENV1	{PRC_ENV,{ENV_LIN,	1.0,	0.5,	0.4,	500,	500,	3000,	6000},	PFNZERO}


//////////////
// Mod delays	
//////////////

		// dtype: delay type DLY_PLAIN, DLY_LOWPASS, DLY_ALLPASS	
		// delay: delay in milliseconds
		// feedback: feedback 0-1.0
		// gain: final gain of output stage, 0-1.0

		// modrate: frequency at which delay values change to new random value. 0 is no self-modulation
		// moddepth: how much delay changes (decreases) from current value (0-1.0) 
		// modglide: glide time between dcur and dnew in milliseconds

//					prctype		dtype		delay	feedback	gain	ftype	cutoff	qwidth	qual	modrate	moddepth modglide
#define	PRC_MDY1	{PRC_MDY,	{DLY_PLAIN,	500.0,	0.5,		1.0,	0,		0,		0,		0,		10,		0.8,		5,}, PFNZERO}			
#define PRC_MDY2	{PRC_MDY,	{DLY_PLAIN,	50.0,	0.8,		1.0,	0,		0,		0,		0,		5,		0.8,		5,}, PFNZERO}

#define PRC_MDY3	{PRC_MDY,	{DLY_PLAIN,	300.0,	0.2,		1.0,	0,		0,		0,		0,		30,		0.01,		15,}, PFNZERO}  // weird 1
#define PRC_MDY4	{PRC_MDY,	{DLY_PLAIN,	400.0,	0.3,		1.0,	0,		0,		0,		0,		0.25,	0.01,		15,}, PFNZERO}  // weird 2
#define PRC_MDY5	{PRC_MDY,	{DLY_PLAIN,	500.0,	0.4,		1.0,	0,		0,		0,		0,		0.25,	0.01,		15,}, PFNZERO} // weird 3

//////////
// Chorus	
//////////

		// lfowav: lfotype is LFO_SIN, LFO_RND, LFO_TRI etc (LFO_RND for chorus, LFO_SIN for flange)
		// rate: rate is modulation frequency in Hz
		// depth: depth is modulation depth, 0-1.0
		// mix: mix is mix of chorus and clean signal

//					prctype		lfowav		rate	depth	mix
#define	PRC_CRS1	{PRC_CRS,	{LFO_SIN,	10,		1.0,	0.5,},	PFNZERO}

/////////////////////
// Envelope follower
/////////////////////

		// takes no parameters
#define PRC_EFO1	{PRC_EFO,	{PRMZERO}, PFNZERO}





// init array of processors - first store pfnParam, pfnGetNext and pfnFree functions for type,
// then call the pfnParam function to initialize each processor

// prcs - an array of prc structures, all with initialized params
// count - number of elements in the array

// returns false if failed to init one or more processors

bool PRC_InitAll( prc_t *prcs, int count ) 
{ 
	int i;
	prc_Param_t pfnParam;			// allocation function - takes ptr to prc, returns ptr to specialized data struct for proc type
	prc_GetNext_t pfnGetNext;		// get next function
	prc_GetNextN_t pfnGetNextN;		// get next function, batch version
	prc_Free_t pfnFree;	
	prc_Mod_t pfnMod;	

	bool fok = true;;

	// set up pointers to XXX_Free, XXX_GetNext and XXX_Params functions

	for (i = 0; i < count; i++)
	{
		switch (prcs[i].type)
		{
		default:
		case PRC_NULL:
			pfnFree		= &(prc_Free_t)NULL_Free;
			pfnGetNext	= &(prc_GetNext_t)NULL_GetNext;
			pfnGetNextN	= &(prc_GetNextN_t)NULL_GetNextN;
			pfnParam	= &NULL_VParams;
			pfnMod		= &(prc_Mod_t)NULL_Mod;
			break;
		case PRC_DLY:
			pfnFree		= &(prc_Free_t)DLY_Free;
			pfnGetNext	= &(prc_GetNext_t)DLY_GetNext;
			pfnGetNextN	= &(prc_GetNextN_t)DLY_GetNextN;
			pfnParam	= &DLY_VParams;
			pfnMod		= &(prc_Mod_t)DLY_Mod;
			break;
		case PRC_RVA:
			pfnFree		= &(prc_Free_t)RVA_Free;
			pfnGetNext	= &(prc_GetNext_t)RVA_GetNext;
			pfnGetNextN	= &(prc_GetNextN_t)RVA_GetNextN;
			pfnParam	= &RVA_VParams;
			pfnMod		= &(prc_Mod_t)RVA_Mod;
			break;
		case PRC_FLT:
			pfnFree		= &(prc_Free_t)FLT_Free;
			pfnGetNext	= &(prc_GetNext_t)FLT_GetNext;
			pfnGetNextN	= &(prc_GetNextN_t)FLT_GetNextN;
			pfnParam	= &FLT_VParams;
			pfnMod		= &(prc_Mod_t)FLT_Mod;
			break;
		case PRC_CRS:
			pfnFree		= &(prc_Free_t)CRS_Free;
			pfnGetNext	= &(prc_GetNext_t)CRS_GetNext;
			pfnGetNextN	= &(prc_GetNextN_t)CRS_GetNextN;
			pfnParam	= &CRS_VParams;
			pfnMod		= &(prc_Mod_t)CRS_Mod;
			break;
		case PRC_PTC:
			pfnFree		= &(prc_Free_t)PTC_Free;
			pfnGetNext	= &(prc_GetNext_t)PTC_GetNext;
			pfnGetNextN	= &(prc_GetNextN_t)PTC_GetNextN;
			pfnParam	= &PTC_VParams;
			pfnMod		= &(prc_Mod_t)PTC_Mod;
			break;
		case PRC_ENV:
			pfnFree		= &(prc_Free_t)ENV_Free;
			pfnGetNext	= &(prc_GetNext_t)ENV_GetNext;
			pfnGetNextN	= &(prc_GetNextN_t)ENV_GetNextN;
			pfnParam	= &ENV_VParams;
			pfnMod		= &(prc_Mod_t)ENV_Mod;
			break;
		case PRC_LFO:
			pfnFree		= &(prc_Free_t)LFO_Free;
			pfnGetNext	= &(prc_GetNext_t)LFO_GetNext;
			pfnGetNextN	= &(prc_GetNextN_t)LFO_GetNextN;
			pfnParam	= &LFO_VParams;
			pfnMod		= &(prc_Mod_t)LFO_Mod;
			break;
		case PRC_EFO:
			pfnFree		= &(prc_Free_t)EFO_Free;
			pfnGetNext	= &(prc_GetNext_t)EFO_GetNext;
			pfnGetNextN	= &(prc_GetNextN_t)EFO_GetNextN;
			pfnParam	= &EFO_VParams;
			pfnMod		= &(prc_Mod_t)EFO_Mod;
			break;
		case PRC_MDY:
			pfnFree		= &(prc_Free_t)MDY_Free;
			pfnGetNext	= &(prc_GetNext_t)MDY_GetNext;
			pfnGetNextN	= &(prc_GetNextN_t)MDY_GetNextN;
			pfnParam	= &MDY_VParams;
			pfnMod		= &(prc_Mod_t)MDY_Mod;
			break;
		case PRC_DFR:
			pfnFree		= &(prc_Free_t)DFR_Free;
			pfnGetNext	= &(prc_GetNext_t)DFR_GetNext;
			pfnGetNextN	= &(prc_GetNextN_t)DFR_GetNextN;
			pfnParam	= &DFR_VParams;
			pfnMod		= &(prc_Mod_t)DFR_Mod;
			break;
		case PRC_AMP:
			pfnFree		= &(prc_Free_t)AMP_Free;
			pfnGetNext	= &(prc_GetNext_t)AMP_GetNext;
			pfnGetNextN	= &(prc_GetNextN_t)AMP_GetNextN;
			pfnParam	= &AMP_VParams;
			pfnMod		= &(prc_Mod_t)AMP_Mod;
			break;
		}

		// set up function pointers

		prcs[i].pfnParam	= pfnParam;
		prcs[i].pfnGetNext	= pfnGetNext;
		prcs[i].pfnGetNextN	= pfnGetNextN;
		prcs[i].pfnFree		= pfnFree;

		// call param function, store pdata for the processor type

		prcs[i].pdata = pfnParam ( (void *) (&prcs[i]) );

		if ( !prcs[i].pdata )
			fok = false;
	}

	return fok;
}

// free individual processor's data

void PRC_Free ( prc_t *pprc )
{
	if ( pprc->pfnFree && pprc->pdata )
		pprc->pfnFree ( pprc->pdata );
}

// free all processors for supplied array
// prcs - array of processors
// count - elements in array

void PRC_FreeAll ( prc_t *prcs, int count )
{
	for (int i = 0; i < count; i++)
		PRC_Free( &prcs[i] );
}

// get next value for processor - (usually called directly by PSET_GetNext)

inline int PRC_GetNext ( prc_t *pprc, int x )
{
	return pprc->pfnGetNext ( pprc->pdata, x );
}

// automatic parameter range limiting
// force parameters between specified min/max in param_rng

void PRC_CheckParams ( prc_t *pprc, prm_rng_t *prng )
{
	// first entry in param_rng is # of parameters

	int cprm = prng[0].iprm;

	for (int i = 0; i < cprm; i++)
	{
		// if parameter is 0.0, always allow it (this is 'off' for most params)

		if ( pprc->prm[i] != 0.0 && (pprc->prm[i] > prng[i+1].hi || pprc->prm[i] < prng[i+1].lo) )
		{
			DevMsg ("DSP: Warning, clamping out of range parameter.\n" );
			pprc->prm[i] = clamp (pprc->prm[i], prng[i+1].lo, prng[i+1].hi);
		}
	}
}


// DSP presets

// A dsp preset comprises one or more dsp processors in linear, parallel or feedback configuration

// preset configurations
//
#define PSET_SIMPLE		0

// x(n)--->P(0)--->y(n)

#define PSET_LINEAR		1

// x(n)--->P(0)-->P(1)-->...P(m)--->y(n)

#define PSET_PARALLEL6	4

// x(n)-P(0)-->P(1)-->P(2)-->(+)-P(5)->y(n)
//          |				  ^
//		    |                 | 
//		    -->P(3)-->P(4)---->


#define PSET_PARALLEL2	5
	
// x(n)--->P(0)-->(+)-->y(n)
//      	       ^
//		           | 
// x(n)--->P(1)-----

#define PSET_PARALLEL4	6

// x(n)--->P(0)-->P(1)-->(+)-->y(n)
//      				  ^
//		                  | 
// x(n)--->P(2)-->P(3)-----

#define PSET_PARALLEL5	7

// x(n)--->P(0)-->P(1)-->(+)-->P(4)-->y(n)
//      				  ^
//		                  | 
// x(n)--->P(2)-->P(3)-----

#define PSET_FEEDBACK	8
 
// x(n)-P(0)--(+)-->P(1)-->P(2)-->P(5)->y(n)
//             ^				|
//             |                v 
//		       -----P(4)<--P(3)--

#define PSET_FEEDBACK3	9
 
// x(n)---(+)-->P(0)--------->y(n)
//         ^                |
//         |                v 
//		   -----P(2)<--P(1)--

#define PSET_FEEDBACK4	10

// x(n)---(+)-->P(0)-------->P(3)--->y(n)
//         ^              |
//         |              v 
//		   ---P(2)<--P(1)--

#define PSET_MOD		11

//
// x(n)------>P(1)--P(2)--P(3)--->y(n)
//                    ^     
// x(n)------>P(0)....:

#define PSET_MOD2		12

//
// x(n)-------P(1)-->y(n)
//              ^     
// x(n)-->P(0)..:


#define PSET_MOD3		13

//
// x(n)-------P(1)-->P(2)-->y(n)
//              ^     
// x(n)-->P(0)..:


#define CPSETS			64				// max number of presets simultaneously active

#define CPSET_PRCS		6				// max # of processors per dsp preset
#define CPSET_STATES	(CPSET_PRCS+3)	// # of internal states

// NOTE: do not reorder members of pset_t - psettemplates relies on it!!!

struct pset_t
{
	int type;							// preset configuration type
	int cprcs;							// number of processors for this preset

	prc_t prcs[CPSET_PRCS];				// processor preset data
	float gain;							// preset gain 0.1->2.0

	int w[CPSET_STATES];				// internal states
	int fused;
};

pset_t psets[CPSETS];

// array of dsp presets, each with up to 6 processors per preset

#define WZERO	{0,0,0,0,0,0,0,0,0},0

pset_t psettemplates[] = 
{
// presets 0-29 map to legacy room_type 0-29

// type		 # proc	 P0			P1			P2			P3			P4			P5		  GAIN
{PSET_SIMPLE,	1, { PRC_NULL1,	PRC0,		PRC0,		PRC0,		PRC0,		PRC0	},1.0, WZERO }, // OFF		0

{PSET_SIMPLE,	1, { PRC_RVA18,	PRC0,		PRC0,		PRC0,		PRC0,		PRC0	},1.4, WZERO }, // GENERIC	1		// general, low reflective, diffuse room

{PSET_LINEAR,	2, { PRC_DFR1,	PRC_RVA3,	PRC0,		PRC0,		PRC0,		PRC0	},1.4, WZERO }, // METALIC_S	2		// highly reflective, parallel surfaces 
{PSET_LINEAR,	2, { PRC_DFR1,	PRC_RVA4,	PRC0,		PRC0,		PRC0,		PRC0	},1.4, WZERO }, // METALIC_M	3
{PSET_LINEAR,	2, { PRC_DFR1,	PRC_RVA5,	PRC0,		PRC0,		PRC0,		PRC0	},1.4, WZERO }, // METALIC_L	4

{PSET_LINEAR,	2, { PRC_DFR1,	PRC_RVA6,	PRC0,		PRC0,		PRC0,		PRC0	},2.0, WZERO }, // TUNNEL_S	5		// resonant reflective, long surfaces
{PSET_LINEAR,	2, { PRC_DFR1,	PRC_RVA7,	PRC0,		PRC0,		PRC0,		PRC0	},1.8, WZERO }, // TUNNEL_M	6
{PSET_LINEAR,	2, { PRC_DFR1,	PRC_RVA8,	PRC0,		PRC0,		PRC0,		PRC0	},1.7, WZERO }, // TUNNEL_L	7

{PSET_LINEAR,	2, { PRC_DFR1,	PRC_RVA12,	PRC0,		PRC0,		PRC0,		PRC0	},1.7, WZERO }, // CHAMBER_S	8		// diffuse, moderately reflective surfaces
{PSET_LINEAR,	2, { PRC_DFR1,	PRC_RVA13,	PRC0,		PRC0,		PRC0,		PRC0	},1.7, WZERO }, // CHAMBER_M	9
{PSET_LINEAR,	2, { PRC_DFR1,	PRC_RVA14,	PRC0,		PRC0,		PRC0,		PRC0	},1.9, WZERO }, // CHAMBER_L	10

{PSET_SIMPLE,	1, { PRC_RVA15,	PRC0,		PRC0,		PRC0,		PRC0,		PRC0	},1.5, WZERO }, // BRITE_S	11		// diffuse, highly reflective
{PSET_SIMPLE,	1, { PRC_RVA16,	PRC0,		PRC0,		PRC0,		PRC0,		PRC0	},1.6, WZERO }, // BRITE_M	12
{PSET_SIMPLE,	1, { PRC_RVA17,	PRC0,		PRC0,		PRC0,		PRC0,		PRC0	},1.7, WZERO }, // BRITE_L	13

{PSET_LINEAR,	2, { PRC_DFR1,	PRC_RVA22,	PRC0,		PRC0,		PRC0,		PRC0	},1.8, WZERO }, // WATER1	14		// underwater fx
{PSET_LINEAR,	2, { PRC_DFR1,	PRC_RVA23,	PRC0,		PRC0,		PRC0,		PRC0	},1.8, WZERO }, // WATER2	15
{PSET_LINEAR,	3, { PRC_DFR1,	PRC_RVA24,	PRC_MDY5,	PRC0,		PRC0,		PRC0	},1.8, WZERO }, // WATER3	16

{PSET_LINEAR,	2, { PRC_DFR1,	PRC_RVA19,	PRC0,		PRC0,		PRC0,		PRC0	},1.7, WZERO }, // CONCRTE_S	17		// bare, reflective, parallel surfaces
{PSET_LINEAR,	2, { PRC_DFR1,	PRC_RVA20,	PRC0,		PRC0,		PRC0,		PRC0	},1.8, WZERO }, // CONCRTE_M	18
{PSET_LINEAR,	2, { PRC_DFR1,	PRC_RVA21,	PRC0,		PRC0,		PRC0,		PRC0	},1.9, WZERO }, // CONCRTE_L	19

{PSET_LINEAR,	2, { PRC_DFR1,	PRC_DLY3,	PRC0,		PRC0,		PRC0,		PRC0	},1.7, WZERO }, // OUTSIDE1	20		// echoing, moderately reflective
{PSET_LINEAR,	2, { PRC_DFR1,	PRC_DLY4,	PRC0,		PRC0,		PRC0,		PRC0	},1.7, WZERO }, // OUTSIDE2	21		// echoing, dull
{PSET_LINEAR,	3, { PRC_DFR1,	PRC_DFR1,	PRC_DLY5,	PRC0,		PRC0,		PRC0	},1.6, WZERO }, // OUTSIDE3	22		// echoing, very dull

{PSET_LINEAR,	2, { PRC_DLY10,	PRC_RVA10,	PRC0,		PRC0,		PRC0,		PRC0	},2.8, WZERO }, // CAVERN_S	23		// large, echoing area
{PSET_LINEAR,	2, { PRC_DLY11,	PRC_RVA10,	PRC0,		PRC0,		PRC0,		PRC0	},2.6, WZERO }, // CAVERN_M	24
{PSET_LINEAR,	3, { PRC_DFR1,	PRC_DLY12,	PRC_RVA11,	PRC0,		PRC0,		PRC0	},2.6, WZERO }, // CAVERN_L	25

{PSET_LINEAR,	2, { PRC_DLY7,	PRC_DFR1,	PRC0,		PRC0,		PRC0,		PRC0	},2.0, WZERO }, // WEIRDO1	26
{PSET_LINEAR,	2, { PRC_DLY8,	PRC_DFR1,	PRC0,		PRC0,		PRC0,		PRC0	},1.9, WZERO }, // WEIRDO2	27
{PSET_LINEAR,	2, { PRC_DLY9,	PRC_DFR1,	PRC0,		PRC0,		PRC0,		PRC0	},1.8, WZERO }, // WEIRDO3	28
{PSET_LINEAR,	2, { PRC_DLY9,	PRC_DFR1,	PRC0,		PRC0,		PRC0,		PRC0	},1.8, WZERO }, // WEIRDO4	29

// presets 30-40 are new presets

{PSET_SIMPLE,	1, { PRC_FLT2,	PRC0,		PRC0,		PRC0,		PRC0,		PRC0	},1.0, WZERO }, // 30 lowpass - facing away
{PSET_LINEAR,	2, { PRC_FLT3,	PRC_DLY14,	PRC0,		PRC0,		PRC0,		PRC0	},1.0, WZERO }, // 31 lowpass - facing away+80ms delay

//{PSET_PARALLEL2,2, { PRC_AMP6,	PRC_LFO2,	PRC0,		PRC0,		PRC0,		PRC0	},1.0, WZERO }, // 32 explosion ring 1
//{PSET_PARALLEL2,2, { PRC_AMP7,	PRC_LFO3,	PRC0,		PRC0,		PRC0,		PRC0	},1.0, WZERO }, // 33 explosion ring 2
//{PSET_PARALLEL2,2, { PRC_AMP8,	PRC_LFO4,	PRC0,		PRC0,		PRC0,		PRC0	},1.0, WZERO }, // 34 explosion ring 3
{PSET_LINEAR,	3, { PRC_DFR1,	PRC_DFR1,  PRC_FLT3,  	PRC0,		PRC0,		PRC0	},0.25, WZERO }, // 32 explosion ring 
{PSET_LINEAR,	3, { PRC_DFR1,	PRC_DFR1,  PRC_FLT3,	PRC0,		PRC0,		PRC0	},0.25, WZERO }, // 33 explosion ring 2
{PSET_LINEAR,	3, { PRC_DFR1,	PRC_DFR1,  PRC_FLT3,	PRC0,		PRC0,		PRC0	},0.25, WZERO }, // 34 explosion ring 3

{PSET_PARALLEL2,2, { PRC_DFR1,	PRC_LFO2,	PRC0,		PRC0,		PRC0,		PRC0	},0.25, WZERO }, // 35 shock muffle 1
{PSET_PARALLEL2,2, { PRC_DFR1,	PRC_LFO2,	PRC0,		PRC0,		PRC0,		PRC0	},0.25, WZERO }, // 36 shock muffle 2
{PSET_PARALLEL2,2, { PRC_DFR1,	PRC_LFO2,	PRC0,		PRC0,		PRC0,		PRC0	},0.25, WZERO }, // 37 shock muffle 3
//{PSET_LINEAR,	3, { PRC_DFR1,	PRC_LFO4,  PRC_FLT3,	PRC0,		PRC0,		PRC0	},1.0, WZERO }, // 35 shock muffle 1
//{PSET_LINEAR,	3, { PRC_DFR1,	PRC_LFO4,  PRC_FLT3,	PRC0,		PRC0,		PRC0	},1.0, WZERO }, // 36 shock muffle 2
//{PSET_LINEAR,	3, { PRC_DFR1,	PRC_LFO4,  PRC_FLT3,	PRC0,		PRC0,		PRC0	},1.0, WZERO }, // 37 shock muffle 3

{PSET_FEEDBACK3,3, { PRC_DLY13,	PRC_PTC4,	PRC_FLT2,	PRC0,		PRC0,		PRC0	},0.25, WZERO }, // 38 fade pitchdown 1
{PSET_LINEAR,	3, { PRC_AMP3,	PRC_FLT5,	PRC_FLT6,	PRC0,		PRC0,		PRC0	},2.0, WZERO }, // 39 distorted speaker 1

// fade out fade in

// presets 40+ are test presets

{PSET_SIMPLE,	1, { PRC_NULL1,	PRC0,		PRC0,		PRC0,		PRC0,		PRC0	},1.0, WZERO }, // 39 null
{PSET_SIMPLE,	1, { PRC_DLY1,	PRC0,		PRC0,		PRC0,		PRC0,		PRC0	},1.0, WZERO }, // 40 delay
{PSET_SIMPLE,	1, { PRC_RVA1,	PRC0,		PRC0,		PRC0,		PRC0,		PRC0	},1.0, WZERO }, // 41 parallel reverb
{PSET_SIMPLE,	1, { PRC_DFR1,	PRC0,		PRC0,		PRC0,		PRC0,		PRC0	},1.0, WZERO }, // 42 series diffusor
{PSET_LINEAR,	2, { PRC_DFR1,	PRC_RVA1,	PRC0,		PRC0,		PRC0,		PRC0	},1.0, WZERO }, // 43 diff & reverb
{PSET_SIMPLE,	1, { PRC_DLY2,	PRC0,		PRC0,		PRC0,		PRC0,		PRC0	},1.0, WZERO }, // 44 lowpass delay
{PSET_SIMPLE,	1, { PRC_MDY2,	PRC0,		PRC0,		PRC0,		PRC0,		PRC0	},1.0, WZERO }, // 45 modulating delay

{PSET_SIMPLE,	1, { PRC_PTC1,	PRC0,		PRC0,		PRC0,		PRC0,		PRC0	},1.0, WZERO }, // 46 pitch shift
{PSET_SIMPLE,	1, { PRC_PTC2,	PRC0,		PRC0,		PRC0,		PRC0,		PRC0	},1.0, WZERO }, // 47 pitch shift
{PSET_SIMPLE,	1, { PRC_FLT1,	PRC0,		PRC0,		PRC0,		PRC0,		PRC0	},1.0, WZERO }, // 48 filter
{PSET_SIMPLE,	1, { PRC_CRS1,	PRC0,		PRC0,		PRC0,		PRC0,		PRC0	},1.0, WZERO }, // 49 chorus
{PSET_SIMPLE,	1, { PRC_ENV1,	PRC0,		PRC0,		PRC0,		PRC0,		PRC0	},1.0, WZERO }, // 50 
{PSET_SIMPLE,	1, { PRC_LFO1,	PRC0,		PRC0,		PRC0,		PRC0,		PRC0	},1.0, WZERO }, // 51 lfo
{PSET_SIMPLE,	1, { PRC_EFO1,	PRC0,		PRC0,		PRC0,		PRC0,		PRC0	},1.0, WZERO }, // 52
{PSET_SIMPLE,	1, { PRC_MDY1,	PRC0,		PRC0,		PRC0,		PRC0,		PRC0	},1.0, WZERO }, // 53 modulating delay

{PSET_SIMPLE,	1, { PRC_FLT2,	PRC0,		PRC0,		PRC0,		PRC0,		PRC0	},1.0, WZERO }, // 54 lowpass - facing away
{PSET_PARALLEL2,2, { PRC_PTC2,	PRC_PTC1,	PRC0,		PRC0,		PRC0,		PRC0	},1.0, WZERO }, // 55 ptc1/ptc2
{PSET_FEEDBACK, 6, { PRC_DLY1,	PRC0,		PRC0,		PRC_PTC1,	PRC_FLT1,	PRC0	},1.0, WZERO }, // 56 dly/ptc1
{PSET_MOD,		4, { PRC_EFO1,	PRC0,		PRC_PTC1,	PRC0,		PRC0,		PRC0	},1.0, WZERO }, // 57 efo mod ptc
{PSET_LINEAR,	3, { PRC_DLY1,	PRC_RVA1,	PRC_CRS1,	PRC0,		PRC0,		PRC0	},1.0, WZERO }  // 58 dly/rvb/crs
};				


// number of presets currently defined above

#define CPSETTEMPLATES  (sizeof(psets)/sizeof(pset_t))

// init a preset - just clear state array

void PSET_Init( pset_t *ppset ) 
{ 
	// clear state array

	if (ppset)
		Q_memset( ppset->w, 0, sizeof (int) * (CPSET_STATES) ); 
}

// clear runtime slots

void PSET_InitAll( void )
{
	for (int i = 0; i < CPSETS; i++)
		Q_memset( &psets[i], 0, sizeof(pset_t));
}

// free the preset - free all processors

void PSET_Free( pset_t *ppset ) 
{ 
	if (ppset)
	{
		// free processors

		PRC_FreeAll ( ppset->prcs, ppset->cprcs );

		// clear

		Q_memset( ppset, 0, sizeof (pset_t));
	}
}

void PSET_FreeAll() { for (int i = 0; i < CPSETS; i++) PSET_Free( &psets[i] ); };

// return preset struct, given index into preset template array
// NOTE: should not ever be more than 2 or 3 of these active simultaneously

pset_t * PSET_Alloc ( int ipsettemplate )
{
	pset_t *ppset;
	bool fok;

	// don't excede array bounds

	if ( ipsettemplate >= CPSETTEMPLATES)
		ipsettemplate = 0;

	// find free slot

	for (int i = 0; i < CPSETS; i++)
	{
		if ( !psets[i].fused )
			break;
	}

	if ( i == CPSETS )
		return NULL;
	
	ppset = &psets[i];
	
	// copy template into preset

	*ppset = psettemplates[ipsettemplate];

	ppset->fused = true;

	// clear state array

	PSET_Init ( ppset );
	
	// init all processors, set up processor function pointers

	fok = PRC_InitAll( ppset->prcs, ppset->cprcs );

	if ( !fok )
	{
		// failed to init one or more processors
		g_pSoundServices->ConSafePrint ("Sound DSP: preset failed to init.\n");
		PRC_FreeAll ( ppset->prcs, ppset->cprcs );
		return NULL;
	}

	return ppset;
}

// batch version of PSET_GetNext for linear array of processors.  For performance.

// ppset - preset array
// pbuffer - input sample data 
// SampleCount - size of input buffer
// OP:	OP_LEFT				- process left channel in place
//		OP_RIGHT			- process right channel in place
//		OP_LEFT_DUPLICATe	- process left channel, duplicate into right

inline void PSET_GetNextN( pset_t *ppset, portable_samplepair_t *pbuffer, int SampleCount, int op )
{
	portable_samplepair_t *pbf = pbuffer;
	prc_t *pprc;
	int count = ppset->cprcs;
	
	switch ( ppset->type )
	{
		default:
		case PSET_SIMPLE:
		{
			// x(n)--->P(0)--->y(n)

			ppset->prcs[0].pfnGetNextN (ppset->prcs[0].pdata, pbf, SampleCount, op);
			return;
		}
		case PSET_LINEAR:
		{

			//      w0     w1     w2 
			// x(n)--->P(0)-->P(1)-->...P(count-1)--->y(n)

			//      w0     w1     w2     w3     w4     w5
			// x(n)--->P(0)-->P(1)-->P(2)-->P(3)-->P(4)-->y(n)

			// call batch processors in sequence - no internal state for batch processing

			// point to first processor

			pprc = &ppset->prcs[0];

			for (int i = 0; i < count; i++)
			{
				pprc->pfnGetNextN (pprc->pdata, pbf, SampleCount, op);
				pprc++;
			}

		return;
		}	
	}
}


// Get next sample from this preset.  called once for every sample in buffer
// ppset is pointer to preset
// x is input sample

inline int PSET_GetNext ( pset_t *ppset, int x )
{
	int *w = ppset->w;
	prc_t *pprc;
	int count = ppset->cprcs;
	
	// initialized 0'th element of state array

	w[0] = x;

	switch ( ppset->type )
	{
	default:
	case PSET_SIMPLE:
		{
			// x(n)--->P(0)--->y(n)

			return ppset->prcs[0].pfnGetNext (ppset->prcs[0].pdata, x);
		}
	case PSET_LINEAR:
		{
			//      w0     w1     w2 
			// x(n)--->P(0)-->P(1)-->...P(count-1)--->y(n)

			//      w0     w1     w2     w3     w4     w5
			// x(n)--->P(0)-->P(1)-->P(2)-->P(3)-->P(4)-->y(n)

			// call processors in reverse order, from count to 1
			
			// point to last processor

			pprc = &ppset->prcs[count-1];

			switch (count)
			{
			default:
			case 5:
				w[5] = pprc->pfnGetNext (pprc->pdata, w[4]);
				pprc--;
			case 4:
				w[4] = pprc->pfnGetNext (pprc->pdata, w[3]);
				pprc--;
			case 3:
				w[3] = pprc->pfnGetNext (pprc->pdata, w[2]);
				pprc--;
			case 2:
				w[2] = pprc->pfnGetNext (pprc->pdata, w[1]);
				pprc--;
			case 1:
				w[1] = pprc->pfnGetNext (pprc->pdata, w[0]);
		}

			//for (int i = count; i > 0; i--, pprc--)
			//	w[i] = pprc->pfnGetNext (pprc->pdata, w[i-1]);

			return w[count];
		}	

	case PSET_PARALLEL6:
		{
			//    w0	w1		w2	   w3	w6      w7
			// x(n)-P(0)-->P(1)-->P(2)-->(+)---P(5)--->y(n)
			//          |				  ^
			//		    |       w4     w5 | 
			//		    -->P(3)-->P(4)---->
			
			pprc = &ppset->prcs[0];
			
			// start with all adders

			w[6] = w[3] + w[5];
			
			// top branch - evaluate in reverse order

			w[7] = pprc[5].pfnGetNext( pprc[5].pdata, w[6] ); 
			w[3] = pprc[2].pfnGetNext( pprc[2].pdata, w[2] );
			w[2] = pprc[1].pfnGetNext( pprc[1].pdata, w[1] );
			
			// bottom branch - evaluate in reverse order

			w[5] = pprc[4].pfnGetNext( pprc[4].pdata, w[4] );
			w[4] = pprc[3].pfnGetNext( pprc[3].pdata, w[1] );
			
			w[1] = pprc[0].pfnGetNext( pprc[0].pdata, w[0] );

			return w[7];
		}
	case PSET_PARALLEL2:
		{	//     w0      w1    w3
			// x(n)--->P(0)-->(+)-->y(n)
			//      	       ^
			//	   w0      w2  | 
			// x(n)--->P(1)-----

			pprc = &ppset->prcs[0];

			w[3] = w[1] + w[2];

			w[1] = pprc->pfnGetNext( pprc->pdata, w[0] );
			pprc++;
			w[2] = pprc->pfnGetNext( pprc->pdata, w[0] );

			return w[3];
		}

	case PSET_PARALLEL4:
		{	//     w0      w1     w2    w5
			// x(n)--->P(0)-->P(1)-->(+)-->y(n)
			//      				  ^
			//	   w0      w3     w4  | 
			// x(n)--->P(2)-->P(3)-----


			pprc = &ppset->prcs[0];

			w[5] = w[2] + w[4];

			w[2] = pprc[1].pfnGetNext( pprc[1].pdata, w[1] );
			w[4] = pprc[3].pfnGetNext( pprc[3].pdata, w[3] );

			w[1] = pprc[0].pfnGetNext( pprc[0].pdata, w[0] );
			w[3] = pprc[2].pfnGetNext( pprc[2].pdata, w[0] );

			return w[5];
		}

	case PSET_PARALLEL5:
		{	//     w0      w1     w2    w5     w6
			// x(n)--->P(0)-->P(1)-->(+)-->P(4)-->y(n)
			//      				  ^
			//	   w0      w3     w4  | 
			// x(n)--->P(2)-->P(3)-----

			pprc = &ppset->prcs[0];

			w[5] = w[2] + w[4];

			w[6] = pprc[4].pfnGetNext( pprc[4].pdata, w[5] );

			w[2] = pprc[1].pfnGetNext( pprc[1].pdata, w[1] );
			w[4] = pprc[3].pfnGetNext( pprc[3].pdata, w[3] );

			w[1] = pprc[0].pfnGetNext( pprc[0].pdata, w[0] );
			w[3] = pprc[2].pfnGetNext( pprc[2].pdata, w[0] );

			return w[6];
		}

	case PSET_FEEDBACK:
		{
			//    w0    w1   w2     w3      w4    w7
			// x(n)-P(0)--(+)-->P(1)-->P(2)-->P(5)->y(n)
			//             ^				|
			//             |  w6     w5     v 
			//		       -----P(4)<--P(3)--

			pprc = &ppset->prcs[0];
			
			// start with adders
			
			w[2] = w[1] + w[6];

			// evaluate in reverse order

			w[7] = pprc[5].pfnGetNext( pprc[5].pdata, w[4] );
			w[6] = pprc[4].pfnGetNext( pprc[4].pdata, w[5] );
			w[5] = pprc[3].pfnGetNext( pprc[3].pdata, w[4] );
			w[4] = pprc[2].pfnGetNext( pprc[2].pdata, w[3] );
			w[3] = pprc[1].pfnGetNext( pprc[1].pdata, w[2] );
			w[1] = pprc[0].pfnGetNext( pprc[0].pdata, w[0] );

			return w[7];
		}
	case PSET_FEEDBACK3:
		{
			//     w0     w1     w2
			// x(n)---(+)-->P(0)--------->y(n)
			//         ^                |
			//         |  w4     w3     v 
			//		   -----P(2)<--P(1)--
			
			pprc = &ppset->prcs[0];
			
			// start with adders
			
			w[1] = w[0] + w[4];

			// evaluate in reverse order

			w[4] = pprc[2].pfnGetNext( pprc[2].pdata, w[3] );
			w[3] = pprc[1].pfnGetNext( pprc[1].pdata, w[2] );
			w[2] = pprc[0].pfnGetNext( pprc[0].pdata, w[1] );
			
			return w[2];
		}
	case PSET_FEEDBACK4:
		{
			//     w0    w1      w2           w5
			// x(n)---(+)-->P(0)-------->P(3)--->y(n)
			//         ^              |
			//         | w4     w3    v 
			//		   ---P(2)<--P(1)--

			pprc = &ppset->prcs[0];
			
			// start with adders
			
			w[1] = w[0] + w[4];

			// evaluate in reverse order

			w[5] = pprc[3].pfnGetNext( pprc[3].pdata, w[2] );
			w[4] = pprc[2].pfnGetNext( pprc[2].pdata, w[3] );
			w[3] = pprc[1].pfnGetNext( pprc[1].pdata, w[2] );
			w[2] = pprc[0].pfnGetNext( pprc[0].pdata, w[1] );
			
			return w[2];
		}
	case PSET_MOD:
		{
			//		w0		  w1    w3     w4
			// x(n)------>P(1)--P(2)--P(3)--->y(n)
			//      w0        w2  ^     
			// x(n)------>P(0)....:

			pprc = &ppset->prcs[0];

			w[4] = pprc[3].pfnGetNext( pprc[3].pdata, w[3] );

			w[3] = pprc[2].pfnGetNext( pprc[2].pdata, w[1] );

			// modulate processor 2

			pprc[2].pfnMod( pprc[2].pdata, ((float)w[2] / (float)PMAX));

			// get modulator output

			w[2] = pprc[0].pfnGetNext( pprc[0].pdata, w[0] );

			w[1] = pprc[1].pfnGetNext( pprc[1].pdata, w[0] );

			return w[4];
		}
	case PSET_MOD2:
		{
			//      w0           w2
			// x(n)---------P(1)-->y(n)
			//      w0    w1  ^     
			// x(n)-->P(0)....:

			pprc = &ppset->prcs[0];

			// modulate processor 1

			pprc[1].pfnMod( pprc[1].pdata, ((float)w[1] / (float)PMAX));

			// get modulator output

			w[1] = pprc[0].pfnGetNext( pprc[0].pdata, w[0] );

			w[2] = pprc[1].pfnGetNext( pprc[1].pdata, w[0] );

			return w[2];

		}
	case PSET_MOD3:
		{
			//      w0           w2      w3
			// x(n)----------P(1)-->P(2)-->y(n)
			//      w0    w1   ^     
			// x(n)-->P(0).....:

			pprc = &ppset->prcs[0];

			w[3] = pprc[2].pfnGetNext( pprc[2].pdata, w[2] );

			// modulate processor 1

			pprc[1].pfnMod( pprc[1].pdata, ((float)w[1] / (float)PMAX));

			// get modulator output

			w[1] = pprc[0].pfnGetNext( pprc[0].pdata, w[0] );

			w[2] = pprc[1].pfnGetNext( pprc[1].pdata, w[0] );

			return w[2];
		}
	}
}


/////////////
// DSP system
/////////////

// Main interface

//     Whenever the preset # changes on any of these processors, the old processor is faded out, new is faded in.
//     dsp_chan is optionally set when a sound is played - a preset is sent with the start_static/dynamic sound.
//  
// sound1---->dsp_chan-->  -------------(+)---->dsp_water--->dsp_player--->out
// sound2---->dsp_chan-->  |             |
// sound3--------------->  ----dsp_room---
//                         |             |		 
//                         --dsp_indirect-

//  dsp_room	- set this cvar to a preset # to change the room dsp.  room fx are more prevalent farther from player.
//					use: when player moves into a new room, all sounds played in room take on its reverberant character
//  dsp_water	- set this cvar (once) to a preset # for serial underwater sound.
//					use: when player goes under water, all sounds pass through this dsp (such as low pass filter)
//	dsp_player	- set this cvar to a preset # to cause all sounds to run through the effect (serial, in-line).
//					use: player is deafened, player fires special weapon, player is hit by special weapon.
//  dsp_facingaway- set this cvar to a preset # appropriate for sounds which are played facing away from player (weapon,voice)

// Dsp presets

ConVar dsp_room			("dsp_room", "0");				// room dsp preset - sounds more distant from player (1ch)
ConVar dsp_water		("dsp_water", "0");				// "15" underwater dsp preset - sound when underwater (1-2ch)
ConVar dsp_player		("dsp_player", "0");			// dsp on player - sound when player hit by special device (1-2ch)
ConVar dsp_facingaway	("dsp_facingaway", "0");		// "30" sounds that face away from player (weapons, voice) (1-4ch)

int ipset_room_prev;
int ipset_water_prev;
int ipset_player_prev;
int ipset_facingaway_prev;

// legacy room_type support

ConVar dsp_room_type		( "room_type", "0" );
int  ipset_room_typeprev;


// DSP processors

int idsp_room;
int idsp_water;
int idsp_player;
int idsp_facingaway;

// Set to "1" to deactivate all dsp processing

ConVar dsp_off		("dsp_off", "0");						// set to 1 to disable all dsp processing
ConVar snd_profile	("snd_profile", "0");					// 1 - profile dsp, 2 - mix, 3 - load sound, 4 - all sound
ConVar dsp_stereo	( "dsp_stereo", "0", FCVAR_ARCHIVE );	// set to 1 for true stereo processing.  2x perf hit.

int dsp_hires = 0;
int dsp_hiresprev = 0;


// DSP preset executor

#define CDSPS		32				// max number dsp executors active
#define DSPCHANMAX	4				// max number of channels dsp can process (allocs a separte processor for each chan)

struct dsp_t
{
	bool fused;
	int cchan;						// 1-4 channels, ie: mono, FrontLeft, FrontRight, RearLeft, RearRight

	pset_t *ppset[DSPCHANMAX];		// current preset (1-4 channels)
	int ipset;						// current ipreset

	pset_t *ppsetprev[DSPCHANMAX];	// previous preset (1-4 channels)
	int ipsetprev;					// previous ipreset
	
	float xfade;					// crossfade time between previous preset and new
	rmp_t xramp;					// crossfade ramp
};

dsp_t dsps[CDSPS];

void DSP_Init( int idsp ) 
{ 
	dsp_t *pdsp;

	if (idsp < 0 || idsp > CDSPS)
		return;
	
	pdsp = &dsps[idsp];

	Q_memset( pdsp, 0, sizeof (dsp_t) ); 
}

void DSP_Free( int idsp ) 
{
	dsp_t *pdsp;

	if (idsp < 0 || idsp > CDSPS)
		return;
	
	pdsp = &dsps[idsp];

	for (int i = 0; i < pdsp->cchan; i++)
	{
		if ( pdsp->ppset[i] )
			PSET_Free( pdsp->ppset[i] );
		
		if ( pdsp->ppsetprev[i] )
			PSET_Free( pdsp->ppsetprev[i] );
	}

	Q_memset( pdsp, 0, sizeof (dsp_t) ); 
}

// Init all dsp processors - called once, during engine startup

void DSP_InitAll ( void )
{
	// order is important, don't rearange.

	FLT_InitAll();
	DLY_InitAll();
	RVA_InitAll();
	LFOWAV_InitAll();
	LFO_InitAll();
	
	CRS_InitAll();
	PTC_InitAll();
	ENV_InitAll();
	EFO_InitAll();
	MDY_InitAll();
	AMP_InitAll();

	PSET_InitAll();

	for (int idsp = 0; idsp < CDSPS; idsp++) 
		DSP_Init( idsp );

	// UNDONE: define what hisound/dsp_hires means ie: no dsp? no 44khz wavs?

	dsp_hires = (hisound.GetFloat() == 0 ? 0 : 1);
	dsp_hiresprev = dsp_hires;
}

// free all resources associated with dsp - called once, during engine shutdown

void DSP_FreeAll (void)
{
	// order is important, don't rearange.

	for (int idsp = 0; idsp < CDSPS; idsp++) 
			DSP_Free( idsp );

	AMP_FreeAll();
	MDY_FreeAll();
	EFO_FreeAll();
	ENV_FreeAll();
	PTC_FreeAll();
	CRS_FreeAll();
	
	LFO_FreeAll();
	LFOWAV_FreeAll();
	RVA_FreeAll();
	DLY_FreeAll();
	FLT_FreeAll();
}


// allocate a new dsp processor chain, kill the old processor.  Called by DSP_CheckNewPreset()
// ipset is new preset 
// xfade is crossfade time when switching between presets (milliseconds)
// cchan is how many simultaneous preset channels to allocate (1-4)
// return index to new dsp

int DSP_Alloc( int ipset, float xfade, int cchan )
{
	dsp_t *pdsp;
	int i;
	int idsp;
	int cchans = clamp( cchan, 1, DSPCHANMAX);

	// find free slot

	for ( idsp = 0; idsp < CDSPS; idsp++ )
	{
		if ( !dsps[idsp].fused )
			break;
	}

	if ( idsp == CDSPS ) 
		return -1;

	pdsp = &dsps[idsp];

	DSP_Init ( idsp );
	
	pdsp->fused = true;

	pdsp->cchan = cchans;

	// allocate a preset processor for each channel

	pdsp->ipset = ipset;
	pdsp->ipsetprev = 0;
	
	for (i = 0; i < pdsp->cchan; i++)
	{
		pdsp->ppset[i] = PSET_Alloc ( ipset );
		pdsp->ppsetprev[i] = NULL;
	}

	// set up crossfade time in seconds

	pdsp->xfade = xfade / 1000.0;				
	
	RMP_SetEnd(&pdsp->xramp);

	return idsp;
}

// return gain for current preset associated with dsp
// get crossfade to new gain if switching from previous preset (from preset crossfader value)
// Returns 1.0 gain if no preset (preset 0)

float DSP_GetGain( int idsp )
{
	float gain_target = 0.0;
	float gain_prev = 0.0;
	float gain;
	dsp_t *pdsp;
	
	if (idsp < 0 || idsp > CDSPS)
		return 1.0;
	
	pdsp = &dsps[idsp];

	// get current preset's gain

	if ( pdsp->ppset[0] )
		gain_target = pdsp->ppset[0]->gain;
	else
		gain_target = 1.0;

	// if not crossfading, return current preset gain

	if ( RMP_HitEnd( &pdsp->xramp ) )
	{
		// return current preset's gain

		return gain_target;
	}
	
	// get previous preset gain

	if ( pdsp->ppsetprev[0] )
		gain_prev = pdsp->ppsetprev[0]->gain;
	else
		gain_prev = 1.0;

	// if current gain = target preset gain, return

	if ( gain_target == gain_prev )
	{
		if ( gain_target == 0.0 )
			return 1.0;

		return gain_target;
	}

	// get crossfade ramp value (updated elsewhere, when actually crossfading preset data)

	int r = RMP_GetCurrent( &pdsp->xramp );	

	// crossfade from previous to current preset gain

	if (gain_target > gain_prev)
	{
		// ramping gain up - ramp up gain to target in last 10% of ramp

		float rf = (float) r;
		float pmax = (float)PMAX;

		rf = rf / pmax;		// rf 0->1.0

		if ( rf < 0.9)
			rf = 0.0;
		else
			rf = (rf - 0.9) / (1.0 - 0.9);	// 0->1.0 after rf > 0.9

		// crossfade gain from prev to target over rf

		gain = gain_prev + (gain_target - gain_prev) * rf;

		return gain;
	}
	else
	{
		// ramping gain down - drop gain to target in first 10% of ramp

		float rf = (float) r;
		float pmax = (float)PMAX;

		rf = rf / pmax;		// rf 0.0->1.0

		if ( rf < 0.1)
			rf = (rf - 0.1) / (0.0 - 0.1);	// 1.0->0.0 if rf < 0.1
		else
			rf = 0.0;	

		// crossfade gain from prev to target over rf

		gain = gain_prev + (gain_target - gain_prev) * (1.0 - rf);

		return gain;
	}
}

// free previous preset if not 0

inline void DSP_FreePrevPreset( dsp_t *pdsp )
{
	// free previous presets if non-null - ie: rapid change of preset just kills old without xfade

	if ( pdsp->ipsetprev )
	{
		for (int i = 0; i < pdsp->cchan; i++)
		{
			if ( pdsp->ppsetprev[i] )
			{
				PSET_Free( pdsp->ppsetprev[i] );
				pdsp->ppsetprev[i] = NULL;
			}
		}

		pdsp->ipsetprev = 0;
	}

}


// alloc new preset if different from current
//		xfade from prev to new preset
//		free previous preset, copy current into previous, set up xfade from previous to new

void DSP_SetPreset( int idsp, int ipsetnew)
{
	dsp_t *pdsp;
	pset_t *ppsetnew[DSPCHANMAX];

	Assert (idsp >= 0 && idsp < CDSPS);

	pdsp = &dsps[idsp];

	// validate new preset range

	if ( ipsetnew >=  CPSETTEMPLATES || ipsetnew < 0 )
		return;

	// ignore if new preset is same as current preset

	if ( ipsetnew == pdsp->ipset )
		return;

	// alloc new presets (each channel is a duplicate preset)
	
	Assert (pdsp->cchan <= DSPCHANMAX);

	for (int i = 0; i < pdsp->cchan; i++)
	{
		ppsetnew[i] = PSET_Alloc ( ipsetnew );
		if ( !ppsetnew[i] )
		{
			DevMsg("WARNING: DSP preset failed to allocate.\n");
			return;
		}
	}

	Assert (pdsp);

	// free PREVIOUS previous preset if not 0

	DSP_FreePrevPreset( pdsp );

	for (i = 0; i < pdsp->cchan; i++)
	{
		// current becomes previous

		pdsp->ppsetprev[i] = pdsp->ppset[i];
		
		// new becomes current

		pdsp->ppset[i] = ppsetnew[i];
	}
	
	pdsp->ipsetprev = pdsp->ipset;
	pdsp->ipset = ipsetnew;

	// clear ramp

	RMP_SetEnd( &pdsp->xramp );
	
	// make sure previous dsp preset has data

	Assert (pdsp->ppsetprev[0]);

	// shouldn't be crossfading if current dsp preset == previous dsp preset

	Assert (pdsp->ipset != pdsp->ipsetprev);

	RMP_Init( &pdsp->xramp, pdsp->xfade, 0, PMAX);
}

///////////////////////////////////////
// Helpers: called only from DSP_Process
///////////////////////////////////////


// return true if batch processing version of preset exists

inline bool FBatchPreset( pset_t *ppset )
{
	
	switch (ppset->type)
	{
	case PSET_LINEAR: 
		return true;
	case PSET_SIMPLE: 
		return true;
	default:
		return false;
	}
}

// Helper: called only from DSP_Process
// mix front stereo buffer to mono buffer, apply dsp fx

inline void DSP_ProcessStereoToMono(dsp_t *pdsp, portable_samplepair_t *pbfront, portable_samplepair_t *pbrear, int sampleCount, bool bcrossfading )
{
	portable_samplepair_t *pbf = pbfront;		// pointer to buffer of front stereo samples to process
	int count = sampleCount;
	int av;
	int x;

	if ( !bcrossfading ) 
	{
		if (FBatchPreset(pdsp->ppset[0]))
		{
			// convert Stereo to Mono in place, then batch process fx: perf KDB

			// front->left + front->right / 2 into front->left, front->right duplicated.

			while ( count-- )
			{
				pbf->left = (pbf->left + pbf->right) >> 1;
				pbf++;
			}
			
			// process left (mono), duplicate output into right

			PSET_GetNextN( pdsp->ppset[0], pbfront, sampleCount, OP_LEFT_DUPLICATE);
		}
		else
		{
			// avg left and right -> mono fx -> duplcate out left and right
			while ( count-- )
			{
				av = ( ( pbf->left + pbf->right ) >> 1 );
				x = PSET_GetNext( pdsp->ppset[0], av );
				x = CLIP_DSP( x );
				pbf->left = pbf->right = x;
				pbf++;
			}
		}
		return;
	}
	// crossfading to current preset from previous preset	

	if ( bcrossfading )
	{
		int r = -1;
		int fl;
		int flp;
		int xf_fl;
		
		while ( count-- )
		{
			av = ( ( pbf->left + pbf->right ) >> 1 );

			// get current preset values

			fl = PSET_GetNext( pdsp->ppset[0], av );
			
			// get previous preset values

			flp = PSET_GetNext( pdsp->ppsetprev[0], av );
			
			fl = CLIP_DSP(fl);
			flp = CLIP_DSP(flp);

			// get current ramp value

			r = RMP_GetNext( &pdsp->xramp );	

			// crossfade from previous to current preset

			xf_fl = XFADE(fl, flp, r);	// crossfade front left previous to front left
			
			pbf->left =  xf_fl;			// crossfaded front left, duplicate in right channel
			pbf->right = xf_fl;
		
			pbf++;

		}

	}
}

// Helper: called only from DSP_Process
// DSP_Process stereo in to stereo out (if more than 2 procs, ignore them)

inline void DSP_ProcessStereoToStereo(dsp_t *pdsp, portable_samplepair_t *pbfront, portable_samplepair_t *pbrear, int sampleCount, bool bcrossfading )
{
	portable_samplepair_t *pbf = pbfront;		// pointer to buffer of front stereo samples to process
	int count = sampleCount;
	int fl, fr;

	if ( !bcrossfading ) 
	{

		if ( FBatchPreset(pdsp->ppset[0]) && FBatchPreset(pdsp->ppset[1]) )
		{

			// process left & right

			PSET_GetNextN( pdsp->ppset[0], pbfront, sampleCount, OP_LEFT );
			PSET_GetNextN( pdsp->ppset[1], pbfront, sampleCount, OP_RIGHT );
		}
		else
		{
			// left -> left fx, right -> right fx
			while ( count-- )
			{
				fl = PSET_GetNext( pdsp->ppset[0], pbf->left );
				fr = PSET_GetNext( pdsp->ppset[1], pbf->right );

				fl = CLIP_DSP( fl );
				fr = CLIP_DSP( fr );

				pbf->left =  fl;
				pbf->right = fr;
				pbf++;
			}
		}
		return;
	}

	// crossfading to current preset from previous preset	

	if ( bcrossfading )
	{
		int r;
		int flp, frp;
		int xf_fl, xf_fr;

		while ( count-- )
		{
			// get current preset values

			fl = PSET_GetNext( pdsp->ppset[0], pbf->left );
			fr = PSET_GetNext( pdsp->ppset[1], pbf->right );
	
			// get previous preset values

			flp = PSET_GetNext( pdsp->ppsetprev[0], pbf->left );
			frp = PSET_GetNext( pdsp->ppsetprev[1], pbf->right );
		
			// get current ramp value

			r = RMP_GetNext( &pdsp->xramp );	

			fl = CLIP_DSP( fl );
			fr = CLIP_DSP( fr );
			flp = CLIP_DSP( flp );
			frp = CLIP_DSP( frp );

			// crossfade from previous to current preset

			xf_fl = XFADE(fl, flp, r);	// crossfade front left previous to front left
			xf_fr = XFADE(fr, frp, r);
			
			pbf->left =  xf_fl;			// crossfaded front left
			pbf->right = xf_fr;
		
			pbf++;
		}
	}
}

// Helper: called only from DSP_Process
// DSP_Process quad in to mono out

inline void DSP_ProcessQuadToMono(dsp_t *pdsp, portable_samplepair_t *pbfront, portable_samplepair_t *pbrear, int sampleCount, bool bcrossfading )
{
	portable_samplepair_t *pbf = pbfront;		// pointer to buffer of front stereo samples to process
	portable_samplepair_t *pbr = pbrear;		// pointer to buffer of rear stereo samples to process
	int count = sampleCount;
	int x;
	int av;

	if ( !bcrossfading ) 
	{
		if ( FBatchPreset(pdsp->ppset[0]) )
		{

			// convert Quad to Mono in place, then batch process fx: perf KDB

			// left front + rear -> left, right front + rear -> right
			while ( count-- )
			{
				pbf->left = ((pbf->left + pbf->right + pbr->left + pbr->right) >> 2);
				pbf++;
				pbr++;
			}
			
			// process left (mono), duplicate into right

			PSET_GetNextN( pdsp->ppset[0], pbfront, sampleCount, OP_LEFT_DUPLICATE);

			// copy processed front to rear

			count = sampleCount;
			
			pbf = pbfront;
			pbr = pbrear;

			while ( count-- )
			{
				pbr->left = pbf->left;
				pbr->right = pbf->right;
				pbf++;
				pbr++;
			}

		}
		else
		{
			// avg fl,fr,rl,rr into mono fx, duplicate on all channels
			while ( count-- )
			{
				av = ((pbf->left + pbf->right + pbr->left + pbr->right) >> 2);
				x = PSET_GetNext( pdsp->ppset[0], av );
				x = CLIP_DSP( x );
				pbr->left = pbr->right = pbf->left = pbf->right = x;
				pbf++;
				pbr++;
			}
		}
			return;
	}

	if ( bcrossfading )
	{
		int r;
		int fl;
		int flp;
		int xf_fl;

		while ( count-- )
		{
			
			av = ((pbf->left + pbf->right + pbr->left + pbr->right) >> 2);

			// get current preset values

			fl = PSET_GetNext( pdsp->ppset[0], av );
			
			// get previous preset values

			flp = PSET_GetNext( pdsp->ppsetprev[0], av );
			
			// get current ramp value

			r = RMP_GetNext( &pdsp->xramp );	

			fl = CLIP_DSP( fl );
			flp = CLIP_DSP( flp );

			// crossfade from previous to current preset

			xf_fl = XFADE(fl, flp, r);	// crossfade front left previous to front left
			
			pbf->left =  xf_fl;			// crossfaded front left, duplicated to all channels
			pbf->right = xf_fl;
			pbr->left =  xf_fl;			
			pbr->right = xf_fl;

			pbf++;
			pbr++;
		}
	}
}

// Helper: called only from DSP_Process
// DSP_Process quad in to stereo out (preserve stereo spatialization, throw away front/rear)

inline void DSP_ProcessQuadToStereo(dsp_t *pdsp, portable_samplepair_t *pbfront, portable_samplepair_t *pbrear, int sampleCount, bool bcrossfading )
{
	portable_samplepair_t *pbf = pbfront;		// pointer to buffer of front stereo samples to process
	portable_samplepair_t *pbr = pbrear;		// pointer to buffer of rear stereo samples to process
	int count = sampleCount;
	int fl, fr;
	
	if ( !bcrossfading ) 
	{
		if ( FBatchPreset(pdsp->ppset[0]) && FBatchPreset(pdsp->ppset[1]) )
		{

			// convert Quad to Stereo in place, then batch process fx: perf KDB

			// left front + rear -> left, right front + rear -> right

			while ( count-- )
			{
				pbf->left =  (pbf->left + pbr->left) >> 1;
				pbf->right = (pbf->right + pbr->right) >> 1;
				pbf++;
				pbr++;
			}
			
			// process left & right

			PSET_GetNextN( pdsp->ppset[0], pbfront, sampleCount, OP_LEFT);
			PSET_GetNextN( pdsp->ppset[1], pbfront, sampleCount, OP_RIGHT );

			// copy processed front to rear

			count = sampleCount;
			
			pbf = pbfront;
			pbr = pbrear;

			while ( count-- )
			{
				pbr->left = pbf->left;
				pbr->right = pbf->right;
				pbf++;
				pbr++;
			}

		}	
		else
		{
			// left front + rear -> left fx, right front + rear -> right fx
			while ( count-- )
			{
				fl = PSET_GetNext( pdsp->ppset[0], (pbf->left + pbr->left) >> 1);
				fr = PSET_GetNext( pdsp->ppset[1], (pbf->right + pbr->right) >> 1);
				fl = CLIP_DSP( fl );
				fr = CLIP_DSP( fr );

				pbr->left =  pbf->left =  fl;
				pbr->right = pbf->right = fr;
				pbf++;
				pbr++;
			}
		}
		return;
	}

	// crossfading to current preset from previous preset	

	if ( bcrossfading )
	{
		int r;
		int flp, frp;
		int avl, avr;
		int xf_fl, xf_fr;
	
		while ( count-- )
		{
			avl = (pbf->left + pbr->left) >> 1;
			avr = (pbf->right + pbr->right) >> 1;

			// get current preset values

			fl = PSET_GetNext( pdsp->ppset[0], avl );
			fr = PSET_GetNext( pdsp->ppset[1], avr );
			
			// get previous preset values

			flp = PSET_GetNext( pdsp->ppsetprev[0], avl );
			frp = PSET_GetNext( pdsp->ppsetprev[1], avr );
			

			fl = CLIP_DSP( fl );
			fr = CLIP_DSP( fr );
			
			// get previous preset values

			flp = CLIP_DSP( flp );
			frp = CLIP_DSP( frp );

			// get current ramp value

			r = RMP_GetNext( &pdsp->xramp );	

			// crossfade from previous to current preset

			xf_fl = XFADE(fl, flp, r);	// crossfade front left previous to front left
			xf_fr = XFADE(fr, frp, r);
			
			pbf->left =  xf_fl;			// crossfaded front left
			pbf->right = xf_fr;

			pbr->left =  xf_fl;			// duplicate front channel to rear channel
			pbr->right = xf_fr;

			pbf++;
			pbr++;
		}
	}
}

// Helper: called only from DSP_Process
// DSP_Process quad in to quad out

inline void DSP_ProcessQuadToQuad(dsp_t *pdsp, portable_samplepair_t *pbfront, portable_samplepair_t *pbrear, int sampleCount, bool bcrossfading )
{
	portable_samplepair_t *pbf = pbfront;		// pointer to buffer of front stereo samples to process
	portable_samplepair_t *pbr = pbrear;		// pointer to buffer of rear stereo samples to process
	int count = sampleCount;
	int fl, fr, rl, rr;

	if ( !bcrossfading ) 
	{
		// each channel gets its own processor

		if ( FBatchPreset(pdsp->ppset[0]) && FBatchPreset(pdsp->ppset[1]) && FBatchPreset(pdsp->ppset[2]) && FBatchPreset(pdsp->ppset[3]))
		{	
			// batch process fx front & rear, left & right: perf KDB

			PSET_GetNextN( pdsp->ppset[0], pbfront, sampleCount, OP_LEFT);
			PSET_GetNextN( pdsp->ppset[1], pbfront, sampleCount, OP_RIGHT );
			PSET_GetNextN( pdsp->ppset[2], pbrear,  sampleCount, OP_LEFT );
			PSET_GetNextN( pdsp->ppset[3], pbrear,  sampleCount, OP_RIGHT );
		}	
		else
		{
			while ( count-- )
			{
				fl = PSET_GetNext( pdsp->ppset[0], pbf->left );
				fr = PSET_GetNext( pdsp->ppset[1], pbf->right );
				rl = PSET_GetNext( pdsp->ppset[2], pbr->left );
				rr = PSET_GetNext( pdsp->ppset[3], pbr->right );
				
				pbf->left =  CLIP_DSP( fl );
				pbf->right = CLIP_DSP( fr );
				pbr->left =  CLIP_DSP( rl );
				pbr->right = CLIP_DSP( rr );

				pbf++;
				pbr++;
			}
		}
		return;
	}

	// crossfading to current preset from previous preset	

	if ( bcrossfading )
	{
		int r;
		int fl, fr, rl, rr;
		int flp, frp, rlp, rrp;
		int xf_fl, xf_fr, xf_rl, xf_rr;

		while ( count-- )
		{
			// get current preset values

			fl = PSET_GetNext( pdsp->ppset[0], pbf->left );
			fr = PSET_GetNext( pdsp->ppset[1], pbf->right );
			rl = PSET_GetNext( pdsp->ppset[2], pbr->left );
			rr = PSET_GetNext( pdsp->ppset[3], pbr->right );

			// get previous preset values

			flp = PSET_GetNext( pdsp->ppsetprev[0], pbf->left );
			frp = PSET_GetNext( pdsp->ppsetprev[1], pbf->right );
			rlp = PSET_GetNext( pdsp->ppsetprev[2], pbr->left );
			rrp = PSET_GetNext( pdsp->ppsetprev[3], pbr->right );

			// get current ramp value

			r = RMP_GetNext( &pdsp->xramp );	

			// crossfade from previous to current preset

			xf_fl = XFADE(fl, flp, r);	// crossfade front left previous to front left
			xf_fr = XFADE(fr, frp, r);
			xf_rl = XFADE(rl, rlp, r);
			xf_rr = XFADE(rr, rrp, r);
			
			pbf->left =  CLIP_DSP(xf_fl);			// crossfaded front left
			pbf->right = CLIP_DSP(xf_fr);
			pbr->left =  CLIP_DSP(xf_rl);
			pbr->right = CLIP_DSP(xf_rr);

			pbf++;
			pbr++;
		}
	}
}

void DSP_ClearState()
{
	dsp_room.SetValue( 0 );
	dsp_water.SetValue( 0 );
	dsp_player.SetValue( 0 );
	dsp_facingaway.SetValue( 0 );
	CheckNewDspPresets();
	dsp_t *pdsp = &dsps[0];
	pdsp->xramp.fhitend = true;	// don't crossfade
}

// Main DSP processing routine:
// process samples in buffers using pdsp processor
// continue crossfade between 2 dsp processors if crossfading on switch
// pfront - front stereo buffer to process
// prear - rear stereo buffer to process (may be NULL)
// sampleCount - number of samples in pbuf to process
// This routine also maps the # processing channels in the pdsp to the number of channels 
// supplied.  ie: if the pdsp has 4 channels and pbfront and pbrear are both non-null, the channels
// map 1:1 through the processors.

void DSP_Process( int idsp, portable_samplepair_t *pbfront, portable_samplepair_t *pbrear, int sampleCount )
{
	bool bcrossfading;
	int cchan_in;								// input channels (2 or 4)
	int cprocs;									// output cannels (1, 2 or 4)
	dsp_t *pdsp;

	if (idsp < 0 || idsp >= CDSPS)
		return;

	Assert ( idsp < CDSPS );					// make sure idsp is valid

	pdsp = &dsps[idsp];

	// if current and previous preset 0, return - preset 0 is 'off'

	if ( !pdsp->ipset && !pdsp->ipsetprev )
		return;

	Assert (pbfront);

	// return right away if fx processing is turned off

	if ( dsp_off.GetInt() )
		return;

	if ( sampleCount < 0 )
		return;
	
	bcrossfading = !RMP_HitEnd( &pdsp->xramp );

	// if not crossfading, and previous channel is not null, free previous
	
	if ( !bcrossfading )
		DSP_FreePrevPreset( pdsp );
	

	cchan_in = (pbrear ? 4 : 2);
	cprocs = pdsp->cchan;
	
	// NOTE: when mixing between different channel sizes, 
	// always AVERAGE down to fewer channels and DUPLICATE up more channels.
	// The following routines always process cchan_in channels. 
	// ie: QuadToMono still updates 4 values in buffer

	// DSP_Process stereo in to mono out (ie: left and right are averaged)

	if ( cchan_in == 2 && cprocs == 1)
	{
		DSP_ProcessStereoToMono( pdsp, pbfront, pbrear, sampleCount, bcrossfading );
		return;
	}

	// DSP_Process stereo in to stereo out (if more than 2 procs, ignore them)

	if ( cchan_in == 2 && cprocs >= 2)
	{
		DSP_ProcessStereoToStereo( pdsp, pbfront, pbrear, sampleCount, bcrossfading );
		return;
	}


	// DSP_Process quad in to mono out

	if ( cchan_in == 4 && cprocs == 1)
	{
		DSP_ProcessQuadToMono( pdsp, pbfront, pbrear, sampleCount, bcrossfading );
		return;
	}


	// DSP_Process quad in to stereo out (preserve stereo spatialization, loose front/rear)

	if ( cchan_in == 4 && cprocs == 2)
	{
		DSP_ProcessQuadToStereo( pdsp, pbfront, pbrear, sampleCount, bcrossfading );
		return;
	}


	// DSP_Process quad in to quad out

	if ( cchan_in == 4 && cprocs == 4)
	{
		DSP_ProcessQuadToQuad( pdsp, pbfront, pbrear, sampleCount, bcrossfading );
		return;
	}

}

// DSP helpers

// free all dsp processors 

void FreeDsps( void )
{

	DSP_Free(idsp_room);
	DSP_Free(idsp_water);
	DSP_Free(idsp_player);
	DSP_Free(idsp_facingaway);
	
	idsp_room = 0;
	idsp_water = 0;
	idsp_player = 0;
	idsp_facingaway = 0;
	
	DSP_FreeAll();
}

// alloc dsp processors

bool AllocDsps( void )
{
	int csurround = (SURROUND_ON ? 2: 0);		// surround channels to allocate

	DSP_InitAll();

	idsp_room = -1.0;
	idsp_water = -1.0;
	idsp_player = -1.0;
	idsp_facingaway = -1.0;
	
	// alloc dsp room channel (mono, stereo if dsp_stereo is 1)
	
	// dsp room is mono, 300ms fade time

	idsp_room = DSP_Alloc( dsp_room.GetInt(), 300, dsp_stereo.GetInt() * 2 );

	// alloc stereo or quad series processors for player or water
	
	// UNDONE: performance not yet good enough for quad surround on 1mhz machines

	//idsp_water = DSP_Alloc( dsp_water.GetInt(), 100, 2 + csurround );
	//idsp_player = DSP_Alloc( dsp_player.GetInt(), 1000, 2 + csurround );
	
	idsp_water = DSP_Alloc( dsp_water.GetInt(), 100, 1 );
	idsp_player = DSP_Alloc( dsp_player.GetInt(), 1000, 1 );

	// alloc facing away filters (stereo or quad)

	idsp_facingaway = DSP_Alloc( dsp_facingaway.GetInt(), 200, 2 + csurround );

	// init prev values

	ipset_room_prev			= dsp_room.GetInt();
	ipset_water_prev		= dsp_water.GetInt();
	ipset_player_prev		= dsp_player.GetInt();
	ipset_facingaway_prev	= dsp_facingaway.GetInt();
	ipset_room_typeprev		= dsp_room_type.GetInt();

	if (idsp_room < 0 || idsp_water < 0 || idsp_player < 0 || idsp_facingaway < 0 )
	{
		DevMsg ("WARNING: DSP processor failed to initialize! \n" );

		FreeDsps();
		return false;
	}
	
	return true;
}


// Helper to check for change in preset of any of 4 processors
// if switching to a new preset, alloc new preset, simulate both presets in DSP_Process & xfade,

void CheckNewDspPresets( void )
{
	int iroom			= dsp_room.GetInt();
	int iwater			= dsp_water.GetInt();
	int iplayer			= dsp_player.GetInt();
	int ifacingaway		= dsp_facingaway.GetInt();
	int iroomtype		= dsp_room_type.GetInt();
	

	// legacy code support for "room_type" Cvar

	if ( iroomtype != ipset_room_typeprev )
	{
		// force dsp_room = room_type
		
		ipset_room_typeprev = iroomtype;
		dsp_room.SetValue(iroomtype);
	}

	if ( iroom != ipset_room_prev )
	{
		DSP_SetPreset( idsp_room, iroom );
		ipset_room_prev = iroom;

		// force room_type = dsp_room

		dsp_room_type.SetValue(iroom);
		ipset_room_typeprev = iroom;
	}

	if ( iwater != ipset_water_prev )
	{
		DSP_SetPreset( idsp_water, iwater );
		ipset_water_prev = iwater;
	}

	if ( iplayer != ipset_player_prev )
	{
		DSP_SetPreset( idsp_player, iplayer );
		ipset_player_prev = iplayer;
	}

	if ( ifacingaway != ipset_facingaway_prev )
	{
		DSP_SetPreset( idsp_facingaway, ifacingaway );
		ipset_facingaway_prev = ifacingaway;
	}

}



// E3: gated one-shot, gate by db of original signal in channel_t, or by absolute volume
// E3: 'panner' - pan fx left<->right, front<->back, circle, at mod frequency. ie: large echo fx



// NEXT: check outputs of modulators - normalize to +/- 0-1.0 and all inputs normalized to same
// NEXT: performance tune
// NEXT: rename base routines
// NEXT: rva may use mda if lfo params set
// NEXT: perf 
//		- chained processor cost high? test with nulls and with release build
//		- all getnext funcitons take samplecount and pbuffer!
//		- use globals to cut down on params to function calls
//		- note: release build is  more than 2x faster due to inline calls
//		- note: mmx for buffer mixing, multiplying etc
// NEXT: stereoize delays - alternate taps with l/r as if sound is reflecting
// NEXT: filter only rva output

// NEXT: test ptc, crs, flt, env, efo
// NEXT: add MDY to RVA
// NEXT: add clipper/distorter, amplifier, noise generator LFO

// NEXT: stereoize all room sounds using l/r roomsize delays
// NEXT: stereo delay using l/r/u/d/f/b dimensions of room and wall materials
// NEXT: spatialize all sounds based on inter-ear time delay (headphone mode)
// NEXT: filter all sounds based on inter-ear filter (headphone mode)



//===============================================================================
//
// Digital Signal Processing algorithms for audio FX.
//
// KellyB 1/24/97
//===============================================================================


#define SXDLY_MAX		0.400							// max delay in seconds
#define SXRVB_MAX		0.100							// max reverb reflection time
#define SXSTE_MAX		0.100							// max stereo delay line time

typedef int sample_t;									// delay lines must be 32 bit, now that we have 16 bit samples

typedef struct dlyline_s {
	int cdelaysamplesmax;								// size of delay line in samples
	int lp;												// lowpass flag 0 = off, 1 = on

	int idelayinput;									// i/o indices into circular delay line
	int idelayoutput;

	int idelayoutputxf;									// crossfade output pointer
	int xfade;											// crossfade value

	int delaysamples;									// current delay setting
	int delayfeed;										// current feedback setting

	int lp0, lp1, lp2, lp3, lp4, lp5;					// lowpass filter buffer

	int mod;											// sample modulation count
	int modcur;			

	HANDLE hdelayline;									// handle to delay line buffer
	sample_t *lpdelayline;								// buffer
} dlyline_t;

#define CSXDLYMAX		4

#define ISXMONODLY		0								// mono delay line
#define ISXRVB			1								// first of the reverb delay lines
#define CSXRVBMAX		2
#define ISXSTEREODLY	3								// 50ms left side delay

dlyline_t rgsxdly[CSXDLYMAX];							// array of delay lines

#define gdly0 (rgsxdly[ISXMONODLY])
#define gdly1 (rgsxdly[ISXRVB])
#define gdly2 (rgsxdly[ISXRVB + 1])
#define gdly3 (rgsxdly[ISXSTEREODLY])

#define CSXLPMAX		10								// lowpass filter memory

int rgsxlp[CSXLPMAX];		
					
int sxamodl, sxamodr;									// amplitude modulation values
int sxamodlt, sxamodrt;									// modulation targets
int sxmod1, sxmod2;
int sxmod1cur, sxmod2cur;

// Mono Delay parameters

ConVar sxdly_delay      ( "room_delay", "0" );			// current delay in seconds
ConVar sxdly_feedback   ( "room_feedback", "0.2" );		// cyles
ConVar sxdly_lp			( "room_dlylp", "1" );			// lowpass filter

float sxdly_delayprev;									// previous delay setting value

// Mono Reverb parameters

ConVar sxrvb_size		( "room_size", "0" );			// room size 0 (off) 0.1 small - 0.35 huge
ConVar sxrvb_feedback	( "room_refl", "0.7" );			// reverb decay 0.1 short - 0.9 long
ConVar sxrvb_lp			( "room_rvblp", "1" );			// lowpass filter		

float sxrvb_sizeprev;

// Stereo delay (no feedback)

ConVar sxste_delay		( "room_left", "0" );			// straight left delay
float sxste_delayprev;

// Underwater/special fx modulations

ConVar sxmod_lowpass	( "room_lp", "0" );
ConVar sxmod_mod		( "room_mod", "0" );

// Main interface

ConVar sxroom_type		( "room_type_l", "0" );			// legacy support
ConVar sxroomwater_type ( "waterroom_type_l", "14" );	// legacy support
float sxroom_typeprev;

ConVar sxroom_off		( "room_off_l", "0" );			// legacy support

int sxhires = 0;
int sxhiresprev = 0;

qboolean SXDLY_Init(int idelay, float delay);
void SXDLY_Free(int idelay);
void SXDLY_DoDelay(int count);
void SXRVB_DoReverb(int count);
void SXDLY_DoStereoDelay(int count);
void SXRVB_DoAMod(int count);

//=====================================================================
// Init/release all structures for sound effects
//=====================================================================

void SX_Init(void)
{

	Q_memset(rgsxdly, 0, sizeof (dlyline_t) * CSXDLYMAX);
	Q_memset(rgsxlp, 0, sizeof(int) * CSXLPMAX);

	sxdly_delayprev = -1.0;
	sxrvb_sizeprev = -1.0;
	sxste_delayprev = -1.0;
	sxroom_typeprev = -1.0;

	// flag hires sound mode
	sxhires = (hisound.GetFloat() == 0 ? 0 : 1);
	sxhiresprev = sxhires;

	// init amplitude modulation params

	sxamodl = sxamodr = 255;
	sxamodlt = sxamodrt = 255;
		
	sxmod1 = 350 * (SOUND_DMA_SPEED / SOUND_11k);	// 11k was the original sample rate all dsp was tuned at	
	sxmod2 = 450 * (SOUND_DMA_SPEED / SOUND_11k);
	sxmod1cur = sxmod1;
	sxmod2cur = sxmod2;

	DevMsg ("FX Processor Initialized\n" );
}

void SX_Free(void)
{
	int i;

	// release mono delay line

	SXDLY_Free(ISXMONODLY);

	// release reverb lines

	for (i = 0; i < CSXRVBMAX; i++)
		SXDLY_Free(i + ISXRVB);

	SXDLY_Free(ISXSTEREODLY);
}

// Set up a delay line buffer allowing a max delay of 'delay' seconds 
// Frees current buffer if it already exists. idelay indicates which of 
// the available delay lines to init.

qboolean SXDLY_Init(int idelay, float delay) {
	int cbsamples;
	HANDLE		hData;
	HPSTR		lpData;
	dlyline_t *pdly;

	pdly = &(rgsxdly[idelay]);

	if (delay > SXDLY_MAX)
		delay = SXDLY_MAX;
	
	if (pdly->lpdelayline) {
		GlobalUnlock(pdly->hdelayline);
		GlobalFree(pdly->hdelayline);
		pdly->hdelayline = NULL;
		pdly->lpdelayline = NULL;
	}

	if (delay == 0.0)
		return true;

	pdly->cdelaysamplesmax = SOUND_DMA_SPEED * delay;
	pdly->cdelaysamplesmax += 1;

	cbsamples = pdly->cdelaysamplesmax * sizeof (sample_t);

	hData = GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE, cbsamples); 
	if (!hData) 
	{ 
		g_pSoundServices->ConSafePrint ("Sound FX: Out of memory.\n");
		return false; 
	}
	
	lpData = (char *)GlobalLock(hData);
	if (!lpData)
	{ 
		g_pSoundServices->ConSafePrint ("Sound FX: Failed to lock.\n");
		GlobalFree(hData);		
		return false; 
	}
	
	memset (lpData, 0, cbsamples);

	pdly->hdelayline = hData;
	pdly->lpdelayline = (sample_t *)lpData;

	// init delay loop input and output counters.

	// NOTE: init of idelayoutput only valid if pdly->delaysamples is set
	// NOTE: before this call!

	pdly->idelayinput = 0;
	pdly->idelayoutput = pdly->cdelaysamplesmax - pdly->delaysamples; 
	pdly->xfade = 0;
	pdly->lp = 1;
	pdly->mod = 0;
	pdly->modcur = 0;

	// init lowpass filter memory
	
	pdly->lp0 = pdly->lp1 = pdly->lp2 = pdly->lp3 = pdly->lp4 = pdly->lp5 = 0;

	return true;
}

// release delay buffer and deactivate delay

void SXDLY_Free(int idelay) {
	dlyline_t *pdly = &(rgsxdly[idelay]);

	if (pdly->lpdelayline) {
		GlobalUnlock(pdly->hdelayline);
		GlobalFree(pdly->hdelayline);
		pdly->hdelayline = NULL;
		pdly->lpdelayline = NULL;				// this deactivates the delay
	}
}


// check for new stereo delay param

void SXDLY_CheckNewStereoDelayVal() {
	dlyline_t *pdly = &(rgsxdly[ISXSTEREODLY]);
	int delaysamples;

	// set up stereo delay

	if (sxste_delay.GetFloat() != sxste_delayprev) {
		if (sxste_delay.GetFloat() == 0.0) {
				
			// deactivate delay line
			
			SXDLY_Free(ISXSTEREODLY);
			sxste_delayprev = 0.0;

		} else {

			delaysamples = min(sxste_delay.GetFloat(), SXSTE_MAX) * SOUND_DMA_SPEED;

			// init delay line if not active

			if (pdly->lpdelayline == NULL) {

				pdly->delaysamples = delaysamples;

				SXDLY_Init(ISXSTEREODLY, SXSTE_MAX);
			}

			// do crossfade to new delay if delay has changed

			if (delaysamples != pdly->delaysamples) {
				
				// set up crossfade from old pdly->delaysamples to new delaysamples
				
				pdly->idelayoutputxf = pdly->idelayinput - delaysamples;

				if (pdly->idelayoutputxf < 0)
					pdly->idelayoutputxf += pdly->cdelaysamplesmax;

				pdly->xfade = 128;
			} 

			sxste_delayprev = sxste_delay.GetFloat();
			
			// UNDONE: modulation disabled
			//pdly->mod = 500 * (SOUND_DMA_SPEED / SOUND_11k);		// change delay every n samples
			pdly->mod = 0;
			pdly->modcur = pdly->mod;

			// deactivate line if rounded down to 0 delay

			if (pdly->delaysamples == 0)
				SXDLY_Free(ISXSTEREODLY);
				
		}
	}
}

// stereo delay, left channel only, no feedback

void SXDLY_DoStereoDelay(int count) {
	int left;
	sample_t sampledly;
	sample_t samplexf;
	portable_samplepair_t *pbuf;
	int countr;

	// process delay line if active

	if (rgsxdly[ISXSTEREODLY].lpdelayline) {

		pbuf = PAINTBUFFER;
		countr = count;
		
		// process each sample in the paintbuffer...

		while (countr--) {

			if (gdly3.mod && (--gdly3.modcur < 0))
				gdly3.modcur = gdly3.mod;
		
			// get delay line sample from left line

			sampledly = *(gdly3.lpdelayline + gdly3.idelayoutput);
			left = pbuf->left;
			
			// only process if left value or delayline value are non-zero or xfading

			if (gdly3.xfade || sampledly || left) {

				// if we're not crossfading, and we're not modulating, but we'd like to be modulating,
				// then setup a new crossfade.

				if (!gdly3.xfade && !gdly3.modcur && gdly3.mod) {
					
					// set up crossfade to new delay value, if we're not already doing an xfade
					
					//gdly3.idelayoutputxf = gdly3.idelayoutput + 
					//		((RandomLong(0,0x7FFF) * gdly3.delaysamples) / (RAND_MAX * 2)); // 100 = ~ 9ms

					gdly3.idelayoutputxf = gdly3.idelayoutput + 
							((g_pSoundServices->RandomLong(0,0xFF) * gdly3.delaysamples) >> 9); // 100 = ~ 9ms

					if (gdly3.idelayoutputxf >= gdly3.cdelaysamplesmax)
						gdly3.idelayoutputxf -= gdly3.cdelaysamplesmax;

					gdly3.xfade = 128;
				}
				
				// modify sampledly if crossfading to new delay value

				if (gdly3.xfade) {
					samplexf = (*(gdly3.lpdelayline + gdly3.idelayoutputxf) * (128 - gdly3.xfade)) >> 7;
					sampledly = ((sampledly * gdly3.xfade) >> 7) + samplexf;
					
					if (++gdly3.idelayoutputxf >= gdly3.cdelaysamplesmax)
						gdly3.idelayoutputxf = 0;
	
					if (--gdly3.xfade == 0) 
						gdly3.idelayoutput = gdly3.idelayoutputxf;
				} 
				
				// save output value into delay line

				// left = CLIP(left);
				
				*(gdly3.lpdelayline + gdly3.idelayinput) = left;
								
				// save delay line sample into output buffer
				pbuf->left = sampledly;

			} 
			else 
			{
				// keep clearing out delay line, even if no signal in or out

				*(gdly3.lpdelayline + gdly3.idelayinput) = 0;
			}

			// update delay buffer pointers

			if (++gdly3.idelayinput >= gdly3.cdelaysamplesmax)
				gdly3.idelayinput = 0;
			
			if (++gdly3.idelayoutput >= gdly3.cdelaysamplesmax)
				gdly3.idelayoutput = 0;

			pbuf++;
		}

	}
}

// If sxdly_delay or sxdly_feedback have changed, update delaysamples
// and delayfeed values.  This applies only to delay 0, the main echo line.

void SXDLY_CheckNewDelayVal() {
	dlyline_t *pdly = &(rgsxdly[ISXMONODLY]);

	if (sxdly_delay.GetFloat() != sxdly_delayprev) {
		
		if (sxdly_delay.GetFloat() == 0.0) {
			
			// deactivate delay line
			
			SXDLY_Free(ISXMONODLY);
			sxdly_delayprev = sxdly_delay.GetFloat();

		} else {
			// init delay line if not active

			pdly->delaysamples = min(sxdly_delay.GetFloat(), SXDLY_MAX) * SOUND_DMA_SPEED;

			if (pdly->lpdelayline == NULL)
				SXDLY_Init(ISXMONODLY, SXDLY_MAX);

			// flush delay line and filters

			if (pdly->lpdelayline) {
				Q_memset(pdly->lpdelayline, 0, pdly->cdelaysamplesmax * sizeof(sample_t));
				pdly->lp0 = 0;
				pdly->lp1 = 0;
				pdly->lp2 = 0;
				pdly->lp3 = 0;
				pdly->lp4 = 0;
				pdly->lp5 = 0;
			}
			
			// init delay loop input and output counters

			pdly->idelayinput = 0;
			pdly->idelayoutput = pdly->cdelaysamplesmax - pdly->delaysamples; 

			sxdly_delayprev = sxdly_delay.GetFloat();
			
			// deactivate line if rounded down to 0 delay

			if (pdly->delaysamples == 0)
				SXDLY_Free(ISXMONODLY);
			
		}
	}

	pdly->lp = (int)(sxdly_lp.GetFloat());
	pdly->delayfeed = sxdly_feedback.GetFloat() * 255;
}


// This routine updates both left and right output with 
// the mono delayed signal.  Delay is set through console vars room_delay
// and room_feedback.

void SXDLY_DoDelay(int count) 
{
	int val;
	int valt;
	int left;
	int right;			
	sample_t sampledly;
	portable_samplepair_t *pbuf;
	int countr;
	float fgain;
	int gain;
		

	// process mono delay line if active

	if (rgsxdly[ISXMONODLY].lpdelayline) 
	{

		// calculate gain of delay line with feedback, and use it to
		// reduce output.  ie: make delay line approx unity gain

		// for constant input x with feedback fb:

		// out = x + x*fb + x * fb^2 + x * fb^3...
		// gain = out/x
		// so gain = 1 + fb + fb^2 + fb^3...
		// which, by the miracle of geometric series, equates to 1/1-fb
		// thus, gain = 1/1-fb

		fgain = 1.0 / (1.0 - gdly0.delayfeed / 255.0);
		gain = (int)((1.0 / fgain)* 255.0);	
		
		gain <<= 2; 
		if (gain > 255)
			gain = 255;

   		pbuf = PAINTBUFFER;
		countr = count;

		// process each sample in the paintbuffer...

		while (countr--) 
		{

			// get delay line sample

			sampledly = *(gdly0.lpdelayline + gdly0.idelayoutput);

			left = pbuf->left;
			right = pbuf->right;
			
			// only process if delay line and paintbuffer samples are non zero

			if (sampledly || left || right) 
			{
				// get current sample from delay buffer
				
				// calculate delayed value from avg of left and right channels

				val = ((left + right) >> 1) + ((gdly0.delayfeed * sampledly) >> 8);	
				
				// limit val to short
				// val = CLIP(val);
				
				// lowpass

				if (gdly0.lp) 
				{
					//valt = (gdly0.lp0 + gdly0.lp1 + val) / 3;  // performance
					//valt = (gdly0.lp0 + gdly0.lp1 + (val<<1)) >> 2;

					valt = (gdly0.lp0 + gdly0.lp1 + gdly0.lp2 + gdly0.lp3 + val) / 5;

					gdly0.lp0 = gdly0.lp1;
					gdly0.lp1 = gdly0.lp2;
					gdly0.lp2 = gdly0.lp3;
					gdly0.lp3 = val;
				} 
				else 
				{
					valt = val;
				}

				// store delay output value into output buffer
				
				*(gdly0.lpdelayline + gdly0.idelayinput) = valt;
				
				// mono delay in left and right channels

				// decrease output value by max gain of delay with feedback
				// to provide for unity gain reverb
				// note: this gain varies with the feedback value.

		        pbuf->left = (valt * gain) >> 8;
				pbuf->right = (valt * gain) >> 8;
			} 
			else 
			{
				// not playing samples, but must still flush lowpass buffer and delay line
				valt = gdly0.lp0 = gdly0.lp1 = gdly0.lp2 = gdly0.lp3 = 0;

				*(gdly0.lpdelayline + gdly0.idelayinput) = valt;

			}

			// update delay buffer pointers

			if (++gdly0.idelayinput >= gdly0.cdelaysamplesmax)
				gdly0.idelayinput = 0;
			
			if (++gdly0.idelayoutput >= gdly0.cdelaysamplesmax)
				gdly0.idelayoutput = 0;			

			pbuf++;
		}
	}
}

// Check for a parameter change on the reverb processor

#define RVB_XFADE (32 * SOUND_DMA_SPEED / SOUND_11k)		// xfade time between new delays
#define RVB_MODRATE1 (500 * (SOUND_DMA_SPEED / SOUND_11k))	// how often, in samples, to change delay (1st rvb)
#define RVB_MODRATE2 (700 * (SOUND_DMA_SPEED / SOUND_11k))	// how often, in samples, to change delay (2nd rvb)

void SXRVB_CheckNewReverbVal() 
{
	dlyline_t *pdly;
	int delaysamples;
	int i; 
	int	mod;


	if (sxrvb_size.GetFloat() != sxrvb_sizeprev) 
	{
		
		sxrvb_sizeprev = sxrvb_size.GetFloat();
		
		if (sxrvb_size.GetFloat() == 0.0) 
		{
			// deactivate all delay lines
			
			SXDLY_Free(ISXRVB);
			SXDLY_Free(ISXRVB + 1);
			
		} 
		else 
		{			

			for(i = ISXRVB; i < ISXRVB + CSXRVBMAX; i++) 
			{
				// init delay line if not active

				pdly = &(rgsxdly[i]);
				
				switch (i) {
					case ISXRVB:					
						delaysamples = min(sxrvb_size.GetFloat(), SXRVB_MAX) * SOUND_DMA_SPEED;
						pdly->mod = RVB_MODRATE1;		
						break;
					case ISXRVB+1:
						delaysamples = min(sxrvb_size.GetFloat() * 0.71, SXRVB_MAX) * SOUND_DMA_SPEED;
						pdly->mod = RVB_MODRATE2;
						break;
					default:
						assert(false);
						delaysamples = 0;
						break;
				}

				mod = pdly->mod;				// KB: bug, SXDLY_Init clears mod, modcur, xfade and lp - save mod before call

				if (pdly->lpdelayline == NULL) 
				{
					pdly->delaysamples = delaysamples;

					SXDLY_Init(i, SXRVB_MAX);
				}
				
				pdly->modcur = pdly->mod = mod;	// KB: bug, SXDLY_Init clears mod, modcur, xfade and lp - restore mod after call

				// do crossfade to new delay if delay has changed

				if (delaysamples != pdly->delaysamples) 
				{
					// set up crossfade from old pdly->delaysamples to new delaysamples
					
					pdly->idelayoutputxf = pdly->idelayinput - delaysamples;

					if (pdly->idelayoutputxf < 0)
						pdly->idelayoutputxf += pdly->cdelaysamplesmax;

					pdly->xfade = RVB_XFADE;
				}

				// deactivate line if rounded down to 0 delay

				if (pdly->delaysamples == 0) 
					SXDLY_Free(i);
			}
		}
	}

	rgsxdly[ISXRVB].delayfeed = (sxrvb_feedback.GetFloat()) * 255;
	rgsxdly[ISXRVB].lp = sxrvb_lp.GetFloat();

	rgsxdly[ISXRVB + 1].delayfeed = (sxrvb_feedback.GetFloat()) * 255;
	rgsxdly[ISXRVB + 1].lp = sxrvb_lp.GetFloat();

}


// main routine for updating the paintbuffer with new reverb values.
// This routine updates both left and right lines with 
// the mono reverb signal.  Delay is set through console vars room_reverb
// and room_feedback.  2 reverbs operating in parallel.
								
void SXRVB_DoReverb(int count) 
{
	int val;
	int valt;
	int left;
	int right;				
	sample_t sampledly;
	sample_t samplexf;
	portable_samplepair_t *pbuf;
	int countr;
	int voutm;
	int vlr;
	float fgain1;
	float fgain2;
	int gain;

	// process reverb lines if active

	if (rgsxdly[ISXRVB].lpdelayline) 
	{
		// calculate reverb gains
		fgain1 = 1.0 / (1.0 - gdly1.delayfeed / 255.0);
		fgain2 = 1.0 / (1.0 - gdly2.delayfeed / 255.0) + fgain1;
		
		// inverse gain of parallel reverbs
		gain = (int)((1.0 / fgain2) * 255.0);
	
		gain <<= 2; 

		if (gain > 255)
			gain = 255;

		pbuf = PAINTBUFFER;
		countr = count;		

		// process each sample in the paintbuffer...

		while (countr--) 
		{

			left = pbuf->left;
			right = pbuf->right;
			voutm = 0;
			vlr = (left + right) >> 1;

			// UNDONE: ignored
			if (--gdly1.modcur < 0)
				gdly1.modcur = gdly1.mod;

			// ========================== ISXRVB============================	

			// get sample from delay line

			sampledly = *(gdly1.lpdelayline + gdly1.idelayoutput);

			// only process if something is non-zero

			if (gdly1.xfade || sampledly || left || right) 
			{
				// modulate delay rate
				// UNDONE: modulation disabled
				if (0 && !gdly1.xfade && !gdly1.modcur && gdly1.mod)
				{
					// set up crossfade to new delay value, if we're not already doing an xfade
				
					//gdly1.idelayoutputxf = gdly1.idelayoutput + 
					//		((RandomLong(0,0x7FFF) * gdly1.delaysamples) / (RAND_MAX * 2)); // performance
					
					gdly1.idelayoutputxf = gdly1.idelayoutput + 
							((g_pSoundServices->RandomLong(0,0xFF) * gdly1.delaysamples) >> 9); // 100 = ~ 9ms

					if (gdly1.idelayoutputxf >= gdly1.cdelaysamplesmax)
						gdly1.idelayoutputxf -= gdly1.cdelaysamplesmax;

					gdly1.xfade = RVB_XFADE;
				}
				
				// modify sampledly if crossfading to new delay value

				if (gdly1.xfade) 
				{
					samplexf = (*(gdly1.lpdelayline + gdly1.idelayoutputxf) * (RVB_XFADE - gdly1.xfade)) / RVB_XFADE;
					sampledly = ((sampledly * gdly1.xfade) / RVB_XFADE) + samplexf;
					
					if (++gdly1.idelayoutputxf >= gdly1.cdelaysamplesmax)
						gdly1.idelayoutputxf = 0;

					if (--gdly1.xfade == 0) 
						gdly1.idelayoutput = gdly1.idelayoutputxf;
				} 

				if (sampledly) 
				{
					// get current sample from delay buffer
					
					// calculate delayed value from avg of left and right channels
					
					val = vlr + ((gdly1.delayfeed * sampledly) >> 8);	

					// limit to short
					// val = CLIP(val);
					
				} 
				else 
				{
					val = vlr;
				}

				// lowpass

				if (gdly1.lp) 
				{
					valt = (gdly1.lp0 + gdly1.lp1 + (val<<1)) >> 2;
					gdly1.lp1 = gdly1.lp0;
					gdly1.lp0 = val;
				} 
				else 
				{
					valt = val;
				}

				// store delay output value into output buffer
				
				*(gdly1.lpdelayline + gdly1.idelayinput) = valt;

				voutm = valt;
			} 
			else 
			{
				// not playing samples, but still must flush lowpass buffer & delay line
					
				gdly1.lp0 = gdly1.lp1 = 0;
				*(gdly1.lpdelayline + gdly1.idelayinput) = 0;

				voutm = 0;
			}

			// update delay buffer pointers

			if (++gdly1.idelayinput >= gdly1.cdelaysamplesmax)
				gdly1.idelayinput = 0;
			
			if (++gdly1.idelayoutput >= gdly1.cdelaysamplesmax)
				gdly1.idelayoutput = 0;

			// ========================== ISXRVB + 1========================

			// UNDONE: ignored
			if (--gdly2.modcur < 0)
				gdly2.modcur = gdly2.mod;
			
			if (gdly2.lpdelayline) 
			{
				// get sample from delay line

				sampledly = *(gdly2.lpdelayline + gdly2.idelayoutput);

				// only process if something is non-zero

				if (gdly2.xfade || sampledly || left || right) 
				{
					// UNDONE: modulation disabled
					if (0 && !gdly2.xfade && gdly2.modcur && gdly2.mod) 
					{
						// set up crossfade to new delay value, if we're not already doing an xfade
						
						//gdly2.idelayoutputxf = gdly2.idelayoutput + 
						//		((RandomLong(0,RAND_MAX) * gdly2.delaysamples) / (RAND_MAX * 2)); // performance
						
						gdly2.idelayoutputxf = gdly2.idelayoutput + 
								((g_pSoundServices->RandomLong(0,0xFF) * gdly2.delaysamples) >> 9); // 100 = ~ 9ms


						if (gdly2.idelayoutputxf >= gdly2.cdelaysamplesmax)
							gdly2.idelayoutputxf -= gdly2.cdelaysamplesmax;

						gdly2.xfade = RVB_XFADE;
					}
					
					// modify sampledly if crossfading to new delay value

					if (gdly2.xfade) 
					{
						samplexf = (*(gdly2.lpdelayline + gdly2.idelayoutputxf) * (RVB_XFADE - gdly2.xfade)) / RVB_XFADE;
						sampledly = ((sampledly * gdly2.xfade) / RVB_XFADE) + samplexf;
						
						if (++gdly2.idelayoutputxf >= gdly2.cdelaysamplesmax)
							gdly2.idelayoutputxf = 0;

						if (--gdly2.xfade == 0) 
							gdly2.idelayoutput = gdly2.idelayoutputxf;
					} 
						
					if (sampledly) 
					{
						// get current sample from delay buffer
						
						// calculate delayed value from avg of left and right channels

						val = vlr + ((gdly2.delayfeed * sampledly) >> 8);	

						// limit to short
						// val = CLIP(val);	
					} 
					else 
					{
						val = vlr;
					}

					// lowpass

					if (gdly2.lp) 
					{
						valt = (gdly2.lp0 + gdly2.lp1 + (val<<1)) >> 2;
						gdly2.lp0 = val;
					} 
					else 
					{
						valt = val;
					}

					// store delay output value into output buffer

					*(gdly2.lpdelayline + gdly2.idelayinput) = valt;

					voutm += valt;
				} 
				else 
				{
					// not playing samples, but still must flush lowpass buffer
					
					gdly2.lp0 = gdly2.lp1 = 0;
					*(gdly2.lpdelayline + gdly2.idelayinput) = 0;
				}

				// update delay buffer pointers

				if (++gdly2.idelayinput >= gdly2.cdelaysamplesmax)
					gdly2.idelayinput = 0;
				
				if (++gdly2.idelayoutput >= gdly2.cdelaysamplesmax)
					gdly2.idelayoutput = 0;	
			}

			// ============================ Mix================================

			// add mono delay to left and right channels

			// drop output by inverse of cascaded gain for both reverbs
			voutm = (gain * voutm) >> 8;
			// voutm = CLIP( voutm );
			
			left = voutm;
			right = voutm;

			pbuf->left = left;
			pbuf->right = right;

			pbuf++;
		}
	}
}

// amplitude modulator, low pass filter for underwater weirdness

void SXRVB_DoAMod(int count) 
{

	int valtl, valtr;
	int left;
	int right;				
	portable_samplepair_t *pbuf;
	int countr;
	int fLowpass;
	int fmod;

	// process reverb lines if active

	if (sxmod_lowpass.GetFloat() != 0.0 || sxmod_mod.GetFloat() != 0.0) {

		pbuf = PAINTBUFFER;
		countr = count;		
		
		fLowpass = (sxmod_lowpass.GetFloat() != 0.0);
		fmod = (sxmod_mod.GetFloat() != 0.0);

		// process each sample in the paintbuffer...

		while (countr--) {

	        left = pbuf->left;
			right = pbuf->right;

			// only process if non-zero

			if (fLowpass) {

				valtl = left;
				valtr = right;
				
				left = (rgsxlp[0] + rgsxlp[1] + rgsxlp[2] + rgsxlp[3] + rgsxlp[4] + left);
				right = (rgsxlp[5] + rgsxlp[6] + rgsxlp[7]+ rgsxlp[8] + rgsxlp[9] + right);

				left = ((left << 1) + (left << 3)) >> 6; // * 10/64
				right = ((right << 1) + (right << 3)) >> 6; // * 10/64
				
				rgsxlp[4] = valtl;
				rgsxlp[9] = valtr;

				rgsxlp[0] = rgsxlp[1];
				rgsxlp[1] = rgsxlp[2];
				rgsxlp[2] = rgsxlp[3];
				rgsxlp[3] = rgsxlp[4];
				rgsxlp[4] = rgsxlp[5];
				rgsxlp[5] = rgsxlp[6];
				rgsxlp[6] = rgsxlp[7];
				rgsxlp[7] = rgsxlp[8];
				rgsxlp[8] = rgsxlp[9];

			}

				
			if (fmod) {
				if (--sxmod1cur < 0)
					sxmod1cur = sxmod1;

				if (!sxmod1)
					sxamodlt = g_pSoundServices->RandomLong(32,255);
				
				if (--sxmod2cur < 0)
					sxmod2cur = sxmod2;

				if (!sxmod2)
					sxamodlt = g_pSoundServices->RandomLong(32,255);
				
				left = (left * sxamodl) >> 8;
				right = (right * sxamodr) >> 8;

				if (sxamodl < sxamodlt) 
					sxamodl++;
				else if (sxamodl > sxamodlt)
					sxamodl--;

				if (sxamodr < sxamodrt) 
					sxamodr++;
				else if (sxamodr > sxamodrt)
					sxamodr--;
			}

			left = CLIP(left);
			right = CLIP(right);

			pbuf->left = left;
			pbuf->right = right;

			pbuf++;			
		}
	}
}


struct sx_preset_t 
{
	float room_lp;					// for water fx, lowpass for entire room
	float room_mod;					// stereo amplitude modulation for room

	float room_size;				// reverb: initial reflection size
	float room_refl;				// reverb: decay time
	float room_rvblp;				// reverb: low pass filtering level

	float room_delay;				// mono delay: delay time
	float room_feedback;			// mono delay: decay time
	float room_dlylp;				// mono delay: low pass filtering level

	float room_left;				// left channel delay time
};


sx_preset_t rgsxpre[CSXROOM] = 
{

// SXROOM_OFF					0

//	lp		mod		size	refl	rvblp	delay	feedbk	dlylp	left  
	{0.0,	0.0,	0.0,	0.0,	1.0,	0.0,	0.0,	2.0,	0.0},

// SXROOM_GENERIC				1		// general, low reflective, diffuse room

//	lp		mod		size	refl	rvblp	delay	feedbk	dlylp	left  
	{0.0,	0.0,	0.0,	0.0,	1.0,	0.065,	0.1,	0.0,	0.01},

// SXROOM_METALIC_S				2		// highly reflective, parallel surfaces
// SXROOM_METALIC_M				3
// SXROOM_METALIC_L				4

//	lp		mod		size	refl	rvblp	delay	feedbk	dlylp	left  
	{0.0,	0.0,	0.0,	0.0,	1.0,	0.02,	0.75,	0.0,	0.01}, // 0.001
	{0.0,	0.0,	0.0,	0.0,	1.0,	0.03,	0.78,	0.0,	0.02}, // 0.002
	{0.0,	0.0,	0.0,	0.0,	1.0,	0.06,	0.77,	0.0,	0.03}, // 0.003


// SXROOM_TUNNEL_S				5		// resonant reflective, long surfaces
// SXROOM_TUNNEL_M				6
// SXROOM_TUNNEL_L				7

//	lp		mod		size	refl	rvblp	delay	feedbk	dlylp	left  
	{0.0,	0.0,	0.05,	0.85,	1.0,	0.018,	0.7,	2.0,	0.01}, // 0.01
	{0.0,	0.0,	0.05,	0.88,	1.0,	0.020,	0.7,	2.0,	0.02}, // 0.02
	{0.0,	0.0,	0.05,	0.92,	1.0,	0.025,	0.7,	2.0,	0.04}, // 0.04

// SXROOM_CHAMBER_S				8		// diffuse, moderately reflective surfaces
// SXROOM_CHAMBER_M				9
// SXROOM_CHAMBER_L				10

//	lp		mod		size	refl	rvblp	delay	feedbk	dlylp	left  
	{0.0,	0.0,	0.05,	0.84,	1.0,	0.0,	0.0,	2.0,	0.012}, // 0.003
	{0.0,	0.0,	0.05,	0.90,	1.0,	0.0,	0.0,	2.0,	0.008}, // 0.002
	{0.0,	0.0,	0.05,	0.95,	1.0,	0.0,	0.0,	2.0,	0.004}, // 0.001

// SXROOM_BRITE_S				11		// diffuse, highly reflective
// SXROOM_BRITE_M				12
// SXROOM_BRITE_L				13

//	lp		mod		size	refl	rvblp	delay	feedbk	dlylp	left  
	{0.0,	0.0,	0.05,	0.7,	0.0,	0.0,	0.0,	2.0,	0.012}, // 0.003
	{0.0,	0.0,	0.055,	0.78,	0.0,	0.0,	0.0,	2.0,	0.008}, // 0.002
	{0.0,	0.0,	0.05,	0.86,	0.0,	0.0,	0.0,	2.0,	0.002}, // 0.001

// SXROOM_WATER1				14		// underwater fx
// SXROOM_WATER2				15
// SXROOM_WATER3				16

//	lp		mod		size	refl	rvblp	delay	feedbk	dlylp	left  
	{1.0,	0.0,	0.0,	0.0,	1.0,	0.0,	0.0,	2.0,	0.01},
	{1.0,	0.0,	0.0,	0.0,	1.0,	0.06,	0.85,	2.0,	0.02},
	{1.0,	0.0,	0.0,	0.0,	1.0,	0.2,	0.6,	2.0,	0.05},

// SXROOM_CONCRETE_S			17		// bare, reflective, parallel surfaces
// SXROOM_CONCRETE_M			18
// SXROOM_CONCRETE_L			19

//	lp		mod		size	refl	rvblp	delay	feedbk	dlylp	left  
	{0.0,	0.0,	0.05,	0.8,	1.0,	0.0,	0.48,	2.0,	0.016}, // 0.15 delay, 0.008 left
	{0.0,	0.0,	0.06,	0.9,	1.0,	0.0,	0.52,	2.0,	0.01 }, // 0.22 delay, 0.005 left
	{0.0,	0.0,	0.07,	0.94,	1.0,	0.3,	0.6,	2.0,	0.008}, // 0.001

// SXROOM_OUTSIDE1				20		// echoing, moderately reflective
// SXROOM_OUTSIDE2				21		// echoing, dull
// SXROOM_OUTSIDE3				22		// echoing, very dull

//	lp		mod		size	refl	rvblp	delay	feedbk	dlylp	left  
	{0.0,	0.0,	0.0,	0.0,	1.0,	0.3,	0.42,	2.0,	0.0},
	{0.0,	0.0,	0.0,	0.0,	1.0,	0.35,	0.48,	2.0,	0.0},
	{0.0,	0.0,	0.0,	0.0,	1.0,	0.38,	0.6,	2.0,	0.0},

// SXROOM_CAVERN_S				23		// large, echoing area
// SXROOM_CAVERN_M				24
// SXROOM_CAVERN_L				25

//	lp		mod		size	refl	rvblp	delay	feedbk	dlylp	left  
	{0.0,	0.0,	0.05,	0.9,	1.0,	0.2,	0.28,	0.0,	0.0},
	{0.0,	0.0,	0.07,	0.9,	1.0,	0.3,	0.4,	0.0,	0.0},
	{0.0,	0.0,	0.09,	0.9,	1.0,	0.35,	0.5,	0.0,	0.0},

// SXROOM_WEIRDO1				26		
// SXROOM_WEIRDO2				27
// SXROOM_WEIRDO3				28
// SXROOM_WEIRDO3				29

//	lp		mod		size	refl	rvblp	delay	feedbk	dlylp	left  
	{0.0,	1.0,	0.01,	0.9,	0.0,	0.0,	0.0,	2.0,	0.05},
	{0.0,	0.0,	0.0,	0.0,	1.0,	0.009,	0.999,	2.0,	0.04},
	{0.0,	0.0,	0.001,	0.999,	0.0,	0.2,	0.8,	2.0,	0.05}

};



// force next call to sx_roomfx to reload all room parameters.
// used when switching to/from hires sound mode.

void SX_ReloadRoomFX()
{
	// reset all roomtype parms

	sxroom_typeprev = -1.0;
		
	sxdly_delayprev = -1.0;
	sxrvb_sizeprev = -1.0;
	sxste_delayprev = -1.0;
	sxroom_typeprev = -1.0;

	// UNDONE: handle sxmod and mod parms? 
}


// main routine for processing room sound fx
// if fFilter is TRUE, then run in-line filter (for underwater fx)
// if fTimefx is TRUE, then run reverb and delay fx
// NOTE: only processes preset room_types from 0-29 (CSXROOM)

void SX_RoomFX(int endtime, int fFilter, int fTimefx) 
{
	int fReset;
	int i;
	int sampleCount;
	float roomType;

	// return right away if fx processing is turned off

	if (sxroom_off.GetFloat() != 0.0)
		return;

	sampleCount = endtime - paintedtime;
	if (sampleCount < 0)
		return;

	// detect changes in hires sound param

	sxhires = (hisound.GetFloat() == 0 ? 0 : 1);

	if (sxhires != sxhiresprev)
	{
		SX_ReloadRoomFX();
		sxhiresprev = sxhires;
	}

	fReset = FALSE;
	if ( g_pClientSidePrediction->GetWaterLevel() > 2 )
		roomType = sxroomwater_type.GetFloat();
	else
		roomType = sxroom_type.GetFloat();

	// only process legacy roomtypes here

	if ( (int)roomType >= CSXROOM )
		return;

	if (roomType != sxroom_typeprev) 
	{
		
		// Msg ("Room_type: %2.1f\n", roomType);

		sxroom_typeprev = roomType;

		i = (int)(roomType);
		
		if (i < CSXROOM && i >= 0)
		{
			sxmod_lowpass.SetValue(  rgsxpre[i].room_lp);
			sxmod_mod.SetValue( rgsxpre[i].room_mod);
			sxrvb_size.SetValue( rgsxpre[i].room_size);
			sxrvb_feedback.SetValue( rgsxpre[i].room_refl);
			sxrvb_lp.SetValue( rgsxpre[i].room_rvblp);
			sxdly_delay.SetValue( rgsxpre[i].room_delay);
			sxdly_feedback.SetValue( rgsxpre[i].room_feedback);
			sxdly_lp.SetValue( rgsxpre[i].room_dlylp);
			sxste_delay.SetValue( rgsxpre[i].room_left);
		}

		SXRVB_CheckNewReverbVal();
		SXDLY_CheckNewDelayVal();
		SXDLY_CheckNewStereoDelayVal();

		fReset = TRUE;
	}

	if ( fReset || roomType != 0.0 ) 
	{
		// debug code
		SXRVB_CheckNewReverbVal();
		SXDLY_CheckNewDelayVal();
		SXDLY_CheckNewStereoDelayVal();	
		// debug code

		if ( fFilter )
			SXRVB_DoAMod(sampleCount);

		if ( fTimefx )
		{
			SXRVB_DoReverb(sampleCount);
			SXDLY_DoDelay(sampleCount);
			SXDLY_DoStereoDelay(sampleCount);
		}
	} 
}





////////////////////////
// Dynamics processing
////////////////////////

// compressor defines
#define COMP_MAX_AMP	32767			// abs max amplitude
#define COMP_THRESH		20000			// start compressing at this threshold

// compress input value - smoothly limit output y to -32767 <= y <= 32767
inline int S_Compress( int xin )
{

	return CLIP( xin >> 2 );	// DEBUG - disabled


	float Yn, Xn, Cn, Fn;
	float C0 = 20000;	// threshold
	float p = .3;		// compression ratio
	float g = 1;		// gain after compression
	
	Xn = (float)xin;

	// Compressor formula:
	// Cn = l*Cn-1 + (1-l)*|Xn|				// peak detector with memory
	// f(Cn) = (Cn/C0)^(p-1)	for Cn > C0	// gain function above threshold
	// f(Cn) = 1				for C <= C0	// unity gain below threshold
	// Yn = f(Cn) * Xn						// compressor output
	
	// UNDONE: curves discontinuous at threshold, causes distortion, try catmul-rom

	//float l = .5;		// compressor memory
	//Cn = l * (*pCnPrev) + (1 - l) * fabs((float)xin);
	//*pCnPrev = Cn;
	
	Cn = fabs((float)xin);

	if (Cn < C0)
		Fn = 1;
	else
		Fn = powf((Cn / C0),(p - 1));
		
	Yn = Fn * Xn * g;
	
	//if (Cn > 0)
	//	Msg("%d -> %d\n", xin, (int)Yn);	// DEBUG

	//if (fabs(Yn) > 32767)
	//	Yn = Yn;			// DEBUG

	return (CLIP((int)Yn));
}

