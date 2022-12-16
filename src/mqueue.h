#pragma once

#include <deque>
#include <mutex>
#include <memory>
#include <vector>
#include <functional>

namespace garden {

template<typename DATA>
class MQueue {
public:
    class Closure {
    public:
        void run() {
            std::unique_ptr<Closure> guard(this);
            if (_run != nullptr) {
                _run();
            }
        }
        friend class MQueue;
    private:
        std::function<void()> _run;
    };

public:
    MQueue(int queue_num = 10) : _queue_num(queue_num), _queues(_queue_num) {}

    // 根据key选择入队队列。相同的key一定会入队同一个队列。
    void push_back(const std::string& key, DATA data) {
        int idx = _hash(key) % _queue_num;
        _queues[idx].push_back(data);
    }

    // 随机获取一个队首的data，多次get的data不会重复。
    bool get_front(DATA* data, Closure* done) {
        for (size_t i = 0; i < _queues.size(); ++i) {
            if (_queues[i].get_front(data)) {
                done->_run = std::bind(&MQueue<DATA>::pop_front, this, i, 1);
                return true;
            }
        }
        return false;
    }
    
    // 批量get
    bool get_front_batch(std::vector<DATA>* datas, int size, Closure* done) {
        for (size_t i = 0; i < _queues.size(); ++i) {
            if (_queues[i].get_front_batch(datas, &size)) {
                done->_run = std::bind(&MQueue<DATA>::pop_front, this, i, size);
                return true;
            }
        }
        return false;

    }

    // 删除key对应的data。只能删除队首的数据。
    bool pop_front(int idx, int size) {
        return _queues[idx].pop_front(size);
    }

    Closure* new_closure() {
        return new Closure();
    }

private:
    template<typename D>
    class SafeQueue {
    public:
        void push_back(const D& data) {
            std::lock_guard<std::mutex> guard(_mutex);
            _queue.push_back(data);
        }
        bool get_front(D* data) {
            std::lock_guard<std::mutex> guard(_mutex);
            if (_has_get || _queue.empty()) {
                return false;
            }
            *data = _queue.front();
            _has_get = true;
            return true;
        }
        bool pop_front(int size) {
            std::lock_guard<std::mutex> guard(_mutex);
            if (_queue.empty()) {
                return false;
            }
            _has_get = false;
            for (int i = 0; i < size; ++i) {
                _queue.pop_front();
            }
            return true;
        }
        bool get_front_batch(std::vector<D>* datas, int* size) {
            std::lock_guard<std::mutex> guard(_mutex);
            if (_has_get || _queue.empty()) {
                return false;
            }
            if (*size == -1 || *size > _queue.size()) {
                *size = _queue.size();
            }
            auto riter = _queue.rbegin();
            for (int i = 0; i < *size; ++i) {
                datas->push_back(*riter);
                riter++;   
            }
            _has_get = true;
            return true;
        }
    private:
        std::deque<D> _queue;
        std::mutex _mutex;
        bool _has_get {false};
    };

private:
    int _queue_num;
    std::vector<SafeQueue<DATA>> _queues;
    std::hash<std::string> _hash;
};

} // garden
