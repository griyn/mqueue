#include <functional>
#include <string>
#include <iostream>
#include <thread>
#include "mqueue.h"

::garden::MQueue<std::string> mqueue;

void f1() {
    mqueue.push_back("f1", "HELLO");
    mqueue.push_back("f1", "WORLD");
    mqueue.push_back("f1", "HELLO");
    mqueue.push_back("f1", "FINALLY");
    mqueue.push_back("f1", "END");
    mqueue.push_back("f1", "WWW");
    mqueue.push_back("f1", "XXX");
    mqueue.push_back("f1", "YYY");
    mqueue.push_back("f1", "ZZZ");
}

void f2() {
    mqueue.push_back("f2", "AAA");
    mqueue.push_back("f2", "BBB");
    mqueue.push_back("f2", "CCC");
    mqueue.push_back("f2", "DDD");
    mqueue.push_back("f2", "EEE");
}

void f3() {
    for (int i : std::vector<int>(3, 0)) {
        std::string read;
        auto* done = mqueue.new_closure();
        if (mqueue.get_front(&read, done)) {
            std::cout << read << std::endl;
            done->run();
        }
    }

    auto* done = mqueue.new_closure();
    std::vector<std::string> reads;
    if (mqueue.get_front_batch(&reads, -1, done)) {
        for (auto& read : reads) {
            std::cout << read << std::endl;
        }
        done->run();
    }

}

int main() {
    std::thread t1(f1);
    std::thread t2(f2);
    std::thread t3(f3);
    std::thread t4(f3);

    t1.join();
    t2.join();
    t3.join();
    t4.join();
    return 0;
}
