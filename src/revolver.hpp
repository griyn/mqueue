#pragma once

#include "revolver.h"

#include <sstream>
#include <bitset>

namespace garden {

template<typename T>
Revolver<T>::Revolver(uint32_t capacity_hint) : 
        _capacity_hint(capacity_hint) {
    init(capacity_hint);
}

template<typename T> 
void Revolver<T>::init(uint32_t capacity_hint) {
    // 实际生成的capacity容量是2的倍数，以支持用mask找到对应槽位。
    _slot_bits = 1;
    while (capacity_hint > 2) {
        ++_slot_bits;
        capacity_hint >>= 1;
    }
    uint32_t capacity = 1 << _slot_bits;
    _slot_mask = capacity - 1;

    // 这里避免使用resize方法导致DefaultInsertable插入数据
    std::vector<Slot> tmp {capacity};
    _slots.swap(tmp);
}

template<typename T> 
uint32_t Revolver<T>::capacity() {
    return _slots.size();
}

template<typename T> 
std::string Revolver<T>::debug() {
    std::stringstream ss;
    ss << "hint:" << _capacity_hint 
        << "\t" << std::bitset<8>(_capacity_hint)
        << "\tcapacity:" << capacity() 
        << "\tmask:" << _slot_mask 
        << "\tbits:" << _slot_bits;
    return ss.str();
}

template<typename T> 
bool Revolver<T>::push(T data) {
    uint32_t index = _next_push_index.load(std::memory_order_acquire);
    Slot* slot;
    uint32_t expected_version;
    do {
        // 逻辑索引找到对应物理slot
        slot = &_slots[index & _slot_mask];
        // 逻辑索引找到对应push版本
        // push检查的当前版本是偶数，push成功后是奇数
        expected_version = (index >> _slot_bits) << 1;
        uint32_t current_version = slot->version.load(std::memory_order_acquire);
        if (current_version < expected_version 
                || current_version == UINT32_MAX) {
            // 添加元素失败，队列已满
            // if current_version < expected_version: 
            //   index顺序递增情况下，push指针即将超过pop指针
            // if current_version > expected_version: 
            //   1. 当前slot已被占用，后续的compare_exchange_weak操作一定会失败，下次循环尝试
            //   2. index溢出情况下，push指针即将超过pop指针（此时的当前版本是uint32max，是奇数）
            return false;
        }
        // 移动push指针，确认占用当前slot
    } while (!_next_push_index.compare_exchange_weak(index, index + 1, 
            std::memory_order_release));
    
    slot->data = std::move(data);
    // 数据生效barrier, 版本更新后slot可读
    slot->version.store(expected_version + 1, std::memory_order_release);

    return true;
}

template<typename T> 
bool Revolver<T>::pop(T* data) {
    uint32_t index = _next_pop_index.load(std::memory_order_acquire);
    Slot* slot;
    uint32_t expected_version;
    do {
        // 逻辑索引找到对应物理slot
        slot = &_slots[index & _slot_mask];
        // 逻辑索引找到对应push版本
        // pop检查的当前版本是奇数，pop成功后是偶数
        expected_version = ((index >> _slot_bits) << 1) + 1;
        uint32_t current_version = slot->version.load(std::memory_order_acquire);
        if (current_version < expected_version) {
            // 添加元素失败，队列空
            // 不会有溢出的问题
            return false;
        }
        // 移动push指针，确认释放当前slot
    } while (!_next_pop_index.compare_exchange_weak(index, index + 1, 
            std::memory_order_release));
    
    *data = std::move(slot->data);
    // 数据生效barrier, 版本更新后slot可写
    slot->version.store(expected_version + 1, std::memory_order_release);

    return true;
}

} // namespace garden