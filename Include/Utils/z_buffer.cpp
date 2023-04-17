#include "z_buffer.h"

/**
 * public
*/

/* Info */

//  std::string形式获取缓冲区内容 并清空缓冲区
std::string zBuffer::get_and_clear()
{
  std::string _str(read_ptr(), readable_size());
  clear_buffer();
  return _str;
}

//  构造函数 初始化缓冲区buffer_和读写位置
zBuffer::zBuffer(size_t _buffer_size/*=1024*/) : read_pos_(0), write_pos_(0), buffer_(_buffer_size) {}

//  可读大小（字节）
size_t zBuffer::readable_size() const
{
  return write_pos_ - read_pos_;
}

//  可写大小（字节）
size_t zBuffer::writable_size() const
{
  return buffer_.size() - write_pos_;
}

//  已经读过的大小（字节）
size_t zBuffer::read_size() const
{
  return read_pos_;
}

//  当前读位置的指针
const char* zBuffer::read_ptr() const
{
  return buffer_begin() + read_pos_;
}

//  当前写位置的指针 const
const char* zBuffer::write_ptr_const() const
{
  return buffer_begin() + write_pos_;
}

//  当前写位置的指针 无const
char* zBuffer::write_ptr()
{
  return buffer_begin() + write_pos_;
}

/* Modify */

//  将缓冲区清空
void zBuffer::clear_buffer()
{
  // fill(buffer_.begin(), buffer_.end(), '\0');
  bzero(&buffer_[0], buffer_.size());
  read_pos_ = write_pos_ = 0;
}

//  更新读指针 后移_size
void zBuffer::update_read_ptr(size_t _size)
{
  assert(_size <= readable_size()); //  保证可读大小足够
  read_pos_ += _size;
}

//  更新读指针 定位到_ptr
void zBuffer::set_read_ptr(const char* _ptr)
{
  assert(_ptr >= read_ptr());       //  保证后移
  update_read_ptr(_ptr - read_ptr());
}

//  更新写指针 后移 _size
void zBuffer::update_write_ptr(size_t _size)
{
  assert(_size <= writable_size()); //  保证可写大小足够
  write_pos_ += _size;
}

/* IO */

//  写入数据
void zBuffer::push(const char* _str, size_t _size)
{
  assert(_str);
  ensure_writable(_size);
  std::copy(_str, _str+_size, write_ptr());
  update_write_ptr(_size);
}
void zBuffer::push(const std::string& _str)
{
  push(_str.data(), _str.length());
}
void zBuffer::push(const void* _data_p, size_t _size)
{
  assert(_data_p);
  push(static_cast<const char*>(_data_p), _size);
}
void zBuffer::push(const zBuffer& _buffer)
{
  push(_buffer.read_ptr(), _buffer.readable_size());
}

//  api for IO

//  从文件(_fd)中读
ssize_t zBuffer::read_fd(int _fd, error_t* _error)
{
  char _buff[65535];
  struct iovec iov[2];
  const size_t _writable = writable_size();
  /* 利用iovec(io向量)  分散读 集中写*/
  iov[0].iov_base = buffer_begin() + write_pos_;
  iov[0].iov_len = _writable;
  iov[1].iov_base = _buff;
  iov[1].iov_len = sizeof(_buff);

  //  read form _fd and put in iov
  const ssize_t _size = readv(_fd, iov/*iovec*/, 2/*iovec cnt*/);
  if(_size < 0) {   //  failed
    *_error = errno;  //  get errno (error code)
  }
  else if(_writable >= static_cast<size_t>(_size)) {  //  success
    write_pos_ += _size;
  }
  else {            //  not enough
    write_pos_ = buffer_.size();
    push(_buff, _size - _writable);
  }
  return _size;
}

//  往文件(_fd)中写
ssize_t zBuffer::write_fd(int _fd, error_t* _error)
{
  size_t _readable_size = readable_size();
  ssize_t _size = write(_fd, read_ptr(), _readable_size);
  if(_size < 0) {
    *_error = errno;
    return _size;
  }
  read_pos_ += _size;
  return _size;
}

//  保证可读 (读大小足够)
void zBuffer::ensure_writable(size_t _size)
{
  if(_size > writable_size()) {
    resize(_size);
  }
  assert(_size <= writable_size());
}

/**
 * private
*/

//  开始位置的指针
char* zBuffer::buffer_begin()
{
  return &*buffer_.begin();
}
const char* zBuffer::buffer_begin() const
{
  return &*buffer_.begin();
}

//  重新分配内存大小 -> 扩容
void zBuffer::resize(size_t _new_size)
{
  //  buffer size = read size + write size
  if(_new_size > read_size() + writable_size())
  {
    buffer_.resize(write_pos_+_new_size+1);
  }
  else
  {
    size_t _readable_size = readable_size();
    std::copy(buffer_begin()+read_pos_, buffer_begin()+write_pos_, buffer_begin());
    read_pos_ = 0;
    write_pos_ = read_pos_ + _readable_size;
    assert(_readable_size == readable_size());
  }
}
