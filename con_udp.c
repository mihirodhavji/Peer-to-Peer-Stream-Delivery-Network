#include "con_udp.h"
#include "tcp_con.h"

/*
    efetuar ligaçao UDP ao servidor de raizes
*/
struct addrinfo * udp_connect(char ip[],char porto[],  int *fd)
{

    ssize_t n = 0;
    struct addrinfo hints, *res;

    memset(&hints,0,sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_NUMERICHOST|AI_NUMERICSERV;


    n = getaddrinfo(ip, porto, &hints,&res);
    if (n != 0 )
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(n));
        exit(1);
    }

    *fd = socket(res-> ai_family, res->ai_socktype, res->ai_protocol);
    if (*fd == -1 )
    {
        perror("Error2: ");
        exit(1);
    }
    return res;
}
/*
    envia dump,recebe e imprime resultado,
*/
ssize_t send_dump(int fd, struct addrinfo *res)
{
    ssize_t n = 0;
    char buffer[64*1024] = {0};
    struct sockaddr_in addr = {0};
    socklen_t addrlen = sizeof(addr);

    if ( send_udp_message(fd,res,DUMP) == -1){
        return -1;
    }

    n = recebe_udp_message(fd, buffer, &addrlen,&addr);

    if (n == TIMEOUT_UDP){
        return TIMEOUT_UDP;
    }
    else if (n == -1){
        return n;
    }
    else{
        if(buffer[0] != 'E'){
            write(1,buffer,n);
            return n;
        }
        else{
            printf("O servidor de raizes enviou erro. O programa vai sair\n");
            write(1,buffer,n);
            return -1;
        }
    }

}

/*
    envia uma mensagem udp, o udp funciona atomicamente: ou manda tudo ou nao manda nada
*/
ssize_t send_udp_message(int fd,struct addrinfo *res, char msg[])
{
    ssize_t n_bytes = 0;
    int tam = strlen(msg);

    n_bytes = sendto(fd,msg,tam,0,res->ai_addr, res->ai_addrlen);
    if (n_bytes == -1 )
    {
        perror("Error_send_udp: ");
        return -1;
    }
    return n_bytes;
}

/*
    recebe uma mensagem udp, espera com timeout
*/
ssize_t recebe_udp_message(int fd,char buffer[], socklen_t *addrlen, struct sockaddr_in *addr)
{
    ssize_t n = 0;
    struct  timeval timeout;
    int retval = 0;
    fd_set readfds;

    timeout.tv_sec = 5;
    timeout.tv_usec = 0;

    FD_ZERO(&readfds);

    FD_SET(fd, &readfds);

    retval = select(fd + 1, &readfds, (fd_set*)NULL, (fd_set*)NULL, &timeout);

    if (retval == 0){ ////////////deu timeout
        return TIMEOUT_UDP;
    }

    else if (FD_ISSET(fd, &readfds))
    {
        n = recvfrom(fd,buffer,64*1024,0,(struct sockaddr *)addr, addrlen);
        if (n == -1 )
        {
            perror("Error_recvfrom: ");
            return -1;
        }
    }
    return n;

}


/*
    fecha o socket_udp e limpa o res
*/
void close_udp_socket(int *fd, struct addrinfo **res)
{
    freeaddrinfo(*res);
    close(*fd);
}

