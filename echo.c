#include <stdio.h>

#define MAX_LENGTH 256

int main(int c, char *args[])
{
    char *progname = args[0];
    char line[MAX_LENGTH];
    
    while (gets(line) != NULL) {
        printf("%s: %s\n", progname+2, line);
    }
}
