#include <arpa/inet.h>
#include <asm-generic/socket.h>
#include <ctype.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
void not_found(int client);
void unimplemented(int sock);
int startup(int port);
void serve_file(int client, const char *filename);
int get_line(int sock, char *buf, int size);
int startup(int port) {
  int buf;
  struct sockaddr_in server_address;
  int server_sock = 0;
  // int socket(int family, int type, int protocal)
  server_sock =
      socket(PF_INET, SOCK_STREAM, IPPROTO_TCP); //指定IPV4协议，字节流，TCP协议
  server_address.sin_family =
      AF_INET; // 指定IPV4 他是一个较小的16位整数，在网络中不需要转换字节序
  server_address.sin_addr.s_addr = htonl(INADDR_ANY); //监听来自任何IP的数据报
  server_address.sin_port = htonl(port); //本服务器的端口是80
  //要设置的socket，socket的级别，socket的可选选项，socket传送的缓冲区，缓冲区大小
  // setsockopt(int fd, int level, int optname, const void *optval, socklen_t
  // optlen)
  if ((setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &buf, sizeof(buf))) <
      0) {
    perror("setsocketopt error");
    exit(EXIT_FAILURE);
  }
  if (bind(server_sock, (struct sockaddr *)&server_address,
           sizeof(server_address)) < 0) // bind的作用是将本地地址与套接字绑定
  {
    perror("bind error");
    exit(EXIT_FAILURE);
  }
  if (listen(server_sock, 64) < 0) {
    perror("listen error");
    exit(EXIT_FAILURE);
  }
  return server_sock;
}




void unimplemented(int sock) {
  char buf[1024];
  sprintf(buf, "HTTP/1.1 501 Method Not Implementd\r\n");
  send(sock, buf, strlen(buf), 0);
  sprintf(buf, "Server: Simple_Server\r\n");
  send(sock, buf, strlen(buf), 0);
  sprintf(buf, "\r\n");
  send(sock, buf, strlen(buf), 0);
  sprintf(buf, "<HTML><TITLE>NOT FOUND!</TITLE>\r\n");
  send(sock, buf, strlen(buf), 0);
  sprintf(buf, "<HTML><TITLE>PLEASE USE GET REQUEST!</TITLE>\r\n");
  send(sock, buf, strlen(buf), 0);
  sprintf(buf, "<HTML><HEAD><TITLE>Method Not Implemented\r\n");
  send(sock, buf, strlen(buf), 0);
  sprintf(buf, "</TITLE></HEAD>\r\n");
  send(sock, buf, strlen(buf), 0);
  sprintf(buf, "<BODY><P>HTTP request method not supported.\r\n");
  send(sock, buf, strlen(buf), 0);
  sprintf(buf, "</BODY></HTML>\r\n");
  send(sock, buf, strlen(buf), 0);
}

void not_found(int client) {
  char buf[1024];

  sprintf(buf, "HTTP/1.0 404 NOT FOUND\r\n");
  send(client, buf, strlen(buf), 0);
  sprintf(buf, "Server: Simple_Server\r\n");
  send(client, buf, strlen(buf), 0);
  sprintf(buf, "Content-Type: text/html\r\n");
  send(client, buf, strlen(buf), 0);
  sprintf(buf, "\r\n");
  send(client, buf, strlen(buf), 0);
  sprintf(buf, "<HTML><TITLE>Not Found</TITLE>\r\n");
  send(client, buf, strlen(buf), 0);
  sprintf(buf, "<BODY><P>The server could not fulfill\r\n");
  send(client, buf, strlen(buf), 0);
  sprintf(buf, "your request because the resource specified\r\n");
  send(client, buf, strlen(buf), 0);
  sprintf(buf, "is unavailable or nonexistent.\r\n");
  send(client, buf, strlen(buf), 0);
  sprintf(buf, "</BODY></HTML>\r\n");
  send(client, buf, strlen(buf), 0);
}
int get_line(int sock, char *buf, int size) {
  //读取一行
  int i = 0;
  char c = '\0';
  int n;
  while ((c != '\n') && (i < size - 1)) { //当c=='\n'的时候读取结束
    n = recv(sock, &c, 1, 0);             //阻塞
    if (n > 0) {
      if (c == '\r') {
        n = recv(sock, &c, 1, MSG_PEEK);
        if ((n > 0) && (c == '\n')) {
          recv(sock, &c, 1, 0);
        } else {
          c = '\n';
        }
      }
      buf[i] = c;
      i++;
    } else {
      c = '\n';
    }
  }
  buf[i] = '\0';
  return i;
}

void accept_request(int client_sock) {
  int numchars;     //获取读取到的信息个数
  char buf[1024];   //缓冲区
  char method[255]; // HTTP的请求方法
  char url[255];    //请求的路径
  char path[255];   //存放html页面的路径
  struct stat st;   //用来查找html文件是否存在

  numchars = get_line(client_sock, buf, sizeof(buf)); //会造成阻塞

  size_t
      i = 0,
      j = 0; // i用来计算所需数据的当前的行数，j则是用来计算这次接受报文的大小
  while (!isspace(buf[i]) && (i < sizeof(method) - 1)) {
    method[i] = buf[i];
    i++;
    j++;
  }
  method[i] = '\0'; //终止符

  if (strcasecmp(method, "GET")) {
    unimplemented(client_sock);
    perror("The method is no GET");
    return;
  }
  i = 0;
  while (isspace(buf[j]) && (j < sizeof(buf))) {
    j++;
  }
  while (!isspace(buf[j]) && (j < sizeof(buf))) {
    url[i] = buf[j];
    i++;
    j++;
  }
  url[i] = '\0';

  sprintf(path, "htmls%s", url); //通过拼接,得到要打开的网页

  if (path[strlen(path) - 1] == '/')
    strcat(path, "index.html");

  if (stat(path, &st) == -1) {
    //如果不存在，那把这次 http 的请求后续的内容(head 和 body)全部读完并忽略
    while ((numchars > 0) && strcmp("\n", buf)) /* read & discard headers */
      numchars = get_line(client_sock, buf, sizeof(buf));
    //然后返回一个找不到文件的 response 给客户端
    not_found(client_sock);
  } else {
    if ((st.st_mode & S_IFMT) == S_IFDIR)
      //如果这个文件是个目录，那就需要再在 path
      //后面拼接一个"/index.html"的字符串
      strcat(path, "/index.html");
//1111111111111111111111111111111111111111111111111111111
  }
  close(client_sock);
}

int main() {
  int server_sock = -1;
  u_short port = 80;
  int client_sock = -1;
  struct sockaddr_in client_address;
  server_sock = startup(port);
  printf("server running on port:%d\n", port);
  socklen_t client_len = sizeof(client_sock);
  while (1) {
    // accept(int fd, struct sockaddr *restrict addr, socklen_t *restrict
    // addr_len)
    client_sock =
        accept(server_sock, (struct sockaddr *)&client_address, &client_len);
    //将address的长度设置为地址是为了获得client_sock的地址
    if (client_sock == -1) {
      perror("accept error");
      exit(EXIT_FAILURE);
    }
    accept_request(client_sock);
  }
}