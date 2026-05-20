#include <cstdint>
#include <iostream>


class progress_bar {

    public: 
        progress_bar(size_t total_work, int width = 16) : m_total_work(total_work) {
            std::cout << "[";
            for (int i = 0; i < width; i++)
                std::cout << ".";
            std::cout << "]";


            /* Move cursor back */
            std::cout << "\x1b[" << width + 1 << "D#\x1b[1D" << std::flush;

            m_work_per = total_work / width;
        }

        void increment_work() {
            m_work_done++;
            if (m_work_done % m_work_per == 0 && m_work_done > 0) {
                std::cout << "-#" << "\x1b[1D" << std::flush;
            }
    
            if (m_total_work == m_work_done)
                std::cout << "]\x1b[1C done" << std::endl;
        }

    private:
        size_t m_total_work = 0;
        size_t m_work_done = 0;
        size_t m_work_per = 0;
};

int main(void) {

    progress_bar a(INT32_MAX - 1);

    for (int i = 0; i < INT32_MAX - 1; i++)
        a.increment_work();
}