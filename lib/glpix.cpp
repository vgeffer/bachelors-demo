#include "glad/glad.h"
#include "glpix.hpp"

#include <chrono>
#include <fstream>
#include <iostream>
#include <regex>
#include <utility>
#include <vector>
#include <glm/glm.hpp>

#define WORK_GROUP_RATIO 16
#define DIV_ROUND_UP(a, b) ((((a) % (b)) > 0) ? ((a) / (b) + 1) : ((a) / (b)))

#define INIT_STEP(name, func, ...)                                                                      \
    std::cerr << "-- init step " << name << ": ";                                                       \
    try { func(__VA_ARGS__); std::cerr << "done\n"; }                                                   \
    catch(std::exception& e) { std::cerr << "received error(s)\n\n === BEGIN ERROR ===\n" << e.what()   \
                                         << "\n ==== END ERROR ====\n\nEXITING\n"; exit(EXIT_FAILURE); }

#define EXEC_AND_CHECK_RET(func, ...)   \
        if ((err = func(__VA_ARGS__)) != CL_SUCCESS) \
            throw std::runtime_error("On line "  + std::to_string(__LINE__) + " - " + std::string(#func) + "() failed with error code: " + std::to_string(err));

#define EXEC_AND_CHECK_ARG(func, ret, ...) \
        ret = func(__VA_ARGS__, &err); \
        if (!ret || err != CL_SUCCESS) \
            throw std::runtime_error("On line "  + std::to_string(__LINE__) + " - " + std::string(#func) + "() failed with error code: " + std::to_string(err));

        
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

    in vec2 uv;
    out vec4 color;

    void main() {
        color = texture(screen, uv);
    }
)";

/* Template specializations */
template <>
struct glpix::type_helper<glpix::buffer_base> { using type = cl_mem; };

template<typename... T> 
struct glpix::type_helper<glpix::buffer<T...>> {using type = cl_mem; };

glpix::kernel_base::kernel_base(const cl_context& context, const cl_device_id& device, const char* kernel_path, const char* kernel_entry)
    : m_instance_count(new uint(1)) {

    /* Load Source File */
    std::ifstream kernel_source = std::ifstream(kernel_path, std::ios::in);

    if (!kernel_source.is_open())
        throw std::runtime_error(std::string("Unable to open shader file ") + kernel_path);
    
    std::string src_buffer = std::string(
        std::istreambuf_iterator<char>(kernel_source), 
        std::istreambuf_iterator<char>()
    );

    if (!kernel_source.eof() && kernel_source.fail())        
		throw std::runtime_error(std::string("Unable to read from shader file ") + kernel_path);

    kernel_source.close();

    /* Setup OpenCL objects */
    
    cl_int err = 0;    
    const char* src_buffer_c = src_buffer.c_str();
    EXEC_AND_CHECK_ARG(clCreateProgramWithSource, m_program, context, 1, &src_buffer_c, nullptr);
    
    err = clBuildProgram(m_program, 0, nullptr, nullptr, nullptr, nullptr);
    if (err != CL_SUCCESS) {
        
        size_t error_len;
        char buffer[2048];

        clGetProgramBuildInfo(m_program, device, CL_PROGRAM_BUILD_LOG, sizeof(buffer), buffer, &error_len);
        throw std::logic_error(std::string(buffer));
    }

    EXEC_AND_CHECK_ARG(clCreateKernel, m_kernel, m_program, kernel_entry);
    EXEC_AND_CHECK_RET(clGetKernelWorkGroupInfo, m_kernel, device, CL_KERNEL_WORK_GROUP_SIZE, sizeof(size_t), &m_max_work_size, NULL);
}

glpix::glpix(const std::string& name, int w, int h, bool fullscreen) 
    :m_w(w), m_h(h) { 

    INIT_STEP("Window", init_glfw, w, h, name, fullscreen)
    INIT_STEP("OpenGL Context", init_opengl)
    INIT_STEP("OpenCL Context", init_opencl)
    
    //glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
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
    INIT_STEP("Application", [&]() { create() ? 0 : throw std::runtime_error("An error occured in the create() function\n"); })
    glfwShowWindow(m_wnd);

    /* Prepare mainloop */
    std::chrono::system_clock::time_point tp_prev = std::chrono::system_clock::now(), 
                                          tp_now;
    m_global_time = 0;

    size_t local_work_size[2];
    size_t global_work_size[2];

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

        /* Check if app kernel has changed */
        if (m_kernel_changed) {
            size_t work_size_x = (m_active_kernel.max_work_size() > WORK_GROUP_RATIO) ? m_active_kernel.max_work_size() / WORK_GROUP_RATIO : m_active_kernel.max_work_size();
            size_t work_size_y = m_active_kernel.max_work_size() / work_size_x;

            local_work_size[0] = (m_active_kernel.max_work_size() > WORK_GROUP_RATIO) ? m_active_kernel.max_work_size() / WORK_GROUP_RATIO : m_active_kernel.max_work_size();
            local_work_size[1] = m_active_kernel.max_work_size() / work_size_x;

            global_work_size[0] = DIV_ROUND_UP(m_w, local_work_size[0]) * local_work_size[0];
            global_work_size[1] = DIV_ROUND_UP(m_h, local_work_size[1]) * local_work_size[1];
            
            m_kernel_changed = false;

            std::cout << "Kernel changed, recomputing work groups: \n"
                      << "Local work groups size: x=" << local_work_size[0] << ", y=" << local_work_size[1] << "\n"
                      << "Global work groups size: x=" << global_work_size[0] << ", y=" << global_work_size[1] << std::endl;
        }

        /* Acquire OpenGL Objects */
        cl_event acquire_event;

        glFinish();
        clEnqueueAcquireGLObjects(m_cl_queue, 1, &m_texture_mem, 0, nullptr, &acquire_event);
        clWaitForEvents(1, &acquire_event);
        clReleaseEvent(acquire_event);

        /* Process user */
        m_global_time += elapsed;
        if (!update(elapsed))
            glfwSetWindowShouldClose(m_wnd, true);

        /* Render */
        if (m_active_kernel.is_valid())
            dispatch_kernel(m_active_kernel, 2, nullptr, local_work_size, global_work_size).await();

        /* Hand objects back over to OpenGL */
        clEnqueueReleaseGLObjects(m_cl_queue, 1, &m_texture_mem, 0, nullptr, nullptr);
        clFinish(m_cl_queue);

        //glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_w, m_h, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_buffer);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        
        /* Display new frame */
        glfwSwapBuffers(m_wnd);
    }
}

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

glpix::compute_event glpix::dispatch_kernel(const kernel_base& kernel, uint work_dim, const size_t* off, const size_t* local, const size_t* global) {

    cl_int err;
    cl_event dispatch_event;
    EXEC_AND_CHECK_RET(clEnqueueNDRangeKernel, m_cl_queue, kernel, work_dim, off, global, local, 0, nullptr, &dispatch_event);
    return compute_event(dispatch_event);
}

glpix::compute_event glpix::dispatch_kernel(const kernel_base& kernel, uint work_dim, const size_t* off, const size_t* local, const size_t* global, const compute_event& wait_for) {

    cl_int err;
    cl_event dispatch_event, wait_for_event = static_cast<cl_event>(wait_for);
    EXEC_AND_CHECK_RET(clEnqueueNDRangeKernel, m_cl_queue, kernel, work_dim, off, global, local, 1, &wait_for_event, &dispatch_event);
    return compute_event(dispatch_event);
}

void glpix::init_glfw(int w, int h, const std::string& title, bool fullscreen) {

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
        w, h, title.c_str(),
        fullscreen ? glfwGetPrimaryMonitor() : NULL,
        nullptr
    );

    if (m_wnd == NULL)
        throw std::runtime_error("Window not initialized!");    
    glfwMakeContextCurrent(m_wnd);

    /* Init buffer */
    m_buffer = new color[w * h];
    std::memset(m_buffer, 0x00, w * h * sizeof(*m_buffer));

    /* Init interactions */
    static glpix* instance = this;

    glfwSetKeyCallback(m_wnd, [](GLFWwindow*, int key, int scancode, int action, int mods) {
        uint8_t shift = (static_cast<uint16_t>(key) & 0x3F); /* Mask last 6 bits */
        if (action == GLFW_PRESS || action == GLFW_RELEASE)
            instance->m_key_delta_buffer[static_cast<uint16_t>(key) >> 6] |= (1 << shift);
    });
}

void glpix::init_opengl() {

    /* Init OpenGL */
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        throw std::runtime_error("OpenGL context initialization failed!");

    glfwSwapInterval(1);

    GLenum vert = glCreateShader(GL_VERTEX_SHADER),
           frag = glCreateShader(GL_FRAGMENT_SHADER);
	  
	glShaderSource(vert, 1, &VERTEX_SOURCE, nullptr);
	glShaderSource(frag, 1, &FRAGMENT_SOURCE, nullptr);
    
    GLint op_result = GL_FALSE;
    glCompileShader(vert);
    glCompileShader(frag);

    /* Error check for vertex shader */
    glGetShaderiv(vert, GL_COMPILE_STATUS, &op_result);
    if (!op_result) {
        GLint error_len = 0;
        glGetShaderiv(vert, GL_INFO_LOG_LENGTH, &error_len);
        
        std::unique_ptr<char> buffer = std::make_unique<char>(error_len + 1);
        glGetShaderInfoLog(vert, error_len, &error_len, buffer.get());

        glDeleteShader(vert); glDeleteShader(frag); /* These do not get picked up by destructor */
        throw std::logic_error(std::string(buffer.get()));
    }

    /* Error check for fragment shader */
    glGetShaderiv(frag, GL_COMPILE_STATUS, &op_result);
    if (!op_result) {
        GLint error_len = 0;
        glGetShaderiv(frag, GL_INFO_LOG_LENGTH, &error_len);
        
        std::unique_ptr<char> buffer = std::make_unique<char>(error_len + 1);
        glGetShaderInfoLog(frag, error_len, &error_len, buffer.get());

        glDeleteShader(vert); glDeleteShader(frag); /* These do not get picked up by destructor */
        throw std::logic_error(std::string(buffer.get()));
    }

    /* Compilation successful, link shader */
    m_program = glCreateProgram();
    glAttachShader(m_program, vert);
    glAttachShader(m_program, frag);
    glLinkProgram(m_program);

    /* Error check for shader linking */
    glGetProgramiv(m_program, GL_LINK_STATUS, &op_result);
    if (!op_result) {
        GLint error_len = 0;
        glGetProgramiv(m_program, GL_INFO_LOG_LENGTH, &error_len);

        std::unique_ptr<char> buffer = std::make_unique<char>(error_len + 1);
        glGetProgramInfoLog(m_program, error_len, &error_len, buffer.get());
        
        glDeleteShader(vert); glDeleteShader(frag);      
        throw std::logic_error(std::string(buffer.get()));
    }

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
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_w, m_h, 0, GL_RGBA, GL_FLOAT, nullptr);
}

