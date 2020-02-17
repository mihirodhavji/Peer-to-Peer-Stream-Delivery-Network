#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <stdio.h>
#include <errno.h>
#include <string.h>

#define PORT "58001"

int main(){

    int fd;
    ssize_t n;
    struct addrinfo hints, *res;
    //struct sockaddr_in addr;
    char buffer[128];

    memset(&hints, 0, sizeof(hints) );
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_NUMERICSERV;

    n = getaddrinfo("mihir-X556URK", PORT, &hints, &res);
    if (n != 0 )
    {
        perror("Error: ");
        exit(1);
    }

    fd = socket(res-> ai_family, res->ai_socktype, res->ai_protocol);
    if (fd == -1 )
    {
        perror("Error: ");
        exit(1);
    }

    n = connect(fd, res->ai_addr, res->ai_addrlen);
    if (n == -1 )
    {
        perror("Error: ");
        exit(1);
    }

    n = write(fd, "Hello!\n", 7);
    if (n == -1 )
    {
        perror("Error: ");
        exit(1);
    }

    n = read(fd, buffer, 128);
    if (n == -1 )
    {
        perror("Error: ");
        exit(1);
    }

    write(1,"echo: ",6);
    write(1,buffer,n);

    freeaddrinfo(res);
    close(fd);
}
