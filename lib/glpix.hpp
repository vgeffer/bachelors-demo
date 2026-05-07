///
/// @file game_window.hpp
/// @author geffevil
///

#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <GLFW/glfw3.h>
#include <utility>


/* Platform Resolution Macros */
#if defined(_WIN32) || defined(WIN32) || defined(__CYGWIN__) || defined(__MINGW32__) || defined(__BORLANDC__)
    #define PLATFORM_WINDOWS
#elif defined(__unix__) || defined(__linux__) || defined(__FreeBSD__)
    #define PLATFORM_UNIX
#elif defined(__APPLE__) || defined(__MACH__)
    #include <OpenCL/cl.h>
    #include <OpenCL/cl_gl.h>
    #include <OpenCL/gcl.h>
    #include <OpenGL/CGLCurrent.h>
    #define PLATFORM_MACOS
#else
    #error Unknown or Unsupported Platform
#endif

class glpix {

    private:
        template <typename T>
        struct type_helper { using type = T; }; 

        class buffer_base {
            public:
                buffer_base() = default;
                buffer_base(buffer_base&& other)
                    : m_memory_buffer(other.m_memory_buffer), m_instance_count(other.m_instance_count) { other.m_instance_count = nullptr; }
                buffer_base(const buffer_base& other)
                    : m_memory_buffer(other.m_memory_buffer), m_instance_count(other.m_instance_count) { if (m_instance_count) (*m_instance_count)++; }

                virtual ~buffer_base() {
                    /* Object moved */
                    if (!m_instance_count) 
                        return;
                    
                    (*m_instance_count)--;
                    if (*m_instance_count == 0) {

                        //(m_program);
                        delete m_instance_count;
                    }
                }

                constexpr operator cl_mem() const noexcept { return m_memory_buffer; }                
                constexpr buffer_base& operator=(const buffer_base& other) noexcept {

                    m_memory_buffer = other.m_memory_buffer;
                    m_instance_count = other.m_instance_count;

                    if (m_instance_count)
                        (*m_instance_count)++;
                    return *this;
                }

            protected:
                uint* m_instance_count = nullptr;
                cl_mem m_memory_buffer = nullptr;
        };

        class kernel_base {
            public:
                kernel_base() = default;
                kernel_base(kernel_base&& other)
                    : m_max_work_size(other.m_max_work_size), m_program(other.m_program), m_kernel(other.m_kernel),
                      m_instance_count(other.m_instance_count) { other.m_instance_count = nullptr; }
                kernel_base(const kernel_base& other)
                    : m_max_work_size(other.m_max_work_size), m_program(other.m_program), m_kernel(other.m_kernel), 
                      m_instance_count(other.m_instance_count) { if (m_instance_count) (*m_instance_count)++; }

                virtual ~kernel_base() {
                    /* Object moved */
                    if (!m_instance_count) 
                        return;
                    
                    (*m_instance_count)--;
                    if (*m_instance_count == 0) {
                        clReleaseKernel(m_kernel);
                        clReleaseProgram(m_program);
                        delete m_instance_count;
                    }
                }

                const bool is_valid() const noexcept { return m_program && m_kernel && m_instance_count; }
                const size_t max_work_size() const noexcept { return m_max_work_size; }
                constexpr operator cl_kernel() const noexcept { return m_kernel; }                
                constexpr kernel_base& operator=(const kernel_base& other) noexcept {
                    m_program = other.m_program;
                    m_kernel = other.m_kernel;
                    m_max_work_size = other.m_max_work_size;
                    m_instance_count = other.m_instance_count;

                    if (m_instance_count)
                        (*m_instance_count)++;
                    return *this;
                }


            protected:
                explicit kernel_base(const cl_context& context, const cl_device_id& device, const char* kernel_path, const char* kernel_entry);

                /* TODO: Do is_valid check */
                template <typename T> void set_arg(uint idx, const T& arg) {
                    cl_int err = clSetKernelArg(m_kernel, idx, sizeof(T), &arg);
                    if (err != CL_SUCCESS)
                        throw std::runtime_error("clSetKernelArg() failed with error code: " + std::to_string(err));
                }
            
                uint*      m_instance_count = nullptr;
                size_t     m_max_work_size  = 0;
                cl_program m_program        = nullptr;
                cl_kernel  m_kernel         = nullptr;
        };

