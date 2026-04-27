#include "glad/glad.h"
#include "glpix.hpp"

#include <chrono>
#include <cstddef>
#include <cstring>
#include <glm/glm.hpp>
#include <iostream>


#define CHECK_SHADER_RESULT(shader) \
    GLint result_##shader;          \
    glGetShaderiv(shader, GL_COMPILE_STATUS, &result_##shader);     \
    if (result_##shader == GL_FALSE)  {                              \
        GLint error_len = 0;                                         \
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &error_len);       \
        char* buffer = new char[error_len + 1];                      \
        glGetShaderInfoLog(shader, error_len, &error_len, buffer);     \
        std::cerr << "[ERROR] " << #shader << ": " << buffer << ":\n";\
        glDeleteShader(vert);                                        \
        glDeleteShader(frag);                                        \
        delete[] buffer;                                             \
        throw std::runtime_error("Shader compilation error");        \
    }                                                                   

#define CHECK_PROGRAM_RESULT(program) \
    GLint result_##program;          \
    glGetProgramiv(program, GL_LINK_STATUS, &result_##program);     \
    if (result_##program == GL_FALSE)  {                              \
        GLint error_len = 0;                                         \
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &error_len);       \
        char* buffer = new char[error_len + 1];                      \
        glGetProgramInfoLog(program, error_len, &error_len, buffer);     \
        std::cerr << "[ERROR] " << #program << " " << buffer << ":\n";\
        glDeleteShader(vert);                                        \
        glDeleteShader(frag);                                        \
        glDeleteProgram(m_program);                                  \
        delete[] buffer;                                             \
        throw std::runtime_error("Shader compilation error");        \
    }  

const char* VERTEX_SOURCE = R"(
    #version 330 core
    
    const vec2 vertices[] = vec2[6](
        vec2(1.0f,  1.0f), vec2(1.0, -1.0f), vec2(-1.0f,  1.0f),
        vec2(1.0f, -1.0f), vec2(-1.0f, -1.0f), vec2(-1.0f,  1.0f)
    );
    
    out vec2 uv;
    void main() {
        uv = (vertices[gl_VertexID] + vec2(1.0f, 1.0f)) / 2.0f;
        gl_Position = vec4(vertices[gl_VertexID], 0.0f, 1.0);
    }
)";

const char* FRAGMENT_SOURCE = R"(
    #version 330 core

    uniform sampler2D screen;
    uniform float time;

    in vec2 uv;
    out vec4 color;

    void main() {
        color = texture(screen, uv);
    }
)";


const char* GPUPIX_TEMPLATE1 = R"(
    #version 330 core

    uniform sampler2D screen;
    uniform float time;

    in vec2 uv;
    out vec4 color;
)";

const char* GPUPIX_TEMPLATE2 = R"(
    void main() {
        color = pixel();
    }
)";

glpix::glpix(const std::string& name, int w, int h, bool fullscreen, const char* gpu_pixel_func, size_t frame_data_buffer_size) 
    :m_w(w), m_h(h), m_frame_data_buffer_size(0) { 

    /* Init window */
    if (!glfwInit())
        throw std::runtime_error("GLFW context initialization failed!");

    glfwSetErrorCallback([](int code, const char* reason) { std::cerr << "(" << code << ") ERROR: " << reason << std::endl; });
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); 
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    m_wnd = glfwCreateWindow(
        w, h, name.c_str(),
        fullscreen ? glfwGetPrimaryMonitor() : NULL,
        nullptr
    );

    if (m_wnd == NULL)
        throw std::runtime_error("Window not initialized!");    
    glfwMakeContextCurrent(m_wnd);

    /* Init buffer */
    m_buffer = new color[w * h];
    std::memset(m_buffer, 0x00, w * h * sizeof(*m_buffer));

    /* Init OpenGL */
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        throw std::runtime_error("OpenGL context initialization failed!");

    glfwSwapInterval(1);


    GLenum vert = glCreateShader(GL_VERTEX_SHADER),
           frag = glCreateShader(GL_FRAGMENT_SHADER);
	  
	glShaderSource(vert, 1, &VERTEX_SOURCE, nullptr);

    if (gpu_pixel_func == nullptr)
	    glShaderSource(frag, 1, &FRAGMENT_SOURCE, nullptr);

    else {
        const char* shaders[3] = {GPUPIX_TEMPLATE1, gpu_pixel_func, GPUPIX_TEMPLATE2};
        glShaderSource(frag, 3, shaders, nullptr);
    }

    glCompileShader(vert);
    glCompileShader(frag);

    CHECK_SHADER_RESULT(vert)    
    CHECK_SHADER_RESULT(frag)

    /* Compilation successful, link shader */
    m_program = glCreateProgram();
    glAttachShader(m_program, vert);
    glAttachShader(m_program, frag);
    glLinkProgram(m_program);

    CHECK_PROGRAM_RESULT(m_program)
    glUseProgram(m_program);

    /* Clean up */
    glDeleteShader(vert);
    glDeleteShader(frag);

    glGenVertexArrays(1, &m_vao);
    glBindVertexArray(m_vao);

    glGenTextures(1, &m_texture);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_texture);  

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    if (gpu_pixel_func) {

        /* Also prepare frame data buffer */
        glGenBuffers(1, &m_frame_data_buffer);
        glBindBuffer(GL_UNIFORM_BUFFER, m_frame_data_buffer);
        glBufferData(GL_UNIFORM_BUFFER, frame_data_buffer_size, nullptr, GL_DYNAMIC_DRAW);

        GLuint buffer_idx = glGetUniformBlockIndex(m_program, "frame_data");   
        glUniformBlockBinding(m_program, buffer_idx, 1);
        glBindBufferBase(GL_UNIFORM_BUFFER, 1, m_frame_data_buffer); 
        m_frame_data_buffer_size = frame_data_buffer_size;
    }

    /* Init interactions */
    static glpix* instance = this;

    glfwSetKeyCallback(m_wnd, [](GLFWwindow*, int key, int scancode, int action, int mods) {
        uint8_t shift = (static_cast<uint16_t>(key) & 0x3F); /* Mask last 6 bits */
        if (action == GLFW_PRESS || action == GLFW_RELEASE)
            instance->m_key_delta_buffer[static_cast<uint16_t>(key) >> 6] |= (1 << shift);
    });

    glfwShowWindow(m_wnd);
}
glpix::~glpix() {
    
    /* Destroy OpenGL */
    glDeleteProgram(m_program);
    glDeleteTextures(1, &m_texture);

    /* Destroy Buffer */
    delete[] m_buffer;

    /* Destroy Window */
    glfwHideWindow(m_wnd);
    glfwDestroyWindow(m_wnd);

    glfwTerminate();
}

