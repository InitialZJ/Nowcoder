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

#include <iostream>

class HttpConn {
 public:
  static int m_epollfd;  // 所有socket上的事件都被注册到同一个epoll对象中
  static int m_user_count;                    // 统计用户的数量
  static const int READ_BUFFER_SIZE = 2048;   // 读缓冲区的大小
  static const int WRITE_BUFFER_SIZE = 2048;  // 写缓冲区的大小

  enum METHOD { GET = 0, POST };

  enum CHECK_STATE {
    CHECK_STATE_REQUESTLINE = 0,
    CHECK_STATE_HEADER,
    CHECK_STATE_CONTENT
  };

  enum LINE_STATUS { LINE_OK = 0, LINE_BAD, LINE_OPEN };

  enum HTTP_CODE { NO_REQUEST = 0, GET_REQUEST, BAD_REQUEST, INTERNAL_ERROR };

  HttpConn(){};

  ~HttpConn(){};

  void init(int sockfd, const struct sockaddr_in& addr);  // 初始化新接收的连接

  void process();  // 处理客户端的请求

  void close_conn();  // 关闭连接

  bool read();  // 非阻塞的读

  bool write();  // 非阻塞的写

  HTTP_CODE process_read();                  // 解析Http请求
  HTTP_CODE parse_request_line(char* text);  // 解析请求首行
  HTTP_CODE parse_header(char* text);        // 解析请求头
  HTTP_CODE parse_content(char* text);       // 解析请求体

  LINE_STATUS parse_line();  // 解析行

  HTTP_CODE do_request();

  char* get_line() { return m_read_buf + m_start_line; }

 private:
  int m_sockfd;                       // 该HTTP连接的socket
  sockaddr_in m_address;              // 通信的socket地址
  char m_read_buf[READ_BUFFER_SIZE];  // 读缓冲区
  int m_read_idx;  // 标识读缓冲区中已经读入的客户端数据的最后一个字节的下一个位置

  int m_checked_idx;  // 当前正在分析的字符在读缓冲区的位置
  int m_start_line;   // 当前正在解析的行的起始位置
  char* m_url; // 请求目标文件的文件名
  char* m_version;  // 协议版本，只支持HTTP1.1
  METHOD m_method;  // 请求方法
  char* m_host;  // 主机名
  bool m_linger;  // HTTP请求是否要保持连接
  int m_contet_length;

  CHECK_STATE m_check_state;  // 主状态机当前所处的状态

  void init();  // 初始化连接以外的信息
};

#endif  // !HTTP_CONN_H_
