#include "http_conn.h"

int HttpConn::m_epollfd = -1;
int HttpConn::m_user_count = 0;

// 设置非阻塞
void setnonblocking(int fd) {
  int old_flag = fcntl(fd, F_GETFL);
  int new_flag = old_flag | O_NONBLOCK;
  fcntl(fd, F_SETFL, new_flag);
}

// 添加文件描述符到epoll中
void addfd(int epollfd, int fd, bool oneshot) {
  epoll_event event;
  event.data.fd = fd;
  // event.events = EPOLLIN | EPOLLRDHUP;  // 默认水平触发
  event.events = EPOLLIN | EPOLLRDHUP | EPOLLET;  // 边缘触发

  if (oneshot) {
    event.events |= EPOLLONESHOT;
  }
  epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);

  // 设置文件描述符非阻塞
  setnonblocking(fd);
}

// 从epoll中删除文件描述符
void removefd(int epollfd, int fd) {
  epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, 0);
  close(fd);
}

// 修改文件描述符
void modifyfd(int epollfd, int fd, int ev) {
  epoll_event event;
  event.data.fd = fd;
  event.events = ev | EPOLLONESHOT | EPOLLRDHUP;
  epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event);
}

// 初始化新接收的连接
void HttpConn::init(int sockfd, const struct sockaddr_in& addr) {
  m_sockfd = sockfd;
  m_address = addr;

  // 端口复用
  int reuse = 1;
  setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

  // 添加到epoll对象中
  addfd(m_epollfd, sockfd, true);
  m_user_count++;
}

// 初始化连接以外的信息
void HttpConn::init() {
  m_check_state = CHECK_STATE_REQUESTLINE;  // 初始化状态为解析请求首行
  m_checked_idx = 0;
  m_start_line = 0;
  m_read_idx = 0;

  m_method = GET;
  m_url = 0;
  m_version = 0;
  m_linger = false;
  bzero(m_read_buf, READ_BUFFER_SIZE);
}

// 关闭连接
void HttpConn::close_conn() {
  if (m_sockfd != -1) {
    removefd(m_epollfd, m_sockfd);
    m_sockfd = -1;
    m_user_count--;
  }
}

// 非阻塞的读
// 循环读取数据，直到无数据可读或者对方关闭连接
bool HttpConn::read() {
  if (m_read_idx >= READ_BUFFER_SIZE) {
    return false;
  }

  // 已经读取到的字节
  int bytes_read = 0;
  while (true) {
    bytes_read = recv(m_sockfd, m_read_buf + m_read_idx,
                      READ_BUFFER_SIZE - m_read_idx, 0);
    if (bytes_read == -1) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        // 没有数据
        break;
      }
      return false;
    } else if (bytes_read == 0) {
      // 对方关闭连接
      return false;
    }
    m_read_idx += bytes_read;
  }
  std::cout << "读取到了数据：" << m_read_buf << std::endl;
  return true;
}

// 非阻塞的写
bool HttpConn::write() {
  std::cout << "一次性写完数据" << std::endl;
  return true;
}

// 处理客户端的请求
void HttpConn::process() {
  // 解析Http请求
  HTTP_CODE read_ret = process_read();
  if (read_ret == NO_REQUEST) {
    modifyfd(m_epollfd, m_sockfd, EPOLLIN);
    return;
  }

  std::cout << "Parse request, create response" << std::endl;

  // 生成响应
}

// 解析Http请求
HttpConn::HTTP_CODE HttpConn::process_read() {
  LINE_STATUS line_status = LINE_OK;
  HTTP_CODE ret = NO_REQUEST;

  char* text = 0;
  while (m_check_state == CHECK_STATE_CONTENT && line_status == LINE_OK ||
         ((line_status = parse_line()) == LINE_OK)) {
    // 解析到了请求体，或者解析到了一行完整的数据

    // 获取一行数据
    text = get_line();

    m_start_line = m_checked_idx;
    std::cout << "Got 1 http line: " << text << std::endl;

    switch (m_check_state) {
      case CHECK_STATE_REQUESTLINE: {
        ret = parse_request_line(text);
        if (ret == BAD_REQUEST) {
          return BAD_REQUEST;
        }
        break;
      }

      case CHECK_STATE_HEADER: {
        ret = parse_header(text);
        if (ret == BAD_REQUEST) {
          return BAD_REQUEST;
        } else if (ret == GET_REQUEST) {
          return do_request();
        }
        break;
      }

      case CHECK_STATE_CONTENT: {
        ret = parse_content(text);
        if (ret == GET_REQUEST) {
          return do_request();
        }
        line_status = LINE_OPEN;
        break;
      }

      default: {
        return INTERNAL_ERROR;
      }
    }
  }
  return NO_REQUEST;
}

// 解析请求首行
HttpConn::HTTP_CODE HttpConn::parse_request_line(char* text) {
  // GET / HTTP/1.1
  m_url = strpbrk(text, " \t");
  *m_url++ = '\0';

  char* method = text;
  if (strcasecmp(method, "GET") == 0) {
    m_method = GET;
  } else {
    return BAD_REQUEST;
  }
  m_version = strpbrk(m_url, " \t");
  if (!m_version) {
    return BAD_REQUEST;
  }
  *m_version++ = '\0';
  if (strcasecmp(m_version, "HTTP/1.1") != 0) {
    return BAD_REQUEST;
  }

  if (strncasecmp(m_url, "http://", 7) == 0) {
    m_url += 7;
    m_url = strchr(m_url, '/');
  }

  if (!m_url || m_url[0] != '/') {
    return BAD_REQUEST;
  }

  m_check_state = CHECK_STATE_HEADER;
  return NO_REQUEST;
}

// 解析请求头
HttpConn::HTTP_CODE HttpConn::parse_header(char* text) {
  if (text[0] == '\0') {
    if (m_contet_length != 0) {

    }
  }
}

// 解析请求体
HttpConn::HTTP_CODE HttpConn::parse_content(char* text) {}

// 解析行
HttpConn::LINE_STATUS HttpConn::parse_line() {
  char temp;

  for (; m_checked_idx < m_read_idx; ++m_checked_idx) {
    temp = m_read_buf[m_checked_idx];
    if (temp == '\r') {
      if ((m_checked_idx + 1) == m_read_idx) {
        return LINE_OPEN;
      } else if (m_read_buf[m_checked_idx + 1] == '\n') {
        m_read_buf[m_checked_idx++] = '\0';
        m_read_buf[m_checked_idx++] = '\0';
        return LINE_OK;
      }
      return LINE_BAD;
    } else if (temp == '\n') {
      if ((m_checked_idx > 1) && (m_read_buf[m_checked_idx - 1] == '\r')) {
        m_read_buf[m_checked_idx - 1] = '\0';
        m_read_buf[m_checked_idx++] = '\0';
        return LINE_OK;
      }
      return LINE_BAD;
    }
    return LINE_OPEN;
  }
  return LINE_OK;
}

HttpConn::HTTP_CODE HttpConn::do_request() {}
