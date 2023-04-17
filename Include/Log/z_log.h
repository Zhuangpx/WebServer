//  <z_log.h>
//  异步日志系统


#ifndef _Z_LOG_H
#define _Z_LOG_H

#include <cstring>
#include <mutex>
#include <thread>
#include <string>
#include <sys/time.h>
#include <sys/stat.h> //  mkdir
#include <stdarg.h>   //  va_start va_end ...

#include "../Utils/z_buffer.h"
#include "z_block_deque.h"


class zLog
{
public:

  //  初始化日志 设置相关属性
  void init(int _level, const char* _path = "./log",
            const char* _suffix = ".log",
            int _max_queue_capacity = 1024
            );

  static zLog *instance();        //  static  获取实例对象
  static void flush_log_thread(); //  static 线程刷新 异步写入

  //  接收日志级别和可变字符串参数 写入文件
  void write(int _level, const char *format, ...);
  void flush(); //  刷新(写入)日志文件

  int get_level();            //  获取日志级别
  void set_level(int _level); //  设置日志级别
  bool is_open()
  { return is_open_; }

private:

  zLog();
  virtual ~zLog();
  void append_log_level_title(int _level); //  根据日志级别添加标题
  void async_write();                      //  异步写入日志文件

  static const int LOG_PATH_LEN = 256;    //  日志文件路径最大长度
  static const int LOG_NAME_LEN = 256;    //  日志文件名最大长度
  static const int LOG_MAX_LINES = 50000; //  单个日志文件最大写入长度

  const char* path_;    //  日志文件路径
  const char* suffix_;  //  日志文件后缀

  int line_cnt_; //  当前日志文件已写入的行数 不应超过LOG_MAX_LINES
  int to_day_;   //  当前日期 当日期变更时，会自动创建一个新的日志文件，以保证每天的日志分别写入不同的文件。
  bool is_open_; // 标识日志文件是否已经打开。

  zBuffer buff_;  //  写缓冲区
  int level_;     //  日志级别
  bool is_async_; //  是否异步

  FILE *fp_;                                        //  日志文件
  std::unique_ptr<zBlockDeque<std::string>> deque_; //  日志异步队列 阻塞队列维护待写入的日志内容 生产者消费者模型
  std::unique_ptr<std::thread> write_thread_;       //  日志线程
  std::mutex mtx_;                                  //  同步锁
};


#define LOG_BASE(level, format, ...) \
    do {\
        zLog* log = zLog::instance();\
        if (log->is_open() && log->get_level() <= level) {\
            log->write(level, format, ##__VA_ARGS__); \
            log->flush();\
        }\
    } while(0);

#define LOG_DEBUG(format, ...) do {LOG_BASE(0, format, ##__VA_ARGS__)} while(0);
#define LOG_INFO(format, ...) do {LOG_BASE(1, format, ##__VA_ARGS__)} while(0);
#define LOG_WARN(format, ...) do {LOG_BASE(2, format, ##__VA_ARGS__)} while(0);
#define LOG_ERROR(format, ...) do {LOG_BASE(3, format, ##__VA_ARGS__)} while(0);




#endif  //  _Z_LOG_H

/**
 * Stills & Src:
*/