void glpix::start() {

    /* Create user */
    if (!create())
        return; /* Log Error */

    /* Prepare mainloop */
    std::chrono::system_clock::time_point tp_prev = std::chrono::system_clock::now(), 
                                          tp_now;
    m_global_time = 0;
    const int time_uniform_location = glGetUniformLocation(m_program, "time");

    /* Mainloop */
    while (!glfwWindowShouldClose(m_wnd)) {
        
        /* Process interactions */
        for (int i = 0; i < m_key_buffer.size(); i++) {
            m_key_buffer[i] ^= m_key_delta_buffer[i];   /* Transfer delta buffer to main key buffer */
            m_key_delta_buffer[i] = 0;                  /* Zero out delta buffer */
        }

        glfwPollEvents();

        /* Calculate time elapsed since last frame */
        tp_now = std::chrono::system_clock::now();
        float elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(tp_now - tp_prev).count() / 1000.0f;
        tp_prev = tp_now;        

        /* Process user */
        m_global_time += elapsed;
        if (!update(elapsed))
            break; /* Update signaled window should close */

        /* Render */
        glUniform1f(time_uniform_location, m_global_time);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_w, m_h, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_buffer);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        /* Display new frame */
        glfwSwapBuffers(m_wnd);
    }
}

/* Interactions */
bool glpix::key_pressed(glpix::key k) {        
        
    uint64_t mask = 1ull << (static_cast<uint16_t>(k) & 0x3F);
    
    return !((m_key_buffer[static_cast<uint16_t>(k) >> 6]) & mask) &&
           ((m_key_delta_buffer[static_cast<uint16_t>(k) >> 6]) & mask); 
}

bool glpix::key_held(glpix::key k) {

    uint64_t mask = 1ull << (static_cast<uint16_t>(k) & 0x3F);

    return ((m_key_buffer[static_cast<uint16_t>(k) >> 6]) & mask) &&
           !((m_key_delta_buffer[static_cast<uint16_t>(k) >> 6]) & mask);
}

bool glpix::key_released(glpix::key k) {
    
    uint64_t mask = 1ull << (static_cast<uint16_t>(k) & 0x3F);

    return ((m_key_buffer[static_cast<uint16_t>(k) >> 6]) & mask) &&
           ((m_key_delta_buffer[static_cast<uint16_t>(k) >> 6]) & mask); 
}

void glpix::clear(const glpix::color& c) noexcept {
    memset(m_buffer, static_cast<int>(c.c), m_w * m_h * sizeof(*m_buffer));
}

void glpix::draw(int w, int h, const glpix::color& c) {
    m_buffer[h * m_w + w] = c;
}

void glpix::frame_data(void* data_ptr) {

    if (m_frame_data_buffer_size == 0 || data_ptr == nullptr) return;
    glBufferSubData(GL_UNIFORM_BUFFER, 0, m_frame_data_buffer_size, data_ptr);
}
