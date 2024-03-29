
# Tiny Universe Engine #
 
Tiny Universe Engine is a free 3D game engine that I'm making in my free time as a hobby and learning project.

I'm using the project as a means to learn OpenGL, to develop my C style and practices on a large-ish project, and to learn computer graphics techniques such as deferred rendering, stencil-buffer shadows, procedural generation, erosion simulation, physically-based rendering, and more. Eventually I hope to turn it into a nice lightweight space exploration game/engine.

Currently the project is only maintained for macOS.

If you're reading this sometime in the far future, there may be development blogs about this project (and others) at [http://www.gavinblog.space/](http://www.gavinblog.space/).

## Compiling ##
Extract to a directory, cd in, and run make.

The executable will be "tu". You can change this in the Makefile.

## Dependencies ##
I try to keep TU light on dependencies, but there are a few things that can't be avoided.

### SDL 2.0 ###
SDL 2.0 is a game library that creates the window context, OpenGL context, handles input, etc.

[http://libsdl.org/download-2.0.php](http://libsdl.org/download-2.0.php)

### SDL_image ###
SDL_image is used for loading different image formats into the engine to be used as textures.

[http://www.libsdl.org/projects/SDL_image/](http://www.libsdl.org/projects/SDL_image/)
### GLEW ###
GLEW helps manage OpenGL extensions.

This may come pre-installed with macOS, otherwise you can get it from Homebrew.
### OpenGL (3.3 minimum) ###
This project uses the OpenGL interface.

### Awk (Gawk, to be specific) ###
Awk is used to convert model files into header files to compile them into the engine.

This should come pre-installed with macOS.

### Lua 5.3 ###
I'm gradually integrating Lua as a scripting and configuration language for the project. I'm also in the process of converting my awk scripts to Lua to reduce dependencies. At the moment, you'll need both the Lua library and the binary. In the future I'll have the library compiled first so it can be used to run the Lua build scripts.
