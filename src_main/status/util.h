#if !defined( UTIL_H )
#define UTIL_H
#ifdef _WIN32
#pragma once
#endif

void TIME_SecondsToHMS( double in, int *w, int *d, int *h, int *m, double *s );
void NET_SplitAddress(char *in, char *out, int *port, int defport);

#endif // UTIL_H