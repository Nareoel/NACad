#pragma once

// the single place that picks the GL headers: WebGL2 (GLES 3.0) on the web, glad on the desktop
#ifdef __EMSCRIPTEN__
#include <GLES3/gl3.h>
#else
#include <glad/glad.h>
#endif
