#include "pistol.h"
#include <string>
#include <sstream>
#include <iostream>

void batch_read(::garden::Pistol<std::string>::TaskIterator& iter) {
    std::cout << ">>>>>>batch_read>>>>>>";
    for (; !iter.is_end(); ++iter) {
        std::cout << *iter << " ";
    }
    std::cout << std::endl;
}

void write(::garden::Pistol<std::string>* pistol, const std::string& name, int t_ms) {
    for (int i = 0; i < 100; ++i) {
        std::stringstream ss;
        ss << name << ":" << std::to_string(i);
        std::string str = ss.str();
        pistol->push(std::move(str));
    }
}

int main() {
    ::garden::Pistol<std::string> pistol(500, std::bind(batch_read, std::placeholders::_1));

    std::thread w1(write, &pistol, "w1", 1);
    std::thread w2(write, &pistol, "w2", 2);
    std::thread w3(write, &pistol, "w3", 3);
    std::thread w4(write, &pistol, "w4", 2);
    std::thread w5(write, &pistol, "w5", 1);

    w1.join();
    w2.join();
    w3.join();
    w4.join();
    w5.join();

    pistol.graceful_stop_and_join(-1);
    return 0;
}
