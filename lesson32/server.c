// TCP通信的服务器端

#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
  // 1. 创建socket（用于监听的套接字）
  int lfd = socket(AF_INET, SOCK_STREAM, 0);
  if (lfd == -1) {
    perror("socket");
    exit(-1);
  }

  // 2. 绑定
  struct sockaddr_in saddr;
  saddr.sin_family = AF_INET;
  // inet_pton(AF_INET, "192.168.75.128", saddr.sin_addr.s_addr);
  saddr.sin_addr.s_addr = INADDR_ANY;
  saddr.sin_port = htons(9999);

  int ret = bind(lfd, (struct sockaddr *)&saddr, sizeof(saddr));
  if (ret == -1) {
    perror("bind");
    exit(-1);
  }

  // 3. 监听
  ret = listen(lfd, 8);
  if (ret == -1) {
    perror("listen");
    exit(-1);
  }

  // 4. 接收客户端的连接
  struct sockaddr_in clientaddr;
  socklen_t len = sizeof(clientaddr);
  int cfd = accept(lfd, (struct sockaddr *)&clientaddr, &len);
  if (cfd == -1) {
    perror("accept");
    exit(-1);
  }

  // 输出客户端的信息
  char clientIP[16];
  inet_ntop(AF_INET, &clientaddr.sin_addr.s_addr, clientIP, sizeof(clientIP));
  unsigned short clientPort = ntohs(clientaddr.sin_port);
  printf("client ip is %s, port is %d\n", clientIP, clientPort);

  // 5. 通信
  char recvBuf[1024] = {0};
  while (1) {
    // 获取客户端的数据
    len = read(cfd, recvBuf, sizeof(recvBuf));
    if (len == -1) {
      perror("read");
      exit(-1);
    } else if (len > 0) {
      printf("recv client data : %s\n", recvBuf);
    } else if (len == 0) {
      // 表示客户端断开连接
      printf("client closed...");
      break;
    }

    // 给客户端发送数据
    char *data = "hello, I am server";
    write(cfd, data, strlen(data));
  }

  // 关闭文件描述符
  close(cfd);
  close(lfd);
  return 0;
}
