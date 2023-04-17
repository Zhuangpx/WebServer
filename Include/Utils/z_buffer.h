//  <z_buffer.h>
//  缓存区

#ifndef _Z_BUFFER_H
#define _Z_BUFFER_H

#include <iostream>  //  IO
#include <vector>    //  vector
#include <atomic>    //  atomic
#include <string>    //  string
#include <assert.h>  //  assert
#include <algorithm> //  fill
#include <sys/uio.h> //  iovec
#include <unistd.h>  //  read&&wtrite
#include <cstring>   //  bzero

class zBuffer
{
public:
  zBuffer(size_t buffer_size = 1024);
  ~zBuffer() = default;

//  Info

  std::string get_and_clear();   //  get [String] of buffer and clear

  size_t writable_size() const;  //  writable size
  size_t readable_size() const;  //  readable size
  size_t read_size() const;      //  size has read

  const char* read_ptr() const;         //  read pointer
  const char* write_ptr_const() const;  //  write pointer const
  char* write_ptr();                    //  write pointer non-const

//  Modify
  void clear_buffer();                  //  clear buffer and reset write&read pointer
  void update_read_ptr(size_t _size);   //  update read pointer after _size
  void set_read_ptr(const char* _ptr);  //  set read pointer to _ptr
  void update_write_ptr(size_t _size);  //  update write pointer after write

// IO

  //  push data into buffer_
  void push(const char* _str, size_t _size);
  void push(const std::string& _str);
  void push(const void* _data_p, size_t _size);
  void push(const zBuffer& _buffer);

  //  client and server for IO
  //  api for IO
  ssize_t read_fd(int _fd, error_t* _error);  //  read from _fd
  ssize_t write_fd(int _fd, error_t* _error); //  write into _fd

  void ensure_writable(size_t _size); //  ensure writable

  void debug()
  {
    std::cout << "buffer_:" << buffer_.size() << std::endl;
    for(auto i:buffer_) std::cout << i ;
    std::cout << std::endl;
  }

private:
  char* buffer_begin();             //  begin pointer
  const char* buffer_begin() const; //  begin pointer const
  void resize(size_t _new_size);    //  resize -> reallocate memory size

  std::vector<char> buffer_;            //  read&write buffer
  std::atomic<std::size_t> read_pos_;   //  read position  !atomic
  std::atomic<std::size_t> write_pos_;  //  write position !atomic

};


#endif

/**
 * Stills & Src:
 * default : c++11关键字
 * static_cast : 强制类型转换
 * iovec : 向量IO
*/
