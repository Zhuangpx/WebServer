#include "z_log.h"

/**
 * public:
*/



//  初始化日志 设置相关属性
void zLog::init(int _level, const char* _path, const char* _suffix, int _max_queue_capacity)
{

  is_open_ = true;
  level_ = _level;
  //  判断异步
  if(_max_queue_capacity > 0)
  {
    is_async_ = true;
    if(!deque_)
    {
      std::unique_ptr<zBlockDeque<std::string>> _n_deque(new zBlockDeque<std::string>);
      deque_ = std::move(_n_deque);
      std::unique_ptr<std::thread> _n_thread(new std::thread(flush_log_thread));
      write_thread_ = std::move(_n_thread);
    }
  }
  else is_async_ = false;
  //  日志格式
  path_ = _path;
  suffix_ = _suffix;
  line_cnt_ = 0;
  time_t _timer = time(nullptr);
  struct tm* _p_sys_time = localtime(&_timer);
  struct tm _sys_time = *_p_sys_time;
  char _file[LOG_NAME_LEN] = { 0 };
  //  path/2012_12_12[suf]
  snprintf(_file, LOG_NAME_LEN - 1, "%s/%04d_%02d_%02d%s", path_, _sys_time.tm_year+1900, _sys_time.tm_mon+1, _sys_time.tm_mday, suffix_);
  to_day_ = _sys_time.tm_mday;

  //  如果日志文件已打开 先清空并关闭
  std::lock_guard<std::mutex> locker(mtx_);
  buff_.clear_buffer();
  if (fp_)
  {
    flush();
    fclose(fp_);
  }
  //  打开新的日志文件 如果打开失败则创建
  fp_ = fopen(_file, "a");
  if (fp_ == nullptr)
  {
    mkdir(path_, 0777); //  mode = 111 111 111
    fp_ = fopen(_file, "a");
  }
  assert(fp_ != nullptr);
}


//  接收日志级别和可变字符串参数 写入文件
//  可变参数format
void zLog::write(int _level, const char* format, ...)
{
  //  当前时间
  struct timeval _now_time = {0, 0};
  gettimeofday(&_now_time, nullptr);
  time_t _now_sec = _now_time.tv_sec;
  struct tm* _p_sys_time = localtime(&_now_sec);
  struct tm _sys_time = *_p_sys_time;
  va_list _va_list;
  //  如果不是同一天 或者当前日志文件写入行数超过最大行数 则要新建日志文件
  if(to_day_ != _sys_time.tm_mday || (line_cnt_ && (line_cnt_ % LOG_MAX_LINES == 0)))
  {
    std::unique_lock<std::mutex> _locker(mtx_);
    _locker.unlock();
    char _new_file[LOG_NAME_LEN];
    char _tail[36] = { 0 };
    //  2012_12_13
    snprintf(_tail, 36, "%04d_%02d_%02d", _sys_time.tm_year + 1900, _sys_time.tm_mon + 1, _sys_time.tm_mday);
    //  不是同一天
    if(to_day_ != _sys_time.tm_mday)
    {
      //  new_path/2012_12_13[suf]
      snprintf(_new_file, LOG_NAME_LEN - 72, "%s/%s%s", path_, _tail, suffix_);
      to_day_ = _sys_time.tm_mday;
      line_cnt_ = 0;
    }
    //  同一天
    else
    {
      //  path/2012_12_13-CNT[suf]
      snprintf(_new_file, LOG_NAME_LEN - 72, "%s/%s-%d%s", path_, _tail, (line_cnt_ / LOG_MAX_LINES), suffix_);
    }
    _locker.lock();
    //  把旧的写入文件 然后fp_打开为新的文件
    flush();
    fclose(fp_);
    fp_ = fopen(_new_file, "a");
    assert(fp_ != nullptr);
  }
  //  正常写入
  std::unique_lock<std::mutex> _locker(mtx_);
  line_cnt_++;
  //  2012-12-12 11:20:30.400000
  int _n = snprintf(buff_.write_ptr(), 128, "%d-%02d-%02d %02d:%02d:%02d.%06ld ",
                    _sys_time.tm_year + 1900, _sys_time.tm_mon + 1, _sys_time.tm_mday,
                    _sys_time.tm_hour, _sys_time.tm_min, _sys_time.tm_sec, _now_time.tv_usec);
  buff_.update_write_ptr(_n);

  va_start(_va_list, format);
  int _m = vsnprintf(buff_.write_ptr(), buff_.writable_size(), format, _va_list);
  buff_.update_write_ptr(_m);
  buff_.push("\n\0", 2);
  // 检查是否启用了异步写入模式
  // 如果启用了异步模式并且队列不满，则将缓冲区中的内容推送到异步队列中
  // 否则直接将其写入到文件中
  if(is_async_ && deque_ && !(deque_->full()))
  {
    deque_->push_back(buff_.get_and_clear());
  }
  else
  {
    fputs(buff_.read_ptr(), fp_);
  }
  buff_.clear_buffer();
}

//  static  获取实例对象
zLog* zLog::instance()
{
  static zLog _log;
  return &_log;
}

//  static 线程刷新 异步写入
void zLog::flush_log_thread()
{
  zLog::instance() -> async_write();
}

//  刷新日志文件
void zLog::flush()
{
  if(is_async_)
  { deque_ -> flush(); }
  fflush(fp_);
}

//  获取日志级别
int zLog::get_level()
{
  std::lock_guard<std::mutex> _locker(mtx_);
  return level_;
}

//  设置日志级别
void zLog::set_level(int _level)
{
  std::lock_guard<std::mutex> _locker(mtx_);
  level_ = _level;
}



/**
 * private:
*/

//  构造函数
zLog::zLog()
{
  line_cnt_ = 0;
  is_async_ = false;
  write_thread_ = nullptr;
  deque_ = nullptr;
  to_day_ = 0;
  fp_ = nullptr;
}

//  析构函数
zLog::~zLog()
{
  //  确保所有日志都已写入文件并释放了相应的资源，防止内存泄漏或日志文件不完整
  if(write_thread_ && write_thread_->joinable())
  {
    while(!(deque_->empty())) //  先刷新 缓冲区里 未写入的部分
    { deque_->flush(); }
    deque_->close();
    write_thread_->join();
  }
  if(fp_)
  {
    //  如果fp_打开 需要刷新并关闭
    std::lock_guard< std::mutex > _locker(mtx_);
    flush();
    fclose(fp_);
  }
}


//  根据日志级别添加标题
void zLog::append_log_level_title(int _level)
{
  switch(_level)
  {
    case 0:
      buff_.push("[debug]: ", 9);
      break;
    case 1:
      buff_.push("[info] : ", 9);
      break;
    case 2:
      buff_.push("[warn] : ", 9);
      break;
    case 3:
      buff_.push("[error]: ", 9);
      break;
    default:
      buff_.push("[info] : ", 9);
      break;
  }
}

//  异步写入日志文件
void zLog::async_write()
{
  std::string _str = "";
  while(deque_->pop(_str))
  {
    std::lock_guard<std::mutex> _locker(mtx_);
    fputs(_str.c_str(), fp_);
  }
}
