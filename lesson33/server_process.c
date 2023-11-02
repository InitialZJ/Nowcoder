#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main() {
  // 创建socket
  int lfd = socket(AF_INET, SOCK_STREAM, 0);
  if (lfd == -1) {
    perror("socket");
    exit(-1);
  }

  struct sockaddr_in saddr;
  saddr.sin_family = AF_INET;
  saddr.sin_addr.s_addr = INADDR_ANY;
  saddr.sin_port = htons(9999);

  // 绑定
  int ret = bind(lfd, (struct sockaddr *)&saddr, sizeof(saddr));
  if (ret == -1) {
    perror("bind");
    exit(-1);
  }

  // 监听
  ret = listen(lfd, 128);
  if (ret == -1) {
    perror("listen");
    exit(-1);
  }

  // 不断循环等待客户端连接
  while (1) {
    struct sockaddr_in cliaddr;
    int len = sizeof(cliaddr);

    // 接受连接
    int cfd = accept(lfd, (struct sockaddr *)&cliaddr, &len);
    if (cfd == -1) {
      perror("accept");
      exit(-1);
    }

    // 每一个连接进来，创建一个子进程跟客户端通信
    pid_t pid = fork();
    if (pid == 0) {
      // 子进程
      // 获取客户端的信息
      char cliIp[16];
      inet_ntop(AF_INET, &cliaddr.sin_addr.s_addr, cliIp, sizeof(cliIp));
      unsigned short cliPort = ntohs(cliaddr.sin_addr.s_addr);

      printf("client IP is : %s, port is %d\n", cliIp, cliPort);

      // 接收客户端发来的数据
      char recvBuf[1024] = {0};
      while (1) {
        int len = read(cfd, &recvBuf, sizeof(recvBuf));

        if (len == -1) {
          perror("read");
          exit(-1);
        } else if (len > 0) {
          printf("recv client data : %s\n", recvBuf);
        } else {
          printf("client closed....");
        }

        write(cfd, recvBuf, strlen(recvBuf));
      }

      close(cfd);
      exit(0);
    }
  }

  close(lfd);

  return 0;
}