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

void lua_server_func(const char *port) {
    struct addrinfo *result = NULL;
    struct addrinfo *ptr = NULL;
    struct addrinfo hints;
    ZeroMemory(&hints, sizeof(hints));

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    int get_addr_info_result = getaddrinfo(NULL, port, &hints, &result);
    if (get_addr_info_result != 0) {
        std::cerr << "getaddrinfo failed: " << get_addr_info_result << "\n";
        return;
    }

    SOCKET svr_sock = INVALID_SOCKET;
    ptr = result;

    svr_sock = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
    if (svr_sock == INVALID_SOCKET) {
        std::cerr << "Error at socket(): " << WSAGetLastError() << "\n";
        freeaddrinfo(result);
        return;
    }

    int bind_result = bind(svr_sock, result->ai_addr, (int)(result->ai_addrlen));
    if (bind_result == SOCKET_ERROR) {
        std::cerr << "Failed to bind socket: " << WSAGetLastError() << "\n";
        freeaddrinfo(result);
        closesocket(svr_sock);
        return;
    }

    if (listen(svr_sock, 1) == SOCKET_ERROR) {
        std::cerr << "Listen failed: " << WSAGetLastError() << "\n";
        closesocket(svr_sock);
        freeaddrinfo(result);
        return;
    }

    SOCKET c_sock = INVALID_SOCKET;

    c_sock = accept(svr_sock, NULL, NULL);
    if (c_sock == INVALID_SOCKET) {
        std::cerr << "Accept failed: " << WSAGetLastError() << "\n";
        closesocket(c_sock);
        closesocket(svr_sock);
        freeaddrinfo(result);
        return;
    }

    const int recv_buf_len = DEFAULT_BUFLEN;
    char recv_buf[recv_buf_len];
    int recv_result;

    do {
        recv_result = recv(c_sock, recv_buf, recv_buf_len, 0);
        if (recv_result > 0) {
            recv_buf[recv_result] = '\0'; // null terminate the buffer at the number of bytes read on receive
            std::cout << "[Server]: " << recv_buf << "\n";
        } else if (recv_result == 0) {
            std::cout << "Connection closed.\n";
        } else {
            std::cerr << "Receive failed: " << WSAGetLastError() << "\n";
        }
    } while (recv_result > 0);

//    int connect_result;
//    do {
//        connect_result = connect(svr_sock, ptr->ai_addr, (int)(ptr->ai_addrlen));
//        if (connect_result == SOCKET_ERROR) {
//            int last_error = WSAGetLastError();
//            std::cout << "Failed at connect(): " << last_error << "\n";
//            
//            if (last_error == 10061) { // Connection refused (server wasn't ready)
//                std::cout << "Retrying in 5 seconds...\n";
//                std::this_thread::sleep_for(std::chrono::seconds(5));
//            } else {
//                closesocket(svr_sock);
//                freeaddrinfo(result);
//                return;
//            }
//        }
//    } while (connect_result != 0); 
//
//    std::cout << "Socket connected!\n";
//    // Now let's get some data!
//    const int recv_buf_len = DEFAULT_BUFLEN;
//    char recv_buf[recv_buf_len];
//    int recv_result;
//
//    do {
//        recv_result = recv(C2_COMMONSEPARATOR, recv_buf, recv_buf_len, 0);
//        if (recv_result > 0) {
//            std::cout << recv_buf << "\n";
//        } else if (recv_result == 0) {
//            std::cout << "Connection closed.\n";
//        } else {
//            std::cerr << "Receive failed: " << WSAGetLastError() << "\n";
//        }
//    } while (recv_result > 0);

    closesocket(c_sock);
    closesocket(svr_sock);
    freeaddrinfo(result);
}

int _tmain(int argc, TCHAR *argv[]) {
    std::cout << 
        "branch: "   << gm_git::branch  << 
        ", tag: "    << gm_git::tag     << 
        ", commit: " << gm_git::commit  << "\n";

    // Initialize Winsock
    WSADATA wsa_data;
    int wsa_init_result;

    wsa_init_result = WSAStartup(MAKEWORD(2,2), &wsa_data);
    if (wsa_init_result != 0) {
        std::cerr << "WSAStartup failed: " << wsa_init_result << "\n";
    }


    // Create a sub-process
    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    if (argc != 2) {
        printf("Usage: %s [cmdline]\n", argv[0]);
        return EXIT_FAILURE;
    }

    // Start a thread to listen for a connection from the lua script (MAME)
    std::thread lua_server(lua_server_func, LUA_SERVER_PORT);

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

    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    GLFWwindow* window = glfwCreateWindow(1280, 720, "Galaga Mothership", NULL, NULL);
    if (!window)
    {
        // Window or OpenGL context creation failed
        printf("Failed to create window");
    }

    glfwSetKeyCallback(window, key_callback);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    int version = gladLoadGL(glfwGetProcAddress);
    if (version == 0) {
        printf("Failed to initialize OpenGL context\n");
        return -1;
    }

    // Successfully loaded OpenGL
    printf("Loaded OpenGL %d.%d\n", GLAD_VERSION_MAJOR(version), GLAD_VERSION_MINOR(version));

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.Fonts->AddFontFromFileTTF("fonts/fira_mono.ttf", 26.0f);
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Our state
    bool show_demo_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    ImGui::GetStyle().ScaleAllSizes(2.0f);

    while (!glfwWindowShouldClose(window))
    {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        glfwPollEvents();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);

        // 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
        {
            static float f = 0.0f;
            static int counter = 0;
            
            ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.
            
            
            //ImGui::InputFloat("Global Scale", &scale, 0.01f, 0.1f, "%.3f");

            ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
            ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
            ImGui::Checkbox("Another Window", &show_another_window);

            ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
            ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

            if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
                counter++;
            ImGui::SameLine();
            ImGui::Text("counter = %d", counter);

            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
            ImGui::End();
        }

        // 3. Show another simple window.
        if (show_another_window)
        {
            ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
            ImGui::Text("Hello from another window!");
            if (ImGui::Button("Close Me"))
                show_another_window = false;
            ImGui::End();
        }

        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);    
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    WaitForSingleObject(pi.hProcess, INFINITE);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    //lua_server.join();
    WSACleanup();

    printf("[Mothership.exe]: Exiting...\n");
    return EXIT_SUCCESS;
}