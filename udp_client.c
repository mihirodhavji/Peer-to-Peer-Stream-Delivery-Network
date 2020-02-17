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

int main ()
{
    int fd, n;
    struct addrinfo hints, *res;
    struct sockaddr_in addr;
    char buffer[128];
    
    socklen_t addrlen = sizeof(addr);

    memset(&hints,0,sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_NUMERICSERV;

    n = getaddrinfo ("mihir-X556URK", PORT, &hints,&res);
    if (n != 0 )
    {
        perror("Error1: ");
        exit(1);
    }

    fd = socket(res-> ai_family, res->ai_socktype, res->ai_protocol);
    if (fd == -1 )
    {
        perror("Error2: ");
        exit(1);
    }

    n=sendto(fd, "Hello!\n", 7,0,res->ai_addr, res->ai_addrlen);
    if (n == -1 )
    {
        perror("Error3: ");
        exit(1);
    }
    
    n = recvfrom(fd,buffer,128,0,(struct sockaddr *)&addr, &addrlen);
    if (n == -1 )
    {
        perror("Error4: ");
        exit(1);
    }

    printf("chegou\n");

    write(1,"echo: ",6);
    write(1,buffer,n);

    freeaddrinfo(res);
    close(fd);


}
