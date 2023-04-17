# WebServer

基于 C++14 实现的高性能 Web 服务器，经过 webbenchh 压力测试可以实现上万的 QPS

## 环境

- Linux(Ubuntu)
- MySQL
- C++11

## 技术点

- OOP编程，采用对象抽象封装
- 利用IO复用技术Epoll与线程池实现多线程的Reactor高并发模型，封装Epoller，向上提供文件监听相关接口
- 基于`std::vector<char>`封装buffer缓冲区，实现自动增长，读写时采用`readv + iov`实现分散读，集中写
- 基于`std::vector<T>`模拟小根堆，构造时间戳结点实现的timer定时器，超时调用回调函数并关闭超时的非活动连接
- 采用单例模式懒汉模式构造数据库连接池，利用RAII机制维护连接与断开，减少数据库连接建立与关闭的开销，同时实现了用户的注册与登录功能
- 预处理线程池，`std::queue<T>`维护任务队列，智能指针维护线程池指针，子线程利用Lambda表达式保存任务后线程分离
- 利用状态机(主从)和正则(`std::regex`)解析HTTP请求报文，构造响应报文，实现处理静态资源的请求，将请求的资源利用文件与共享内存的映射提高访问速度
- 利用单例模式与阻塞队列实现异步的日志系统，记录服务器运行状态，借助全局宏来打标记
- C++新特性：
  - `cast<static>`实现数据类型强转
  - `const_cast<T>`去除复合类型中const和volatile属性
  - `atomic<T>`实现读写的原子操作
  - 使用 `lock_guard<std::mutex>` 和 `lock_unique<std::mutex>` 实现数据共享区的同步访问。
  - 使用智能指针 `shared_ptr<T>` 和 `unique_ptr<T>` 维护线程池等动态资源。
  - `bind`函数适配器转换为适配线程池参数的函数类型。

## 结构

```shell
.
├── Bin
│   └── server
├── Build
│   └── Makefile
├── copy.cpp
├── Debug
├── Doc
│   ├── cpp_API.md
├── Include
│   ├── Http
│   │   ├── z_http_conn
│   │   ├── z_http_request
│   │   ├── z_http_response
│   ├── Log
│   │   ├── z_block_deque.h
│   │   ├── z_log.cpp
│   │   └── z_log.h
│   ├── Pool
│   │   ├── z_sql_conn_pool
│   │   ├── z_sql_conn_RAII
│   │   └── z_threadpool
│   ├── Utils
│   │   ├── README.md
│   │   ├── z_buffer
│   │   ├── z_epoller
│   │   ├── z_timer
│   ├── z_cond
│   ├── z_locker
│   └── z_sem
├── Makefile
├── mysql-community-release-el7-5.noarch.rpm
├── README.md
├── Resource
├── Source
│   ├── main.cpp
│   ├── z_webserver
└── Test
    └── webbench-1.5
```

## 启动

先准备数据库。

```shell
// 新建库
create database [DB_name];

// 新建用户表(user)
USE [DB_name];
CREATE TABLE user(
    username char(50) NULL,
    password char(50) NULL
)ENGINE=InnoDB;

// 例子 添加用户
INSERT INTO user(username, password) VALUES('name', 'password');
```

之后构建，运行。

```shell
make
./Bin/server
```

在`log`目录下会有日志文件。

## 压力测试

利用webbench工具来测压。
webbench工具会先通过父线程fork出若干子进程，由子进程循环web访问，然后把结果通过pipe返回给父进程，由父进程统计。

```shell
./Test/webbench-1.5/webbench -c [并发数] -t [时间] http://ip:port/
```

Queries Per Second：QPS 10000+

## 致谢

感谢开源互联网。

@qinguoyi，@markparticle，@agedcat

[牛客](https://www.nowcoder.com/courses/cover/live/504)，[Linux高性能服务器编程 游双著](https://book.douban.com/subject/24722611/)

[C++风格指南](https://zh-google-styleguide.readthedocs.io/en/latest/google-cpp-styleguide/contents/)
