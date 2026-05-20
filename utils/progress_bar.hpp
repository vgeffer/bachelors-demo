#pragma once
#include <iostream>


class progress_bar {

    public: 
        progress_bar() = default;
        progress_bar(size_t total_work, const std::string name = "", size_t width = 64)
            : m_total_work(total_work) {

            width = std::min(width, total_work);
            std::cerr << name << " [";
            for (int i = 0; i < width; i++)
                std::cerr << ".";
            std::cerr << "]";
            
            /* Move cursor back */
            std::cerr << "\x1b[" << width + 1 << "D>\x1b[1D" << std::flush;
            
            m_work_per = total_work / width;
        }

        void increment_work() {
            if (m_total_work == 0) return;

            m_work_done++;
            if (m_work_done % m_work_per == 0 && m_work_done > 0)
                std::cerr << "=>" << "\x1b[1D" << std::flush;
    
            if (m_total_work == m_work_done)
                std::cerr << "]\x1b[1Cdone" << std::endl;
        }

    private:
        size_t m_total_work = 0;
        size_t m_work_done  = 0;
        size_t m_work_per   = 0;
};