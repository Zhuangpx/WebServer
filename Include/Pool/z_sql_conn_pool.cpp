#include "z_sql_conn_pool.h"


/**
 * public:
*/

//  静态成员函数 创建静态线程池指针
zSqlConnPool* zSqlConnPool::instance()
{
  static zSqlConnPool _conn_pool;
  return &_conn_pool;
}

//  初始化 MYSQL连接池
void zSqlConnPool::init(const char* _host, int _port,
                          const char* _user, const char* _pwd,
                          const char* _dbname, int _conn_size = 10)
{
  assert(_conn_size > 0);
  for(int _i=0; _i<_conn_size; _i++)
  {
    MYSQL *_sql = nullptr;
    _sql = mysql_init(_sql);
    if(!_sql)   //  error return nullptr
    {
      LOG_ERROR("Mysql init error!");
      assert(_sql);
    }
    _sql = mysql_real_connect(_sql, _host, _user, _pwd, _dbname, _port, nullptr, 0);
    if(!_sql)
    { LOG_ERROR("Mysql Connect error!"); }
    conn_que_.push(_sql);
  }
  max_conn_cnt_ = _conn_size;
  sem_init(&sem_, 0, max_conn_cnt_);  //  sem_ 设为 最大连接数
}

//  从连接池里取出一个可用sql连接
MYSQL* zSqlConnPool::get_conn_ptr()
{
  MYSQL *_sql = nullptr;
  if(conn_que_.empty()) //  可用连接池为空
  {
    LOG_WARN("zSqlConnPool is busy...");
    return nullptr;
  }
  sem_wait(&sem_);
  std::lock_guard<std::mutex> _locker(mtx_);
  _sql = conn_que_.front();
  conn_que_.pop();
  return _sql;
}

//  获取连接池中的可用连接数
std::size_t zSqlConnPool::get_free_conn_cnt()
{
  std::lock_guard<std::mutex> _locker(mtx_);
  return conn_que_.size();
}

//  释放连接
void zSqlConnPool::free_conn(MYSQL *_sql)
{
  assert(_sql);
  std::lock_guard<std::mutex> _locker(mtx_);
  conn_que_.push(_sql);
  sem_post(&sem_);
}

//  关闭连接池
void zSqlConnPool::close_pool()
{
  std::lock_guard<std::mutex> _locker(mtx_);
  while(!conn_que_.empty())
  {
    auto _now_conn_sql = conn_que_.front();
    conn_que_.pop();
    mysql_close(_now_conn_sql);
  }
  mysql_library_end();
}

/**
 * private:
*/

//  构造函数 初始化计数变量
zSqlConnPool::zSqlConnPool()
{
  max_conn_cnt_ = 0;
  use_conn_cnt_ = 0;
  free_conn_cnt_ = 0;
}

//  析构函数 关闭线程池
zSqlConnPool::~zSqlConnPool()
{
  close_pool();
}
