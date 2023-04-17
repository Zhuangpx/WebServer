
#include <unistd.h>
#include "z_webserver.h"

int main()
{
  /* 守护进程 后台运行 */

  /* 端口 ET模式 超时时间 linger模式  */
  int _port = 9999;
  int _mode = 3;
  int _time_out_MS = 60000;
  bool _opt_linger = true;
  /* MySQL */
  int _sql_port = 3306;
  const char* _sql_user = "root";
  const char* _sql_pwd = "2001";
  const char* _db_name = "webserver";
  /* 数据库连接池大小 线程池大小 日志启用 日志等级 日志异步队列大小 */
  int _conn_pool_num = 12;
  int _thread_num = 6;
  bool _open_log = true;
  int _log_level = 0;
  int _log_que_size = 1024;
  // zWebServer __server(9999, 3, 60000, false,   3306, "root", "123456", "webserver",   12, 6, true, 1, 1024);
  //  Server
  zWebServer __server(
                      _port, _mode, _time_out_MS, _opt_linger,
                      _sql_port, _sql_user, _sql_pwd, _db_name,
                      _conn_pool_num, _thread_num, _open_log, _log_level ,_log_que_size
                     );
  __server.server();
  return 0;
}
