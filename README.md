
#Sock Engine#
 
Sock is a free 3D game engine that I'm making in my free time as a hobby and learning project. Sock sort of stands for Shitty OpenGL C. The 'k' is silent.

Currently only really maintained for OSX.

##Compiling##
Extract to a directory, cd in, and run make.

The executable will be "sock". You can change this in the Makefile.

##Dependencies##
I try to keep Sock light on dependencies, but there are a few things that can't be avoided.

###SDL 2.0###
SDL 2.0 is a game library that creates the window context, OpenGL context, handles input, etc.

[http://libsdl.org/download-2.0.php](http://libsdl.org/download-2.0.php)
###SDL_image###
SDL_image is used for loading different image formats into the engine to be used as textures.

[http://www.libsdl.org/projects/SDL_image/](http://www.libsdl.org/projects/SDL_image/)
###Glew###
Glew helps manage OpenGL extensions.

This should come pre-installed with OSX.
###OpenGL (3.2 minimum)###
OpenGL is the graphics layer used for this project.

This should come pre-installed with OSX.
###Awk (Gawk, to be specific)###
Awk is used to convert model files into header files to compile them into the engine.

This should come pre-installed with OSX.


###Luajit###
I'm in the process of converting my awk scripts to Lua for performance and ease of editing. You can use a different version of Lua than luajit, but you'll have to change the command which calls it in models/Makefile.

###glalgebra###
glalgebra implements vector/matrix operations I wrote for this project, then later split off to make a separate library.
https://github.com/GavinHigham/glalgebra

###ceffectpp###
ceffectpp preprocesses shaders to group them by effect, and extract vertex attribute and uniform variable names.
https://github.com/GavinHigham/ceffectpp
