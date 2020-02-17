#ifndef CON_UDP_H
#define CON_UDP_H

#include "init.h"

struct addrinfo * udp_connect(char ip[],char porto[],  int *fd);

ssize_t send_dump(int fd, struct addrinfo *res);

ssize_t send_udp_message(int fd,struct addrinfo *res, char msg[]);

ssize_t recebe_udp_message(int fd,char buffer[], socklen_t *addrlen, struct sockaddr_in *addr);

void close_udp_socket(int *fd, struct addrinfo **res);

void send_whoisroot(parametros_entrada *entry);

void decifra_msg_udp(char msg[],int tam, char streamID[],char palavra[],char aux1[]);

void send_remove(parametros_entrada *entry);

////////////// a partir daqui sao funcoes do servidor de acesso
void abrir_servidor_de_acesso(parametros_entrada *entry);

void recebido_POPREQ(parametros_entrada *entry);

endereco *procura_ponto_de_acesso(ponto_presenca *head);

ssize_t send_POPREQ(parametros_entrada *entry);

#endif