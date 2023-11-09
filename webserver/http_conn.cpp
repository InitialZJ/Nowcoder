#include "http_conn.h"

// 设置非阻塞
void setnonblocking(int fd) {
  int old_flag = fcntl(fd, F_GETFL);
  int new_flag = old_flag |= O_NONBLOCK;
  fcntl(fd, F_SETFL, new_flag);
}

// 添加文件描述符到epoll中
void addfd(int epollfd, int fd, bool oneshot) {
  epoll_event event;
  event.data.fd = fd;
  event.events = EPOLLIN | EPOLLRDHUP;

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

// 关闭连接
void HttpConn::close_conn() {
  if (m_sockfd != -1) {
    removefd(m_epollfd, m_sockfd);
    m_sockfd = -1;
    m_user_count--;
  }
}
