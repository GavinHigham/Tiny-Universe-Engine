#ifndef PRINT_EFFECT_H
#define PRINT_EFFECT_H

void print_filepaths(char **paths, int len);
void print_effect_definition(char **ustrs, int ulen, char **astrs, int alen);
void print_effect_list(char **progstrs, int len);
void print_uniform_strings(char **ustrs, unsigned long len);
void print_attribute_strings(char **astrs, unsigned long len);

#endif