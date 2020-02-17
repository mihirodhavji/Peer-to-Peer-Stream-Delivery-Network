#include "init.h"
#include "con_udp.h"
#include "tcp_con.h"
#include "bestpops.h"

void desligar()
{
    signal(SIGINT,SIG_IGN);
    printf("\nDetectado ctrl + C\nMas para sair do programa é com o exit na linha de comandos.\n\n");
    signal(SIGINT,desligar);
    return;
}

int main(int argc, char *argv[])
{
    parametros_entrada entry = {0};
    fd_set readfds;
    int retval = 0, maxfd = 0, i = 0;
    struct sigaction act;
    srandom(time(NULL));

    verifica_argumentos_de_arranque(argc,argv,&entry);

    memset(&act,0,sizeof act);
    act.sa_handler = desligar;
    if ( sigaction(SIGINT, &act, NULL) != 0){
        perror("sigaction:");
        limpa_parametros_entrada(&entry);
        exit(0);
    }
    signal(SIGPIPE,SIG_IGN);
    signal(SIGINT,desligar);

    entry.servidor_de_raizes.res = udp_connect(entry.servidor_de_raizes.ip,entry.servidor_de_raizes.porto_udp, &entry.servidor_de_raizes.fd_udp);
    mecanismo_de_init(&entry,0,0);

    if (entry.iamroot){
        if ( timerfd_settime(entry.pop_query_fd,0,&entry.bestpops_timer2,( struct itimerspec *)NULL) == -1){
            perror("error pop_query_timerfd_settime:");
            limpa_parametros_entrada(&entry);
            exit(0);
        }
        if ( timerfd_settime(entry.root_renew_fd,0,&entry.root_renew_timer2,( struct itimerspec *)NULL) == -1){
            perror("error root_renew_timerfd_settime:");
            limpa_parametros_entrada(&entry);
            exit(0);
        }
    }

    while (!entry.will_to_exit)
    {
        maxfd = 0;

        FD_ZERO(&readfds);

        FD_SET(0, &readfds);//// para ler do terminal
        maxfd = max(maxfd,0);

        FD_SET(entry.fonte.fd_tcp, &readfds); /// para ler aquilo que a fonte enviou
        maxfd = max(maxfd,entry.fonte.fd_tcp);

        FD_SET(entry.meu_endereco.fd_tcp, &readfds); /// para fazer os accpets de tcp
        maxfd = max(maxfd,entry.meu_endereco.fd_tcp);

        if (entry.iamroot){ ///// para o servidor de acesso(so faz se fores root), e o timer do bestpops e do renew do registo no servidor de raizes

            FD_SET(entry.meu_endereco.fd_udp, &readfds);
            maxfd = max(maxfd,entry.meu_endereco.fd_udp);

            FD_SET(entry.pop_query_fd,&readfds);
            maxfd = max(maxfd,entry.pop_query_fd);

            FD_SET(entry.root_renew_fd,&readfds);
            maxfd = max(maxfd,entry.root_renew_fd);
        }

        for (i = 0;i < entry.tcp_sessions; i++) ////// se receber mensagem de pontos a jusante
        {
            if(entry.filhos[i].being_used){
                FD_SET(entry.filhos[i].fd_tcp, &readfds);
                maxfd = max(maxfd,entry.filhos[i].fd_tcp);
            }
        }

        retval = select(maxfd + 1, &readfds, (fd_set*)NULL, (fd_set*)NULL, (struct  timeval *)NULL);
        if (retval == -1){ ///// deu erro
            perror("select()");
            limpa_parametros_entrada(&entry);
            exit(0);
        }

        if(entry.iamroot && FD_ISSET(entry.root_renew_fd, &readfds)){ /////// para a renovacao do whoisroot
            send_whoisroot(&entry);
            entry.root_renew_timer2.it_value = entry.root_renew_timer;
            entry.root_renew_timer2.it_interval = entry.root_renew_timer;
            if ( timerfd_settime(entry.root_renew_fd,0,&entry.root_renew_timer2,( struct itimerspec *)NULL) == -1){
                perror("error root_renew_timerfd_settime:");
                limpa_parametros_entrada(&entry);
                exit(0);
            }

        }

        if(entry.iamroot && FD_ISSET(entry.pop_query_fd, &readfds)){ ///// para ver o timer de atualizacao para enviar o pop_query
            if(entry.d){printf("Mensagem de Debug: Avail:%d\n",conta_avail(entry.points_of_presence_of_tree_head));}
            pop_query_needed(&entry);
            entry.bestpops_timer2.it_value = entry.bestpops_timer;
            entry.bestpops_timer2.it_interval = entry.bestpops_timer;
            if ( timerfd_settime(entry.pop_query_fd,0,&entry.bestpops_timer2,( struct itimerspec *)NULL) == -1){
                perror("error pop_query_timerfd_settime:");
                limpa_parametros_entrada(&entry);
                exit(0);
            }
        }

        if (FD_ISSET(0, &readfds)){ //// utilizador escreveu algum na linha de comandos
            if(entry.d){printf("Mensagem de Debug: Há informação no teclado para ser lida\n");}
            ler_informacao_linha_comandos(&entry);
        }

        if ( FD_ISSET(entry.meu_endereco.fd_tcp, &readfds) ){ //// recebi um pedido de conecao tcp
            if(entry.d){printf("Mensagem de Debug: Connection a jusante request\n");}
            fazer_accept_tcp(&entry);
        }

        if ( entry.iamroot && FD_ISSET(entry.meu_endereco.fd_udp, &readfds) ){ //// alguem quer ligar-se a esta arvore, pedido no servidor de acesso
            if(entry.d){printf("Mensagem de Debug: Tree entry request\n");}
            recebido_POPREQ(&entry);
        }

        if ( FD_ISSET(entry.fonte.fd_tcp, &readfds) ){ //// fonte enviou algum
            decifra_info_fonte(&entry);
        }

        for (i = 0;i < entry.tcp_sessions; i++)
        {
            if (entry.filhos[i].being_used){////é necessario porque eu posso aceitar N ligações, mas apenas uma esta ativa.
                if (FD_ISSET(entry.filhos[i].fd_tcp, &readfds)){ ////////////para ler aquilo que os filhos enviaram
                    if(entry.d){printf("Mensagem de Debug: Information a jusante\n");}
                    decifra_info_jusante(&entry,i);
                }
            }
        }
    }
    printf("O programa vai sair\n");
    limpa_parametros_entrada(&entry);
    exit(0);
}