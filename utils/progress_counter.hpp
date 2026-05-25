#pragma once
#include <iostream>
#include <string>


class progress_counter {

    public: 
        progress_counter() = default;
        progress_counter(const char* action,  const std::string& name = "")
            : m_action(action) {
            
            std::string output = "0" + std::string(m_action);
            m_last_len = output.size();
            
            std::cerr << name << " " << output << std::flush;
        }

        void increment_work() {
            m_work_done++;

            std::cerr << "\x1b[" << m_last_len << "D" << std::flush;
            std::string output = std::to_string(m_work_done) + m_action;
            m_last_len = output.size();
            
            std::cerr << output << std::flush;
        }

        void end_work() {
            increment_work();
            std::cout << std::endl;
        }

    private:
        size_t m_work_done  = 0;
        size_t m_last_len = 0;
        const char* m_action;
};