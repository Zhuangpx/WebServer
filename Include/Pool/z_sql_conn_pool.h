//  <z_sql_conn_pool.h>
//  数据库连接池

#ifndef _Z_SQL_CONN_POLL_H
#define _Z_SQL_CONN_POLL_H

#include <queue>         //  queue
#include <mutex>         //  mutex
#include <mysql/mysql.h> //  mysql
#include <semaphore.h>   //  sem
#include <thread>
#include <assert.h>      // assert
#include "../Log/z_log.h"

class zSqlConnPool
{

public:

  static zSqlConnPool* instance();  //  static function : create static pool pointer
  //  init the sql pool
  void init(const char* _host, int _port,
            const char* _user, const char* _pwd,
            const char* _dbname, int _conn_size);
  MYSQL *get_conn_ptr();       //  get connect sql from pool
  void free_conn(MYSQL *_sql); //  free the connected _sql
  void close_pool();           //  close the sql pool
  size_t get_free_conn_cnt();  //  get free connect count

private:
  zSqlConnPool();
  ~zSqlConnPool();

  size_t max_conn_cnt_;  //  max connect count (最大连接数)
  size_t use_conn_cnt_;  //  be using connect count (使用连接数)
  size_t free_conn_cnt_; //  free connect count (可用连接数)

  std::queue<MYSQL*> conn_que_;
  std::mutex mtx_;
  sem_t sem_;

};


#endif // _Z_THREADPOOL_SQL

/**
 * Stills & Src:
 * mutex & sem
 * mysql
*/