    public:
        enum class key : uint16_t {
            A = GLFW_KEY_A, B = GLFW_KEY_B, C = GLFW_KEY_C,
            D = GLFW_KEY_D, E = GLFW_KEY_E, F = GLFW_KEY_F,
            G = GLFW_KEY_G, H = GLFW_KEY_H, I = GLFW_KEY_I,
            J = GLFW_KEY_J, K = GLFW_KEY_K, L = GLFW_KEY_L,
            M = GLFW_KEY_M, N = GLFW_KEY_N, O = GLFW_KEY_O,
            P = GLFW_KEY_P, Q = GLFW_KEY_Q, R = GLFW_KEY_R,
            S = GLFW_KEY_S, T = GLFW_KEY_T, U = GLFW_KEY_U,
            V = GLFW_KEY_V, W = GLFW_KEY_W, X = GLFW_KEY_X,
            Y = GLFW_KEY_Y, Z = GLFW_KEY_Z,
    
            UP_ARROW     = GLFW_KEY_UP, DOWN_ARROW   = GLFW_KEY_DOWN,
            LEFT_ARROW   = GLFW_KEY_LEFT, RIGHT_ARROW  = GLFW_KEY_RIGHT,
    
            NUM0 = GLFW_KEY_0, NUM1 = GLFW_KEY_1,
            NUM2 = GLFW_KEY_2, NUM3 = GLFW_KEY_3,
            NUM4 = GLFW_KEY_4, NUM5 = GLFW_KEY_5,
            NUM6 = GLFW_KEY_6, NUM7 = GLFW_KEY_7,
            NUM8 = GLFW_KEY_8, NUM9 = GLFW_KEY_9,

            F1 = GLFW_KEY_F1, F2 = GLFW_KEY_F2, F3 = GLFW_KEY_F3,
            F4 = GLFW_KEY_F4, F5 = GLFW_KEY_F5, F6 = GLFW_KEY_F6,
            F7 = GLFW_KEY_F7, F8 = GLFW_KEY_F8, F9 = GLFW_KEY_F9,
    
            BACKSPACE = GLFW_KEY_BACKSPACE,
            SPACE = GLFW_KEY_SPACE,
            LSHIFT = GLFW_KEY_LEFT_SHIFT,
            RSHIFT = GLFW_KEY_RIGHT_SHIFT,
            LCTRL = GLFW_KEY_LEFT_CONTROL,
            RCTRL = GLFW_KEY_RIGHT_CONTROL,
            LALT = GLFW_KEY_LEFT_ALT,
            RALT = GLFW_KEY_RIGHT_ALT,
            ENTER = GLFW_KEY_ENTER,
            DEL = GLFW_KEY_DELETE,
            ESC = GLFW_KEY_ESCAPE
        };

        struct color {

            constexpr color() : c(0x00000000)  {}
            constexpr color(uint32_t color) : c(color) {}
            constexpr color(uint8_t r, uint8_t g, uint8_t b, uint8_t a) : r(r), g(g), b(b), a(a) {}
            constexpr color(float r, float g, float b) 
                : r(static_cast<uint8_t>(r * 255)), g(static_cast<uint8_t>(g * 255)), 
                  b(static_cast<uint8_t>(b * 255)), a(static_cast<uint8_t>(255)) {}

            union {
                uint32_t c;
                struct {
                    uint8_t r, g, b, a;
                };
            };
        };

        template<typename T> class buffer : public buffer_base {
            


        };

        template <typename ...T> class kernel : public kernel_base {
            public:
                template <uint N> using nth_arg_type = typename glpix::type_helper<typename std::tuple_element<N, std::tuple<T...>>::type>::type;

            public:
                kernel() : kernel_base() {}
                kernel(const kernel& other) : kernel_base(other) {}
                kernel(kernel&& other) : kernel_base(other) {}

                constexpr kernel<T...>& operator=(const kernel<T...>& other) noexcept {
                    m_program = other.m_program;
                    m_kernel = other.m_kernel;
                    m_max_work_size = other.m_max_work_size;
                    m_instance_count = other.m_instance_count;

                    if (m_instance_count)
                        (*m_instance_count)++;
                    return *this;
                }

                template <uint N> void set_arg(const nth_arg_type<N>& arg) { kernel_base::set_arg<nth_arg_type<N>>(N, static_cast<nth_arg_type<N>>(arg)); }

            private:
                kernel(const cl_context& context, const cl_device_id& device, const char* kernel_path, const char* kernel_entry)
                    : kernel_base(context, device, kernel_path, kernel_entry) {}
                
                kernel(const cl_context& context, const cl_device_id& device, const char* kernel_path, const char* kernel_entry, const T&... args)
                    : kernel_base(context, device, kernel_path, kernel_entry) { rec_set_args(0, args...); }

                template<typename A> inline void rec_set_args(uint idx, const A& arg) { kernel_base::set_arg(idx, arg); }
                template<typename A, typename... B> inline void rec_set_args(uint idx, const A& arg, const B&... rest) { kernel_base::set_arg(idx, arg); rec_set_args(idx + 1, rest...); }
            
            friend class glpix;
        };

