#include "mothership.hpp"

std::atomic<global_t> globals({0});

void error_callback(int error, const char* description)
{
    fprintf(stderr, "Error: %s\n", description);
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
}

class simple_tex {
public:
    GLuint texture;
    int width;
    int height;
};

// Simple helper function to load an image into a OpenGL texture with common settings, returns null if texture can't be created
bool load_texture_from_file(const char* filename, simple_tex * tex)
{
    // Load from file
    int image_width = 0;
    int image_height = 0;
    unsigned char* image_data = stbi_load(filename, &image_width, &image_height, NULL, 4);
    if (image_data == NULL)
        return false;

    // Create a OpenGL texture identifier
    GLuint image_texture;
    glGenTextures(1, &image_texture);
    glBindTexture(GL_TEXTURE_2D, image_texture);

    // Setup filtering parameters for display
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // This is required on WebGL for non power-of-two textures
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); // Same

    // Upload pixels into texture
#if defined(GL_UNPACK_ROW_LENGTH) && !defined(__EMSCRIPTEN__)
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_width, image_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
    stbi_image_free(image_data);

    //std::unique_ptr<simple_tex> tex = std::make_unique<simple_tex>();
    tex->texture = image_texture;
    tex->width = image_width;
    tex->height = image_height;

    return true;
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
    mame::server_config config = { .port = MAME_COMMS_SERVER_PORT };
    auto comms_server = mame::comms_server::init(config);
    comms_server->start();

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

    GLFWwindow* window = glfwCreateWindow(300, 1064, "Galaga Mothership", NULL, NULL);
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

    std::string fname_prefix = "images/";
    std::string fname_suffix = ".png";
    std::vector<std::string> sprite_fnames = {
        "fighter(64x64)",
        "zako(64x64)",
        "goei(64x64)",
        "boss(64x64)",
        "tombo(64x64)",
        "ogawamushi(64x64)",
        "momiji(64x64)",
        "ei(64x64)",
        "galaxian(64x64)",
        "enterprise(64x64)"
    };
    simple_tex textures[10];

    std::unordered_map<std::string, simple_tex*> sprite_map;

    //for (std::string fname : sprite_fnames) {
    for (size_t i = 0; i < 10; i++) {
        //std::string full_fname = fname_prefix + fname + fname_suffix;
        std::string fname = sprite_fnames[i];
        std::string full_fname = fname_prefix + fname + fname_suffix;
        simple_tex *tex = (textures + i);
        bool success = load_texture_from_file(full_fname.c_str(), tex);
        if (!success) {
            std::cerr << "Bad news!\n";
        }
        sprite_map[fname] = tex;
    }
    
    std::unordered_map<std::string, std::size_t> counter_map;

    for (std::string fname : sprite_fnames) {
        counter_map[fname] = 0;
    }

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

        // 1. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
        {
            #ifdef IMGUI_HAS_VIEWPORT
            auto viewport = ImGui::GetMainViewport();
            ImGui::SetNextWindowPos(viewport->WorkPos);
            ImGui::SetNextWindowSize(viewport->WorkSize);
            ImGui::SetNextWindowViewport(viewport->ID);
            #else
            ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
            ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
            #endif

            // Push down the window rounding style
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);

            float scale_fac = 1.0f;
            ImGui::Begin("Mothership Main View", (bool *)0, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBringToFrontOnFocus);

            auto g = globals.load();
            float accuracy = (float)g.hit_count * 100.0f / (float)g.shot_count;
            ImGui::Text("Score: %ld", g.score);
            ImGui::Text("Credits: %ld", g.credits);
            ImGui::Text("Stage: %ld", g.stage);
            ImGui::Text("Lives: %ld", g.lives);
            ImGui::Text("Shot Count: %ld", g.shot_count);
            ImGui::Text("Hit Count: %ld", g.hit_count);
            ImGui::Text("Accuracy: %0.2f%%", accuracy);

            ImGui::AlignTextToFramePadding();
            for (std::string fname : sprite_fnames) {
                auto sprite = sprite_map[fname];
                // TODO - Adjust text alignment vertically
                ImGui::Image((void*)(intptr_t)sprite->texture, ImVec2(scale_fac * sprite->width, scale_fac * sprite->height));
                auto count = counter_map[fname];
                ImGui::SameLine();
                ImGui::Text("%d", count);
            }

            ImGui::Text("Viewport (w x h): %f x %f", viewport->WorkSize.x, viewport->WorkSize.y);

            ImGui::End();

            // Pop off the window rounding style
            ImGui::PopStyleVar(1);
        }

        // 2. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);

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

    printf("[Mothership.exe]: Exiting...\n");
    return EXIT_SUCCESS;
}