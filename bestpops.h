#ifndef BESTPOPS_H
#define BESTPOPS_H

#include "init.h"
#include "con_udp.h"
#include "tcp_con.h"

void send_pop_query(parametros_entrada *entry, int queryID, int bestpops);

void recebi_pop_query(parametros_entrada *entry, char msg[]);

void decifra_pop_query(char msg[], int *bp, char query[5]);

void send_pop_reply(parametros_entrada *entry, int ID, char ip[],char porto[],int avail);

void recebi_pop_reply(parametros_entrada *entry, char msg[]);

void decifra_pop_reply(char msg_original[], char **ip, char **porto,int *avail);

void regista_pedido(int ID, int bp, pedido *solicitacao);

int retira_registo(parametros_entrada *entry, pedido resposta, int ID_msg);

void pop_query_needed(parametros_entrada *entry);

int conta_avail(ponto_presenca *head);

#endif 