        class compute_event {
            public:

                compute_event() = default;
                compute_event(compute_event&& other)
                    : m_envent(other.m_envent), m_instance_count(other.m_instance_count) { other.m_instance_count = nullptr; }
                compute_event(const compute_event& other)
                    :  m_envent(other.m_envent), m_instance_count(other.m_instance_count) { if (m_instance_count) (*m_instance_count)++; }

                virtual ~compute_event() {
                    /* Object moved */
                    if (!m_instance_count) 
                        return;
                    
                    (*m_instance_count)--;
                    if (*m_instance_count == 0) {

                        clReleaseEvent(m_envent); 
                        delete m_instance_count;
                    }
                }

                constexpr operator cl_event() const noexcept { return m_envent; }
                constexpr compute_event& operator=(const compute_event& other) noexcept {

                    m_envent = other.m_envent;
                    m_instance_count = other.m_instance_count;

                    if (m_instance_count)
                        (*m_instance_count)++;
                    return *this;
                }

                void await() { clWaitForEvents(1, &m_envent); }
                const bool is_valid() { return m_envent; }

            private:
                compute_event(const cl_event& event) : m_envent(event) { m_instance_count = new uint(1);}    

                uint* m_instance_count = nullptr;
                cl_event m_envent      = nullptr;
            
            friend class glpix;
        };


    public:
        inline static const color BLACK   = color(0x00, 0x00, 0x00, 0xFF);
        inline static const color RED     = color(0xFF, 0x00, 0x00, 0xFF);
        inline static const color GREEN   = color(0x00, 0xFF, 0x00, 0xFF);
        inline static const color YELLOW  = color(0xFF, 0xFF, 0x00, 0xFF);
        inline static const color BLUE    = color(0x00, 0x00, 0xFF, 0xFF);
        inline static const color MAGENTA = color(0xFF, 0x00, 0xFF, 0xFF);
        inline static const color CYAN    = color(0x00, 0xFF, 0xFF, 0xFF);
        inline static const color WHITE   = color(0xFF, 0xFF, 0xFF, 0xFF);
            
    public:
        glpix(const std::string& name, int w, int h, bool fullscreen = false);
        virtual ~glpix();

        void start();

    protected:
        virtual bool create() { return false; }
        virtual bool update(float delta) { return false; }

        /* Interactions */
        bool key_pressed(key k);
        bool key_held(key k);
        bool key_released(key k);

        /* Drawing */
        void clear(const color& c) noexcept;
        void draw(int x, int y, const color& c);

        /* Window */
        inline int win_width() const noexcept { return m_w; }
        inline int win_height() const noexcept { return m_h; }

        inline float time() const noexcept { return m_global_time; }
        inline const cl_mem& gpu_screen_buffer() const noexcept { return m_texture_mem; }

        template <typename T>
        buffer<T> create_buffer() {}

        template <typename ...T>
        kernel<T...> create_kernel(const char* source_path, const char* kernel_entry = "kernel") { 
            return glpix::kernel<T...>(m_cl_context, m_cl_device, source_path, kernel_entry);
        }

        template <typename ...T>
        kernel<T...> create_kernel(const char* source_path, const char* kernel_entry, const T&... args) {
            return glpix::kernel<T...>(m_cl_context, m_cl_device, source_path, kernel_entry, args...);
        }

        inline void draw_kernel(const kernel_base& kern) { m_kernel_changed = true; m_active_kernel = kern; }
        
        compute_event dispatch_kernel(const kernel_base& kernel, uint work_dim, const size_t* off, const size_t* local, const size_t* global);
        compute_event dispatch_kernel(const kernel_base& kernel, uint work_dim, const size_t* off, const size_t* local, const size_t* global, const compute_event& wait_for);

    private:
        void init_glfw(int w, int h, const std::string& title, bool fullscreen);
        void init_opengl();
        void init_opencl();

    private:
        /* Interactions */
        std::array<uint64_t, 1024> m_key_buffer {},
                                   m_key_delta_buffer {};    
        float m_global_time;

        /* Drawing */
        bool m_buffer_dirty = false;
        color* m_buffer;

        GLuint m_texture;
        GLuint m_program;
        GLuint m_vao;

        /* Window */
        int m_w, m_h;
        GLFWwindow* m_wnd;

        /* OpenCL */
        cl_context m_cl_context;
        cl_device_id m_cl_device;
        cl_command_queue m_cl_queue;

        cl_mem m_texture_mem;
        bool m_kernel_changed = false;
        kernel_base m_active_kernel;
};
