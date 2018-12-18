#ifndef EMCC_STUBS_H
#define EMCC_STUBS_H

#ifdef __EMSCRIPTEN__
#define popen(...) 0
#define pclose(...) 0
#endif

#endif