#ifndef GM_MOTHERSHIP
#define GM_MOTHERSHIP

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <iostream>
#include <windows.h>

#include <tchar.h>
#include <thread>
#include <chrono>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#define GLFW_INCLUDE_NONE
#include "glad/gl.h"
#include "GLFW/glfw3.h"

#include "git.hpp"

#ifdef WIN32
#include "win32_mame_comms_server.hpp"
#endif

#endif