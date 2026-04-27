///
/// @file game_window.hpp
/// @author geffevil
///

#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <GLFW/glfw3.h>

class glpix {

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
        glpix(const std::string& name, int w, int h, bool fullscreen = false, const char* gpu_pixel_func = nullptr, size_t frame_data_buffer_size = 0);
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
        void frame_data(void* data_ptr);

        /* Window */
        inline int win_width() const noexcept { return m_w; }
        inline int win_height() const noexcept { return m_h; }

        inline float time() const noexcept { return m_global_time; }
        
        
    private:
        /* Interactions */
        std::array<uint64_t, 1024> m_key_buffer {},
                                   m_key_delta_buffer {};    
        float m_global_time;

        /* Drawing */
        bool m_buffer_dirty = false;
        color* m_buffer;

        GLuint m_frame_data_buffer;
        GLuint m_texture;
        GLuint m_program;
        GLuint m_vao;

        size_t m_frame_data_buffer_size;        

        /* Window */
        int m_w, m_h;
        GLFWwindow* m_wnd;
};
