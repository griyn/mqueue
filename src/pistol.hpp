#include "pistol.h"

#ifndef likely
#define likely(x) __builtin_expect(!!(x), 1) 
#endif // likely

#ifndef unlikely
#define unlikely(x) __builtin_expect(!!(x), 0)
#endif // unlikely

template<typename T>
int64_t Pistol<T>::size() {
    return TaskNode::s_size.load(std::memory_order_acquire);
}

template<typename T>
int64_t Pistol<T>::capacity() {
    return _capacity;
}

template<typename T>
bool Pistol<T>::started() {
    return _started.load(std::memory_order_acquire);
}

template<typename T>
void Pistol<T>::graceful_stop_and_join(int wait_until_ms) {
    auto start = std::chrono::system_clock::now();
    while (size() > 0) {
        auto now = std::chrono::system_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - start);
        if (wait_until_ms != -1 && duration.count() >= wait_until_ms) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    _started.store(false, std::memory_order_release);
    if (_worker.joinable()) {
        // thread不可以多次join，所以stop_and_join不能并发执行
        _worker.join();
        destory_queue_range(_head, _tail);
    }
}

template<typename T>
bool Pistol<T>::post(T data) {
    if (size() >= capacity() || !started()) {
        return false;
    }

    NodeBase* node = new TaskNode(std::move(data));
    
    // wait-free 地向队列头添加节点。可能会有暂时的断链情况，多线程并发情况下可能会有多处断链。
    // 向队列添加数据只会修改链表头节点，不影响尾结点
    NodeBase* old_head = _head.exchange(node, std::memory_order_release);
    node->next = old_head;
    old_head->prev = node;
    return true;
}

template<typename T>
void Pistol<T>::_work_loop() {
    while (true) {
        while (size() <= 0 && started()) {
            // 此时是空队列，不数据处理
            std::this_thread::yield();
        }

        if (!started()) {
            break;
        }

        // 获取待执行的node列表
        NodeBase* snap_end = _END_NODE->prev;
        _END_NODE->prev = nullptr;
        // 和向队列添加node方式相同，但此时不需要链接后面的节点
        NodeBase* snap_head = _head.exchange(_END_NODE, std::memory_order_release);
        if (unlikely(snap_head == _END_NODE || snap_end == nullptr)) {
            // double check 空队列，队列有数据时快照不应该只有end节点
            throw std::overflow_error("Internal Error in snap empty check");
        }

        TaskIterator iter(snap_head, snap_end);
        _work_func(iter);

        destory_queue_range(snap_head, snap_end);
    }
}

template<typename T>
void Pistol<T>::destory_queue_range(NodeBase* head, NodeBase* tail) {
    if (unlikely(head == nullptr || tail == nullptr)) {
        return;
    }

    while (tail != head) {
        if (unlikely(tail == nullptr)) {
            if (tail->is_iteratored == true) {
                // 已遍历过的节点不应该为空
                throw std::overflow_error("Internal Error in destory node nullptr check");
            } else {
                // 可能是主动销毁节点时有断链的情况
                // 这种情况只可能发生在未消费即销毁的节点，比如队列强行退出时
                std::this_thread::yield();
            }
        }
        auto* ptr = tail;
        tail = tail->prev;
        delete ptr;
    }

    delete tail;
}

/////// TaskIterator
template<typename T>
typename Pistol<T>::TaskIterator& Pistol<T>::TaskIterator::operator++() {
    _cur->is_iteratored = true;

    if (!is_end()) {
        while (_cur->prev == nullptr && !is_end()) {
            // 等待断链接上，但通常不会走到此分支
            std::this_thread::yield();
        }
        _cur = _cur->prev;
    }

    return *this;
}

template<typename T>
T& Pistol<T>::TaskIterator::operator*() const {
    if (is_end()) {
        // 走到这里说明使用前没有检查iter已经遍历完
        throw std::overflow_error("iterator reached end");
    }

    return dynamic_cast<TaskNode*>(_cur)->data;
}

template<typename T>
T* Pistol<T>::TaskIterator::operator->() const { 
    return &(operator*()); 
}

template<typename T>
bool Pistol<T>::TaskIterator::is_end() const {
    return _head->is_iteratored == true;
}