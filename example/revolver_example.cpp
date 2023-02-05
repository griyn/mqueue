#include <iostream>
#include <string>
#include <thread>
#include "revolver.h"

std::atomic<uint32_t> g_write_count {0};
std::atomic<uint32_t> g_read_count {0};

void write(::garden::Revolver<std::string>* revolver, const std::string& name) {
    for (int i = 0; i < 100; ++i) {
        std::stringstream ss;
        ss << name << ":" << std::to_string(i);
        std::string str = ss.str();
        if (revolver->push(std::move(str))) {
            ++g_write_count;
        }
    }
}

void read(::garden::Revolver<std::string>* revolver, const std::string& name) {
    std::string read;
    for (int i = 0; i < 1000; ++i) {
        if (revolver->pop(&read)) {
            ++g_read_count;
            std::cout << name << ">" << read << " ";
        }
    }
}

int main() {
    garden::Revolver<std::string> revolver(500);

    std::cout << revolver.debug() << std::endl;

    std::thread r1(read, &revolver, "r1");
    std::thread w1(write, &revolver, "w1");
    std::thread r2(read, &revolver, "r2");
    std::thread w2(write, &revolver, "w2");
    std::thread r3(read, &revolver, "r3");
    std::thread w3(write, &revolver, "w3");
    std::thread r4(read, &revolver, "r4");
    std::thread w4(write, &revolver, "w4");
    std::thread r5(read, &revolver, "r5");
    std::thread w5(write, &revolver, "w5");

    r1.join();
    r2.join();
    r3.join();
    r4.join();
    r5.join();
    w1.join();
    w2.join();
    w3.join();
    w4.join();
    w5.join();

    std::string read;
    ::read(&revolver, "last");

    std::cout << "\ntotal write: " << g_write_count.load() 
            << ", total read: " << g_read_count.load() << std::endl;

    return 0;
}
