#ifndef MACROS_H
#define MACROS_H

#define LENGTH(arr) (sizeof((arr))/sizeof(((arr)[0])))
#define VEC3_COORDS(v) v.x, v.y, v.z

#ifndef strdup
#define strdup(str) strcpy(malloc(strlen(str)), str)
#endif

#endif