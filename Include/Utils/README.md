# Utils

## z_buffer

- 使用 `std::vector` 维护缓冲区，维护读写位置，下标，长度等，实现动态增长。
- 空间不足时扩容，底层用 `resize` 重新声明大小并拷贝移动。
- 文件读写使用 `readv+iovec` 的IO向量实现散布读（scatter read）和聚集写（gather write）。

## z_epoller

- 基于IO复用的epoll技术，采用事件驱动的方式监听IO。
- 封装zEpoller类，维护就绪的事件集合，为上层提供接口。

## z_timer

- 定时器封装zTimerNode，维护编号（取决于HTTP连接时的描述符），时间戳，回调函数，按时间顺序重载优先级。
- 定时器列表封装zTimer，用 `std::vector` 模拟的**小根堆**维护，以时间戳作为优先级。
- 使用 `chrono::high_resolution_clock` 维护时间戳，使用 `chrono::duration_cast` 实现时间戳转化
