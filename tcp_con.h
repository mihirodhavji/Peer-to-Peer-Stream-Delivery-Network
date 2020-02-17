#ifndef TCP_CON_H
#define TCP_CON_H

#include "init.h"
#include "con_udp.h"
#include "bestpops.h"


int ligar_a_fonte(parametros_entrada *entry);

void ser_fonte(parametros_entrada *entry);

void fazer_accept_tcp(parametros_entrada *entry);

void decifra_info_fonte(parametros_entrada *entry);

int send_welcome(int local, parametros_entrada *entry);

void send_redirect(parametros_entrada *entry, int fd);

ssize_t recebe_mensagem_tcp(int fd, char buffer[],ssize_t tam);

void send_tree_query(parametros_entrada *entry, char tree_ip[],char tree_port[], int local);

void send_tree_reply(parametros_entrada *entry);

ssize_t manda_para_um_jusante(parametros_entrada *entry, char buffer[], int local);

void manda_para_todos_jusante(parametros_entrada *entry, char buffer[], int maneira);

ssize_t mensagem_para_montante(parametros_entrada *entry, char msg[]);

ssize_t send_new_pop(parametros_entrada *entry);

ssize_t le_letra_a_letra(int max,int fd, char msg[], char final[]);

void decifra_info_jusante(parametros_entrada *entry,int local);

void montante_lost(parametros_entrada *entry);

void jusante_lost(parametros_entrada *entry, int local);

ssize_t le_letra_a_letra(int max,int fd, char msg[], char aux[]);

#endif