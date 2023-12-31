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

  // 2. 客户端绑定本地的IP和端口
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(9999);
  addr.sin_addr.s_addr = INADDR_ANY;

  int ret = bind(fd, (struct sockaddr *)&addr, sizeof(addr));
  if (ret == -1) {
    perror("bind");
    exit(-1);
  }

  // 2. 通信
  int num = 0;
  while (1) {
    char buf[128];

    // 接收数据
    int num = recvfrom(fd, buf, sizeof(buf), 0, NULL, NULL);

    printf("server say : %s\n", buf);
  }
  close(fd);
  return 0;
}
