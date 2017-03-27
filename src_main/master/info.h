#ifndef INFO_H
#define INFO_H
#ifdef _WIN32
#pragma once
#endif

void Info_SetValueForStarKey ( char *s, const char *key, const char *value, int maxsize);
// userinfo functions
const char *Info_ValueForKey ( const char *s, const char *key );
void Info_RemoveKey ( char *s, const char *key );
void Info_RemovePrefixedKeys ( char *start, char prefix );
void Info_SetValueForKey ( char *s, const char *key, const char *value, int maxsize );

#endif // INFO_H