void glpix::init_opencl() {

    cl_platform_id id;
    cl_int err;

    cl_uint platform_count = 0;
    EXEC_AND_CHECK_RET(clGetPlatformIDs, 0, nullptr, &platform_count);
    if (platform_count <= 0) throw std::runtime_error("No viable OpenCL platforms found");

    /* Alloc memory */
    cl_platform_id selected_platform;
    std::vector<cl_platform_id> platforms = std::vector<cl_platform_id>(platform_count);
    EXEC_AND_CHECK_RET(clGetPlatformIDs, platforms.capacity(), platforms.data(), nullptr);

    std::vector<std::pair<cl_platform_id, cl_device_id>> viable_devices;
    for (auto platform : platforms) {
        
        cl_uint platform_dev_count = 0;
        err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 0, nullptr, &platform_dev_count);

        if (err == CL_DEVICE_NOT_FOUND || platform_dev_count == 0)
            continue;

        else if (err != CL_SUCCESS)
            throw std::runtime_error("ASD");

        std::vector<cl_device_id> platform_devices = std::vector<cl_device_id>(platform_dev_count);
        EXEC_AND_CHECK_RET(clGetDeviceIDs, platform, CL_DEVICE_TYPE_ALL, platform_dev_count, platform_devices.data(), nullptr);


        for (const auto& device : platform_devices) {
            if (dev_has_extension(device, CL_GL_INTEROP_EXT))
                viable_devices.emplace_back(std::make_pair(platform, device));
        }
    }

    if (viable_devices.empty())
        throw std::runtime_error("No Viable OpenCL devices found!");

    /* TODO: Selection mechanism */
    selected_platform = viable_devices[0].first;
    m_cl_device       = viable_devices[0].second;

    /* Get device name */
    char dev_name[256];
    clGetDeviceInfo(m_cl_device, CL_DEVICE_NAME, sizeof(dev_name), dev_name, nullptr);
    std::cerr << "Using device: " << dev_name << " - ";

    /* Init OpenCL Interop */
    #if defined(PLATFORM_WINDOWS)
        cl_context_properties cl_ctx_props[] = {
            CL_GL_CONTEXT_KHR, (cl_context_properties)glfwGetWGLContext(window),
            CL_WGL_HDC_KHR, (cl_context_properties)GetDC(glfwGetWin32Window(window)),
            CL_CONTEXT_PLATFORM, (cl_context_properties) selected_platform,
            0
        };
    #elif defined(PLATFORM_UNIX)
        cl_context_properties cl_ctx_props[] = {
            CL_GL_CONTEXT_KHR, (cl_context_properties) glfwGetGLXContext(m_wnd),
            CL_GLX_DISPLAY_KHR, (cl_context_properties) glfwGetX11Display(),
            CL_CONTEXT_PLATFORM, (cl_context_properties) selected_platform,
            0
        };
    #elif defined(PLATFORM_MACOS)
        CGLContextObj cgl_context = CGLGetCurrentContext();
        CGLShareGroupObj cgl_share = CGLGetShareGroup(cgl_context);

        cl_context_properties cl_ctx_props[] = {
            CL_CONTEXT_PROPERTY_USE_CGL_SHAREGROUP_APPLE,
            (cl_context_properties) cgl_share,
            0
        };
    #endif

    EXEC_AND_CHECK_ARG(clCreateContext, m_cl_context, cl_ctx_props, 1, &m_cl_device, NULL, NULL);

    #if defined(PLATFORM_MACOS)
        cl_queue_properties_APPLE props[] = { 0 };
        EXEC_AND_CHECK_ARG(clCreateCommandQueueWithPropertiesAPPLE, m_cl_queue, m_cl_context, m_cl_device, props);
    #else
        EXEC_AND_CHECK_ARG(clCreateCommandQueueWithProperties, m_cl_queue, m_cl_context, m_cl_device, nullptr);
    #endif

    /* Init OGL <-> OCL interop */
    EXEC_AND_CHECK_ARG(clCreateFromGLTexture, m_texture_mem, m_cl_context, CL_MEM_READ_WRITE, GL_TEXTURE_2D, 0, m_texture);
}

bool glpix::dev_has_extension(const cl_device_id& dev, const char* extension) {

    cl_int err;

    size_t args_size;
    EXEC_AND_CHECK_RET(clGetDeviceInfo, dev, CL_DEVICE_EXTENSIONS, 0, nullptr, &args_size);

    std::vector<char> ext_buffer = std::vector<char>(args_size + 1);
    EXEC_AND_CHECK_RET(clGetDeviceInfo, dev, CL_DEVICE_EXTENSIONS, ext_buffer.size(), (void*)ext_buffer.data(), nullptr);

    std::regex ext_regex(extension);
    return std::regex_search(ext_buffer.data(), ext_regex);
}
