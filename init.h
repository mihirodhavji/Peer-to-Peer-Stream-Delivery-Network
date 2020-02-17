#ifndef INIT_H
#define INIT_H

#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <sys/time.h>
#include <sys/select.h>
#include <time.h>
#include <assert.h>
#include <ctype.h>
#include <sys/timerfd.h>


#define ERROR_MALLOC 100
#define ERROR_WRITE -1
#define TIMEOUT_UDP -100
#define ERROR_TCP -10

#define TAM_MAX 63
#define MINUS_I 0
#define MINUS_T 1
#define MINUS_U 2
#define MINUS_S 3
#define MINUS_P 4
#define MINUS_N 5
#define MINUS_X 6
#define MINUS_B 7
#define MINUS_D 8
#define MINUS_H 9

#define TCP_PORT "58000"
#define UDP_PORT "58000"
#define ROOT_SERVER_IP "192.168.1.1"/*"193.136.138.142"*/
#define UDP_PORT_SERVER "59000"
#define TCP_SESSIONS 1
#define BESTPOPS 1
#define TSECS 5

#define BESTPOPSTIMER 15

#define DUMP "DUMP\n" //// done
#define WHOISROOT "WHOISROOT "////done
#define ROOTIS "ROOTIS "////done
#define URROOT "URROOT "////done
#define REMOVE "REMOVE "////done

#define POPREQ "POPREQ\n" ////done
#define POPRESP "POPRESP " ////done

#define streams "streams\n" ////done
#define status "status\n" /////done
#define display_on "display on\n" ////done
#define display_off "display off\n" ////done
#define format_ascii "format ascii\n" /////done
#define format_hex "format hex\n" /////done
#define debug_on "debug on\n" ////done
#define debug_off "debug off\n" ////done
#define TREE "tree\n" /////done
#define EXIT "exit\n" ////done

#define WELCOME "WE "/////done
#define NEW_POP "NP "/////done
#define REDIRECT "RE " /////done
#define STREAM_FLOWING "SF\n" /////done
#define BROKEN_STREAM "BS\n" /////done
#define DATA "DA "           //////done
#define POP_QUERY "PQ " /////done
#define POP_REPLY "PR " /////done
#define TREE_QUERY "TQ " ///done
#define TREE_REPLY "TR " ////done
#define max(A,B) ((A)>=(B)?(A):(B))
#define min(A,B) ((A)<(B)?(A):(B))


typedef struct ponto_presenca_{
    char *ip;
    char *porto_tcp;
    int avail;
    struct ponto_presenca_ *next;
}ponto_presenca;

typedef struct pedido_{
    int queryID;
    int bestpops;
}pedido;

typedef struct endereco{
    char *ip;
    char *porto_udp;
    char *porto_tcp;
    int fd_tcp;
    int fd_udp;
    struct addrinfo *res;
    bool being_used;
}endereco;

typedef struct parametros_entrada{
    char *streamID;
    endereco fonte;
    endereco meu_endereco;
    endereco servidor_de_raizes;
    endereco servidor_de_acesso;
    endereco *filhos;
    ponto_presenca *points_of_presence_of_tree_head;
    pedido pedido_efetuado;
    int tcp_sessions;
    int bestpops;
    int tsecs;
    int queryID;
    int meu_filhos_ocupados;
    bool b;
    bool d;
    bool iamroot;
    bool will_to_exit;
    bool stream_flowing;
    bool f_ascii;
    struct timespec bestpops_timer;
    struct itimerspec bestpops_timer2;
    struct timespec root_renew_timer;
    struct itimerspec root_renew_timer2;
    int pop_query_fd;
    int root_renew_fd;
} parametros_entrada;

void verifica_argumentos_de_arranque(int argc, char *argv[], parametros_entrada *entry);

void quais_sao_parametros_de_entrada(int argc, char *argv[], int flags[]);

int procura_flag_do_parametro(char *aux, char *argv[], int max);

void imprimir_sinopse_linha_de_comandos();

bool existe(int a);

int verifica_segundo_argumento_e_streamID(char argv[]);

int guarda_arg_int(int local, char *argv[], int por_omissao);

int identifica_streamID_ip_port_da_fonte( char **streamID, char **fonte_ip, char **fonte_port, char *string_original);

int decifra_ip_porto(char **ip, char **port, char *string_original);

void limpa_parametros_entrada(parametros_entrada *entry);

void limpa_lista_bestpops(ponto_presenca *head);

void init_points_of_presence_of_tree(parametros_entrada *entry);

void adiciona_point_of_presence(parametros_entrada *entry, ponto_presenca **head,char ip[], char porto_tcp[], int avail);

void incrementa(int *ID);

void imprime_argumentos_de_entrada(parametros_entrada entry);

void ler_informacao_linha_comandos(parametros_entrada *entry);

void imprime_status(parametros_entrada *entry);

void mecanismo_de_init(parametros_entrada *entry, int flag_u, int flag_tcp);

void mecanismo_de_re_init(parametros_entrada *entry, int flag_u, int flag_tcp);

void imprime_data(parametros_entrada *entry, char data[], bool flag);

#endif
