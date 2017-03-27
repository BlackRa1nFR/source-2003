//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: determine CPU speed under linux
//
// $NoKeywords: $
//=============================================================================
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <tier0/platform.h>


#define rdtsc(x) \
	__asm__ __volatile__ ("rdtsc" : "=A" (x))

// Compute 64 bit value / timeval 
static inline uint64 operator / (uint64 v, timeval t)
{
  double tmp = (double)(t.tv_sec) + ((double)(t.tv_usec) / 1000000.0);
  uint64 ret = (uint64) ((double) v / tmp);
  return ret;
//  return uint64 (v / ( ((double)(t.tv_sec)) + ((double)(t.tv_usec) / 1000000.)));
}

// Compute left - right for timeval structures.
static inline timeval operator - (const timeval &left, const timeval &right)
{
  uint64 left_us = (uint64) left.tv_sec * 1000000 + left.tv_usec;
  uint64 right_us = (uint64) right.tv_sec * 1000000 + right.tv_usec;
  uint64 diff_us = left_us - right_us;
  timeval r = { diff_us / 1000000, diff_us % 1000000 };
  return r;
}

// Compute the positive difference between two 64 bit numbers.
static inline uint64 diff(uint64 v1, uint64 v2)
{
  uint64 d = v1 - v2;
  if (d >= 0) return d; else return -d;
}

uint64 CalculateCPUFreq()
{
  // Compute the period. Loop until we get 3 consecutive periods that
  // are the same to within a small error. The error is chosen
  // to be +/- 0.01% on a P-200.
  const uint64 error = 20000;
  const int max_iterations = 300;
  int count;
  uint64 period, period1 = error * 2, period2 = 0,  period3 = 0;

  for (count = 0; count < max_iterations; count++)
    {
      timeval start_time, end_time;
      uint64 start_tsc, end_tsc;
      gettimeofday (&start_time, 0);
      rdtsc (start_tsc);
      usleep (5000); // sleep for 5 msec
      gettimeofday (&end_time, 0);
      rdtsc (end_tsc);
	
      period3 = (end_tsc - start_tsc) / (end_time - start_time);

      if (diff (period1, period2) <= error &&
  	  diff (period2, period3) <= error &&
   	  diff (period1, period3) <= error)
 		break;

      period1 = period2;
      period2 = period3;
    }

   if (count == max_iterations)
    {
	return 0;
    }

   // Set the period to the average period measured.
   period = (period1 + period2 + period3) / 3;

   // Some Pentiums have broken TSCs that increment very
   // slowly or unevenly. 
   if (period < 10000000)
    {
	return 0;
    }

   return period;
}