/*
    controi e envia whoisroot, e analisa a resposta
*/
void send_whoisroot(parametros_entrada *entry)
{
    char buffer[64*1024] = {0},send[250] = {0},aux[250]= {0};
    ssize_t n = 0;
    struct addrinfo *res = entry->servidor_de_raizes.res;
    struct sockaddr_in addr = {0};
    socklen_t addrlen = sizeof(addr);


    if ( sprintf(send,"%s%s %s:%s\n",WHOISROOT,entry->streamID, entry->meu_endereco.ip, entry->meu_endereco.porto_udp) < 0){
        printf("error sprintf\n");
        limpa_parametros_entrada(entry);
        exit(1);
    }

    if(entry->d){printf("Mensagem de Debug:%s",send);}

    if ( send_udp_message(entry->servidor_de_raizes.fd_udp,res,send) == -1){
        perror("Error3_who_is_root: ");
        limpa_parametros_entrada(entry);
        exit(1);
    }

    n = recebe_udp_message(entry->servidor_de_raizes.fd_udp,buffer,&addrlen,&addr);
    if(n == -1){
        perror("Error4: ");
        limpa_parametros_entrada(entry);
        exit(1);
    }
    else if(n == TIMEOUT_UDP){
        printf("Timeout! Root server not responding.Closing program\n");
        limpa_parametros_entrada(entry);
        exit(1);
    }

    if(buffer[0] == 'U'){ //// recebi urroot
        entry->iamroot = true;
        if(entry->d){printf("Mensagem de Debug: Recebi:%s",buffer);}
    }
    else if(buffer[0] == 'E'){ //// recebi erro
        printf("O servidor de raizes enviou erro. O programa vai sair\n");
        printf("%s",buffer);
        limpa_parametros_entrada(entry);
        exit(1);
    }
    else{ //// recebi rootis, assim temos de decifrar a mensagem e estabeler ligacao com o tal servidor de acesso dessa arvora
        decifra_msg_udp(buffer,n-1, entry->streamID,ROOTIS,aux);
        if(entry->d){printf("Mensagem de Debug:Recebi: %s",buffer);}

        //////// porque esta função é chamada dentro da funcao mecanismo_de_init, que por sua vez é recursiva
        if ( entry->servidor_de_acesso.ip != NULL){
            free(entry->servidor_de_acesso.ip);
        }
        //////// porque esta função é chamada dentro da funcao mecanismo_de_init, que por sua vez é recursiva
        if ( entry->servidor_de_acesso.porto_udp != NULL){
            free(entry->servidor_de_acesso.porto_udp);
        }
        if (decifra_ip_porto(&entry->servidor_de_acesso.ip,&entry->servidor_de_acesso.porto_udp, aux) == ERROR_MALLOC){
            limpa_parametros_entrada(entry);
            close_udp_socket(&entry->servidor_de_raizes.fd_udp, &entry->servidor_de_raizes.res);
        }
        if(entry->d){printf("Mensagem de Debug:Novo servidor de acesso ip:%s\n",entry->servidor_de_acesso.ip);}
        if(entry->d){printf("Mensagem de Debug:Novo servidor de acesso porto:%s\n",entry->servidor_de_acesso.porto_udp);}


    }
    entry->servidor_de_raizes.res = res;
    res = NULL;
}
/*
    funcao que tira o ip e porto das mensagens de popresp e rootis
*/
void decifra_msg_udp(char msg[],int tam, char streamID[],char palavra[],char aux1[])
{
    int i = 0,tam_aux = 0;
    char aux[250]= {0};

    strcpy(aux,palavra);
    strcat(aux,streamID);
    strcat(aux," ");
    tam_aux = strlen(aux);

    for(i = tam_aux;i < tam; i++)
    {
        aux1[i-tam_aux] = msg[i];

    }
}
/*
    controi e envia o remove para tirar o registo do servidor de raizes
*/
void send_remove(parametros_entrada *entry)
{
    char send[250]= {0};
    int fd = entry->servidor_de_raizes.fd_udp;
    struct addrinfo *res = entry->servidor_de_raizes.res;

    if ( sprintf(send,"%s%s\n",REMOVE,entry->streamID) < 0){
        printf("error sprintf\n");
        exit(0);
    }
    if(entry->d){printf("Mensagem de Debug: %s\n",send);}

    if ( send_udp_message(fd,res,send) == -1){
        printf("error sending remove\n");
        exit(0);
    }

    res = NULL;

}
/*/////////////////////////////////////////////////////////////////////////////////////////////



ate aqui todas as funcoes eram para o acesso ap servidor de raizes



a partir daqui todas as funções sao para o acesso ao servidor de acesso



////////////////////////////////////////////////////////////////////////////////////////////////*/
/*
    abrir o servidor de acesso udp, so correr se for raiz
*/
void abrir_servidor_de_acesso(parametros_entrada *entry)
{
    int n = 0;
    struct addrinfo hints, *res;

    memset(&hints,0,sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE|AI_NUMERICSERV;

    n = getaddrinfo (NULL, entry->meu_endereco.porto_udp, &hints,&res);
    if (n != 0 )
    {
        perror("Error1: ");
        limpa_parametros_entrada(entry);
        exit(1);
    }

    entry->meu_endereco.fd_udp = socket(res-> ai_family, res->ai_socktype, res->ai_protocol);
    if ( entry->meu_endereco.fd_udp == -1 )
    {
        perror("Error2: ");
        limpa_parametros_entrada(entry);
        exit(1);
    }

    n = bind(entry->meu_endereco.fd_udp, res->ai_addr, res->ai_addrlen);
    if (n == -1 )
    {
        perror("Error3_abrir_servidor_de_acesso: ");
        limpa_parametros_entrada(entry);
        exit(1);
    }

    entry->meu_endereco.res = res;
    init_points_of_presence_of_tree(entry);//// guarda os proprios filhos na lista de bestpops, por isso a lista quase nunca esta vazia
    freeaddrinfo(res);
    if(entry->d){printf("Mensagem de Debug:Servidor de acesso aberto\n");}
}
/*
    procedimento a tomar se receber popreq
*/
void recebido_POPREQ(parametros_entrada *entry)
{
    ssize_t n = 0;
    struct sockaddr_in addr = {0};
    socklen_t addrlen = sizeof(addr);
    char buffer[200] = {0}, msg[64*1024] = {0};
    endereco *send = NULL;


    n = recebe_udp_message(entry->meu_endereco.fd_udp, msg, &addrlen,&addr);
    if (n == -1 )
    {
        perror("Error4: ");
        limpa_parametros_entrada(entry);
        exit(1);
    }
    else if(n == -100){

        printf("Timeout: returning\n");
        return;
    }

    if(entry->d){printf("Mensagem de Debug:%s\n",msg);}

    if (strcmp(msg,POPREQ) == 0){

        ///////////algoritmo de procura de ponto de acesso, se nao houver nada envia o ultimo
        send = procura_ponto_de_acesso(entry->points_of_presence_of_tree_head);
        //// controi a mensagem
        if (sprintf(buffer,"%s%s %s:%s\n",POPRESP,entry->streamID,send->ip,send->porto_tcp) < 0){
            free(send);
            limpa_parametros_entrada(entry);
            exit(0);
        }
        free(send);

        n=sendto(entry->meu_endereco.fd_udp, buffer,strlen(buffer),0,(struct sockaddr *)&addr, addrlen);
        if (n == -1 )
        {
            perror("Error5: ");
            limpa_parametros_entrada(entry);
            exit(1);
        }

        if(entry->d){printf("Mensagem de Debug:Enviei resposta do popreq : %s",buffer);}
    }

}

/*
    procura um endereço ip:porto com tcp sessions livre e retorna isso
*/
endereco *procura_ponto_de_acesso(ponto_presenca *head)
{
    ponto_presenca *aux = head;
    endereco *add = NULL;

    add = (endereco * )malloc(sizeof(endereco));
    if (add == NULL){
        ///////funcao de limpeza
        exit(0);
    }

    if (aux->avail > 0){
        add->ip = aux->ip;
        add->porto_tcp = aux->porto_tcp;
        aux->avail -=1;
        return add;
    }

    while (aux->next != NULL){
        if (aux->next->avail > 0){
            add->ip = aux->next->ip;
            add->porto_tcp = aux->next->porto_tcp;
            aux->next->avail -=1;
            return add;
        }
        aux = aux->next;
    }
    ///////////////se nao houver nenhum disponivel manda-se o endereco da raiz, porque ai eu tenho a certeza que existe
    /////////////// e vai fazer redirect
    add->ip = head->ip;
    add->porto_tcp = head->porto_tcp;

    return add;
}

/*
    funcao que envia popreq ao servidor de acesso e aguarda resposta
*/
ssize_t send_POPREQ(parametros_entrada *entry)
{
    char send[50]= {0}, aux1[100]= {0}, buffer[64*1024]= {0};
    ssize_t n = 0;
    struct addrinfo hints, *res;
    struct sockaddr_in addr = {0};
    socklen_t addrlen = sizeof(addr);
    int fd = 0;

    memset(&hints,0,sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_NUMERICSERV;


    n = getaddrinfo(entry->servidor_de_acesso.ip, entry->servidor_de_acesso.porto_udp, &hints,&res);
    if (n != 0 )
    {
        perror("Error1: ");
        limpa_parametros_entrada(entry);
        exit(1);
    }

    fd = socket(res-> ai_family, res->ai_socktype, res->ai_protocol);
    if (fd == -1 )
    {
        perror("Error2: ");
        limpa_parametros_entrada(entry);
        exit(1);
    }
    /// controi e envia mensagem
    if (sprintf(send,"%s",POPREQ) < 0){
        printf("error sprintf\n");
        exit(0);
    }

    if(entry->d){printf("Mensagem de Debug:Enviei %s\n",send);}

    if ( send_udp_message(fd,res,send) == -1){
        printf("error sending popreq\n");
        limpa_parametros_entrada(entry);
        exit(1);
    }
    ///// aguarda resposta
    n = recebe_udp_message(fd, buffer,&addrlen,&addr);
    if (n == -1)
    {
        printf("error receiving popresp\n");
        limpa_parametros_entrada(entry);
        freeaddrinfo(res);
        exit(1);
    }
    if (n == TIMEOUT_UDP)// deu timeout
    {
        printf("timeout no receive_udp\n");
        freeaddrinfo(res);
        return TIMEOUT_UDP;
    }

    if(entry->d){printf("Mensagem de Debug:Recebi %s\n",buffer);}
    ////// decifrar a mensagem
    decifra_msg_udp(buffer, strlen(buffer) - 1, entry->streamID,POPRESP,aux1);/////// o -1 é para tirar o \n

    free(entry->fonte.ip);
    free(entry->fonte.porto_tcp);

    entry->fonte.ip = NULL;
    entry->fonte.porto_tcp = NULL;
    ///saber quais sao o novo porto e ip da fonte, de onde vais fazer download do stream
    if (decifra_ip_porto(&entry->fonte.ip,&entry->fonte.porto_tcp, aux1) == ERROR_MALLOC){
            perror("error malloc : ");
            limpa_parametros_entrada(entry);
            close_udp_socket(&entry->servidor_de_acesso.fd_udp, &res);
    }
    if (strcmp(entry->fonte.ip,entry->meu_endereco.ip) == 0 && strcmp(entry->fonte.porto_tcp,entry->meu_endereco.porto_tcp) == 0){
        if(entry->d){printf("Mensagem de Debug:Recebi meu proprio ip:porto.\n");}
        if ( send_POPREQ(entry) == TIMEOUT_UDP)
            return TIMEOUT_UDP;
        else
            return 0;
    }
    freeaddrinfo(res);
    close(fd);

    if(entry->d){
      printf("Mensagem de Debug:Fonte ip : %s\n",entry->fonte.ip);
      printf("Mensagem de Debug:Fonte porto : %s\n",entry->fonte.porto_tcp);
    }
    return 0;
}
