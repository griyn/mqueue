#pragma once

#include <atomic>                   // std::atomic
#include <stdint.h>                 // int*_t
#include <vector>                   // std::vector
#include <cstddef>                  // size_t
#include <memory>

namespace garden {

template<typename T>
class Revolver {
public:
    Revolver(uint32_t capacity_hint);

    // 实际生成的capacity容量是2的倍数
    uint32_t capacity();

    std::string debug();

    // try型接口，
    bool push(T data);
    
    bool pop(T* data);

private:
    struct Slot {
        T data;
        std::atomic<uint32_t> version {0};
    };

private:
    void init(uint32_t capacity_hint);

private:
    uint32_t _capacity_hint;

    std::vector<Slot> _slots;
    uint32_t _slot_mask; // 用于通过index生成真正的槽位索引
    uint32_t _slot_bits; // 用于通过index生成槽位版本号

    std::atomic<uint32_t> _next_push_index {0};
    std::atomic<uint32_t> _next_pop_index {0};
};

} // namespace garden

#include "revolver.hpp"