// callbacks.h
#ifndef INC_CALLBACKSH
#define INC_CALLBACKSH

extern void (*Cbuf_AddText) (char *text);
extern void (*SetStartupMode)(BOOL bMode);
extern double (*Sys_FloatTime)(void);
extern void (*Host_GetHostInfo)(float *fps, int *nActive, int *nSpectators, int *nMaxPlayers, char *pszMap);
extern void (*Con_GetInput)( char *pszBuffer, int nBufferLength );
extern void (*Key_UpLine)(void);

#endif // !INC_CALLBACKSH