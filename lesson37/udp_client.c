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

  // 服务器的地址信息
  struct sockaddr_in saddr;
  saddr.sin_family = AF_INET;
  saddr.sin_port = htons(9999);
  inet_pton(AF_INET, "127.0.0.1", &saddr.sin_addr.s_addr);

  // 2. 通信
  int num = 0;
  while (1) {
    char sendbuf[128];
    sprintf(sendbuf, "hello, I am client %d\n", num++);

    // 发送数据
    sendto(fd, sendbuf, strlen(sendbuf) + 1, 0, (struct sockaddr *)&saddr,
           sizeof(saddr));

    // 接收数据
    int num = recvfrom(fd, sendbuf, sizeof(sendbuf), 0, NULL, NULL);

    printf("server say : %s\n", sendbuf);
    sleep(1);
  }
  close(fd);
  return 0;
}
