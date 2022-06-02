# MQueue
并行队列，相同key按顺序处理 & 保证并发性能。

mqueue实例中包含多个队列，依据入队时的key提交到各个队列里，相同的key会存到同一个队里中按顺序处理。
## Featrue
* 多个队列锁粒度降低
* 相同key按顺序取数据
* 只支持多写一读，可以通过批量读提高并发

## example
```c++
// 写
::garden::MQueue<std::string> mqueue;
mqueue.push_back("f1", "HELLO"); // push_back(key, data)
mqueue.push_back("f1", "WORLD");

// 读
auto* done = mqueue.new_closure();
if (mqueue.get_front(&read, done)) { // get_front(&data, clourse)
    std::cout << read << std::endl;
    done->run(); // clourse用于删除读到的数据，释放读锁
}

// 批量读
auto* done = mqueue.new_closure();
std::vector<std::string> reads;
if (mqueue.get_front_batch(&reads, -1, done)) { // get_front_batch(&data, batch_size, clourse)
    for (auto& read : reads) {
        std::cout << read << std::endl;
    }
    done->run();
}
```
