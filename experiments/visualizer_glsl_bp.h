//Include this file in "X-Macros" pattern to generate OpenGL boilerplate.
//name is the name of the GLSL uniform, and the name of the generated handle.
//Using the variadic argument, one can optionally supply an expression to declare storage for the uniform.
#define U(name, ...) UNIFORM(name, __VA_ARGS__)
U(iResolution)
U(iMouse)
U(iTime)
U(tweaks, float tweaks[4])
U(tweaks2, float tweaks2[4])
U(frequencies, GLuint frequencies)
U(style, int style)
#undef U
ATTRIBUTE(pos)
ATTRIBUTE(position)
