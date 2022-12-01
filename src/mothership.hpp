#ifndef GM_MOTHERSHIP
#define GM_MOTHERSHIP

#define LUA_SERVER_PORT "2159"
#define DEFAULT_BUFLEN 512

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <iostream>
#include <windows.h>
#include <WinSock2.h>
#include <WS2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")

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

#endif