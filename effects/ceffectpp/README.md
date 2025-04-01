##ceffectpp##


ceffectpp is a sort of pre-processor for GLSL shaders (C-Effect-Pre-Processor). An "effect" is a combination of several shaders, usually at least a vertex shader and a fragment shader. ceffectpp, when given a list of filepaths to shaders, will group shader files by name into effects (ex. forward.vs, forward.fs get grouped together), and attempt to identify uniform variables and vertex attributes within them. It can then print this information out into a header and source file that can be used in an OpenGL project. 


###Example Usage###


To view what output would be:

    /usr/local/bin/ceffectpp -c shaders/*.vs shaders/*.fs shaders/*.gs
    /usr/local/bin/ceffectpp -h shaders/*.vs shaders/*.fs shaders/*.gs

To generate source and header files:

    /usr/local/bin/ceffectpp -c shaders/*.vs shaders/*.fs shaders/*.gs > effects.c
    /usr/local/bin/ceffectpp -h shaders/*.vs shaders/*.fs shaders/*.gs > effects.h

ceffectpp is best used as a part of the make process for your project.
Example output can be found in the example_output folder.
	
###Compiling and Installing###
The following command will build ceffectpp and stick it in your /usr/local/bin/:

	make install
	
###Motivation###

I wrote this tool in the course of making a 3D game engine in C (still a work in progress). I wanted a simple and fast way to make my engine "aware" of shader files dropped in a folder and given similar names, without having to manually specify in code what my shaders were called and what their attributes / uniforms were.