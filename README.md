# 并行队列

## Pistol
MPSC(Multi-Producer Single-Consumer)执行队列，所有操作无锁且wait-free。

### feature
* 可以多个线程并发地提交数据
* 异步有序执行已提交的数据，严格按提交的顺序处理数据；通过批量读提高吞吐，用户也可以在业务层做数据merge
* 业务层不用考虑资源竞争，可以解决一些业务场景下的时序竞态
* 支持优雅退出
* 所有操作无锁且wait-free，对队列的操作不会导致阻塞

### example
```c++
// 初始化
::garden::Pistol<std::string> pistol(500, std::bind(batch_read, std::placeholders::_1));

// 读
void batch_read(::garden::Pistol<std::string>::TaskIterator& iter) {
    for (; !iter.is_end(); ++iter) {
        std::cout << *iter << " ";
    }
    std::cout << std::endl;
}

// 写
std::string str = "Hello";
pistol.post(std::move(str));
pistol.post("World");
```

### detail
doc: [pistol.md](https://github.com/griyn/mqueue/blob/main/docs/pistol.md)

## Revolver
MPMC(Multi-Producer Multi-Consumer)数据队列，所有操作无锁且wait-free，基于ringbuffer。

### feature
* 支持多线程并发地读写队列数据
* 所有操作无锁且wait-free，对队列的操作不会导致阻塞
* 多线程下的读写不保证顺序

### example
```
::garden::Revolver<std::string> revolver(500);

// 读
std::string data;
revolver.pop(&data);

// 写
std::string ws = "Hello";
revolver.push(std::move(ws));
```

### detail
doc: [revolver.md](https://github.com/griyn/mqueue/blob/main/docs/revolver.md)

## MQueue
并行队列，相同key按顺序处理 & 并发性能。

mqueue实例中包含多个micro队列，依据入队时的key提交到各个队列里，相同的key会存到同一个队里中按顺序处理。

TODO: 解决读数据的锁竞争

### featrue
* 多个队列锁粒度降低
* 相同key按顺序取数据
* 只支持多写一读，可以通过批量读提高并发

### example
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
