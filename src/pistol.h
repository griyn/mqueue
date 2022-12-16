#pragma once

#include <thread>
#include <string>
#include <atomic>
#include <memory>
#include <thread>
#include <iostream>
#include <functional>
#include <chrono>

namespace garden {

template<typename T>
class Pistol {
public:
    class TaskIterator;
    typedef std::function<void(TaskIterator&)> work_func_t;

public:
    Pistol(uint64_t capacity, work_func_t work_func) : 
        _capacity(capacity),
        _work_func(work_func),
        _head(new EndNode),
        _tail(_head) {};

    // 当前队列大小
    int64_t size();

    // 队列最大容量
    int64_t capacity();

    bool started();

    // 手动调用停止worker线程消费，清理node。threadunsafe
    // wait_until_ms: 最大等待队列消费时间，到时间后强制退出销毁节点。-1：等待直到消费完毕。
    void graceful_stop_and_join(int wait_until_ms);
    
    // 向队列中添加数据，wait-free
    bool post(T data);

public:
    // 队列中节点的定义
    class NodeBase {
    public:
        NodeBase(bool is) : is_end(is) {}
        virtual ~NodeBase() = default;
        bool is_end;  // 是否是end节点
        bool is_iteratored { false };  // 是否已遍历过
        NodeBase* next { nullptr };
        NodeBase* prev { nullptr };
    };

    class EndNode : public NodeBase {
    public:
        EndNode() : NodeBase(true) {};
    };

    class TaskNode : public NodeBase {
    public:
        TaskNode(T&& d) : data(std::move(d)), NodeBase(false) {
            s_size.fetch_add(1, std::memory_order_release);
        }
        ~TaskNode() {
            s_size.fetch_sub(1, std::memory_order_release);
        }
        T data;
        static inline std::atomic<uint64_t> s_size { 0 };
    };

    // 执行任务时用于遍历用户数据的游标，封装了一些关于内部双向链表的操作
    class TaskIterator {
    public:
        TaskIterator(NodeBase* head, NodeBase* tail) :
            _head(head), _tail(tail), 
            _cur(tail->prev) {} // iter创建时保证_cur不为空

        TaskIterator& operator++();
        T& operator*() const;
        T* operator->() const;

        // 是否已经遍历完成
        bool is_end() const;

    private:
        // 创建时的queue快照
        NodeBase* _head;
        NodeBase* _tail;
        // 当前游标，从后往前遍历
        NodeBase* _cur;
    };

private:
    void _work_loop();

    // 清空队列中的所有节点，顺序从尾到头
    void destory_queue_range(NodeBase* head, NodeBase* tail);

private:
    // 双向链表的头尾指针，初始化时会有一个end空节点，范围[_head, _tail)左闭右开
    // _head原子变量用于无锁地向队列顺序添加数据
    std::atomic<NodeBase*> _head;
    // _tail用于从队列顺序取数据，只有单消费线程会访问，可以使用普通类型
    NodeBase* _tail;

    // 工作线程，实例启动立即开始工作
    std::thread _worker {&Pistol::_work_loop, this};

    // 队列容量
    uint64_t _capacity;

    // 启动状态
    std::atomic<bool> _started {true};

    work_func_t _work_func;
};

#include "pistol.hpp"

} // namespace garden
