#ifndef HTTP_CONN_H_
#define HTTP_CONN_H_

#include <arpa/inet.h>
#include <error.h>
#include <fcntl.h>
#include <memory.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

class HttpConn {
 public:
  static int m_epollfd;  // 所有socket上的事件都被注册到同一个epoll对象中
  static int m_user_count;  // 统计用户的数量

  HttpConn(){};

  ~HttpConn(){};

  void init(int sockfd, const struct sockaddr_in& addr);  // 初始化新接收的连接

  void process();  // 处理客户端的请求

  void close_conn();  // 关闭连接
 private:
  int m_sockfd;           // 该HTTP连接的socket
  sockaddr_in m_address;  // 通信的socket地址
};

#endif  // !HTTP_CONN_H_
