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

#include "http_conn.h"
#include "locker.h"
#include "thread_poll.h"

#define MAX_FD 65535
#define MAX_EVNEN_NUMBER 10000  // 监听的最大的事件数量

// 添加信号捕捉
void addsig(int sig, void(handler)(int)) {
  struct sigaction sa;
  memset(&sa, '\0', sizeof(sa));
  sa.sa_handler = handler;
  sigfillset(&sa.sa_mask);
  sigaction(sig, &sa, NULL);
}

// 添加文件描述符到epoll中
extern void addfd(int epollfd, int fd, bool oneshot);

// 从epoll中删除文件描述符
extern void removefd(int epollfd, int fd);

// 修改文件描述符
extern void modifyfd(int epollfd, int fd, int ev);

int main(int argc, char* argv[]) {
  if (argc <= 1) {
    std::cout << "Format: " << basename(argv[0]) << " port_number" << std::endl;
    exit(-1);
  }

  // 获取端口号
  int port = atoi(argv[1]);

  // 对SIGPIE信号进行处理
  addsig(SIGPIPE, SIG_IGN);

  // 创建线程池，初始化线程池
  Threadpool<HttpConn>* pool = NULL;
  try {
    pool = new Threadpool<HttpConn>();
  } catch (...) {
    exit(-1);
  }

  // 创建一个数组用于保存所有的客户端信息
  HttpConn* users = new HttpConn[MAX_FD];

  int listenfd = socket(PF_INET, SOCK_STREAM, 0);

  // 设置端口复用
  int reuse = 1;
  setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

  // 绑定
  struct sockaddr_in address;
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(port);
  bind(listenfd, (struct sockaddr*)&address, sizeof(address));

  // 监听
  listen(listenfd, 5);

  // 创建epoll对象，事件数组，添加
  epoll_event events[MAX_EVNEN_NUMBER];
  int epollfd = epoll_create(5);

  // 将监听的文件描述符添加到epollfd中
  addfd(epollfd, listenfd, false);
  HttpConn::m_epollfd = epollfd;

  while (true) {
    int num = epoll_wait(epollfd, events, MAX_EVNEN_NUMBER, -1);
    if ((num < 0) && (errno != EINTR)) {
      std::cout << "epoll failure" << std::endl;
      break;
    }

    // 循环遍历事件数组
    for (int i = 0; i < num; i++) {
      int sockfd = events[i].data.fd;
      if (sockfd == listenfd) {
        // 有客户端连接进来
        struct sockaddr_in client_address;
        socklen_t client_addrlen = sizeof(client_address);
        int connfd = accept(listenfd, (struct sockaddr*)&client_address,
                            &client_addrlen);

        if (HttpConn::m_user_count >= MAX_FD) {
          // 目前连接数满了
          // 给客户端写一个信息：服务器内部正忙
          close(connfd);
          continue;
        }
        // 将新的客户的数据初始化，放到数组中
        users[connfd].init(connfd, client_address);
      } else if (events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
        // 对方异常断开或者错误等事件
        users[sockfd].close_conn();
      } else if (events[i].events & EPOLLIN) {
        if (users[sockfd].read()) {
          // 一次性把所有数据都读完
          pool->append(users + sockfd);
        } else {
          users[sockfd].close_conn();
        }
      } else if (events[i].events & EPOLLOUT) {
        if (!users[sockfd].write()) {
          // 一次性写完所有数据
          users[sockfd].close_conn();
        }
      }
    }
  }

  close(epollfd);
  close(listenfd);
  delete[] users;
  delete pool;

  return 0;
}
