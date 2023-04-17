//  <z_sql_conn_RAII.h>
//  RAII机制维护数据库连接和所属连接池，构造时获取连接，析构时归还

#ifndef _Z_SQL_CONN_RAII_H
#define _Z_SQL_CONN_RAII_H

#include "z_sql_conn_pool.h"

class zSqlConnRAII {
public:
  zSqlConnRAII(MYSQL** _sql, zSqlConnPool *_connpool) {
    assert(_connpool);
    *_sql = _connpool->get_conn_ptr();
    sql_ = *_sql;
    connpool_ = _connpool;
  }

  ~zSqlConnRAII() {
    if(sql_)
    { connpool_->free_conn(sql_); }
  }

private:
  MYSQL *sql_;
  zSqlConnPool* connpool_;
};

#endif

/**
 * Stills & Src:
*/
