#ifndef MACROS_H
#define MACROS_H

#define LENGTH(arr) (sizeof((arr))/sizeof(((arr)[0])))
#define VEC3_COORDS(v) v.x, v.y, v.z

// printf colors
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_NUMERIC_FCOLOR "\x1b[38;5;%im"
#define ANSI_NUMERIC_BCOLOR "\x1b[48;5;%im"
#define ANSI_FCOLOR "\x1b[38;2;%i;%i;%im"
#define ANSI_BCOLOR "\x1b[48;2;%i;%i;%im"
#define ANSI_COLOR_RESET   "\x1b[0m"

#endif