#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main() {
  // 1. 创建一个通信的socket
  int fd = socket(AF_INET, SOCK_DGRAM, 0);

  if (fd == -1) {
    perror("socket");
    exit(-1);
  }

  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(9999);
  addr.sin_addr.s_addr = INADDR_ANY;

  // 2. 绑定
  int ret = bind(fd, (struct sockaddr *)&addr, sizeof(addr));
  if (ret == -1) {
    perror("bind");
    exit(-1);
  }

  // 3. 通信
  while (1) {
    char recvbuf[128];
    char IP[16];
    struct sockaddr_in cliaddr;
    int len = sizeof(cliaddr);

    // 接收数据
    int num = recvfrom(fd, recvbuf, sizeof(recvbuf), 0,
                       (struct sockaddr *)&cliaddr, &len);

    printf("client IP : %s, port : %d\n",
           inet_ntop(AF_INET, &cliaddr.sin_addr.s_addr, IP, sizeof(IP)),
           ntohs(cliaddr.sin_port));

    printf("client say : %s\n", recvbuf);

    // 发送数据
    sendto(fd, recvbuf, strlen(recvbuf) + 1, 0, (struct sockaddr *)&cliaddr,
           sizeof(cliaddr));
  }
  close(fd);
  return 0;
}
