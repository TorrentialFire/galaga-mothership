#include "mothership.hpp"

void error_callback(int error, const char* description)
{
    fprintf(stderr, "Error: %s\n", description);
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
}

int _tmain(int argc, TCHAR *argv[]) {
    std::cout << 
        "branch: "   << gm_git::branch  << 
        ", tag: "    << gm_git::tag     << 
        ", commit: " << gm_git::commit  << "\n";
    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    if (argc != 2) {
        printf("Usage: %s [cmdline]\n", argv[0]);
        return EXIT_FAILURE;
    }

    //LPCSTR dir = "\"C:\\Users\\torre\\vcs\\galaga-mothership\\\"";

    if (!CreateProcess(
        NULL,
        argv[1],
        NULL,
        NULL,
        FALSE,
        0,
        NULL,
        NULL,
        &si,
        &pi)) {
        
        printf("CreateProcess failed (%d).\n", GetLastError());
        return EXIT_FAILURE;
    }

    glfwSetErrorCallback(error_callback);

    if (!glfwInit())
    {

    }

    GLFWwindow* window = glfwCreateWindow(640, 480, "Galaga Mothership", NULL, NULL);
    if (!window)
    {
        // Window or OpenGL context creation failed
        printf("Failed to create window");
    }

    glfwSetKeyCallback(window, key_callback);

    glfwMakeContextCurrent(window);

    glfwSwapInterval(1);

    while (!glfwWindowShouldClose(window))
    {
        // Keep running

        glfwSwapBuffers(window);

        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();

    WaitForSingleObject(pi.hProcess, INFINITE);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    printf("[Mothership.exe]: Exiting...\n");
    return EXIT_SUCCESS;
}