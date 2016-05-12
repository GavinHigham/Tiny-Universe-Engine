#ifndef SHADER_UTILS_H
#define SHADER_UTILS_H

struct shader_prog;
struct shader_info;
int init_effects(struct shader_prog **programs, struct shader_info **infos, int numprogs);
int reload_effects(struct shader_prog **programs, struct shader_info **infos, int numprogs);

#endif