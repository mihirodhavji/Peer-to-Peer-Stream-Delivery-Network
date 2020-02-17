#include "init.h"
#include "con_udp.h"
#include "tcp_con.h"
/*
    funcao que verifica, guarda todas os parametros
*/
void verifica_argumentos_de_arranque (int argc, char *argv[], parametros_entrada *entry)
{
    int flags[10] = {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};
    int i = 0;

    entry->streamID = NULL;
    entry->fonte.ip = NULL;
    entry->fonte.porto_tcp = NULL;
    entry->meu_endereco.ip = NULL;
    entry->meu_endereco.porto_tcp = NULL;
    entry->meu_endereco.porto_udp = NULL;
    entry->servidor_de_raizes.ip = NULL;
    entry->servidor_de_raizes.porto_udp = NULL;
    entry->servidor_de_raizes.res = NULL;
    entry->servidor_de_acesso.ip = NULL;
    entry->servidor_de_acesso.porto_udp = NULL;
    entry->filhos = NULL;
    entry->will_to_exit = false;
    entry->stream_flowing = false;
    entry->iamroot = false;
    entry->b = true;
    entry->d = false;
    entry->f_ascii = true;
    entry->points_of_presence_of_tree_head = NULL;

    if (argc == 1){ /// caso em chamou o programa sem nenhum parametro, ou seja, ./iamroot
        entry->servidor_de_raizes.res = udp_connect(ROOT_SERVER_IP,UDP_PORT_SERVER, &entry->servidor_de_raizes.fd_udp);
        send_dump(entry->servidor_de_raizes.fd_udp, entry->servidor_de_raizes.res);
        close_udp_socket(&entry->servidor_de_raizes.fd_udp, &entry->servidor_de_raizes.res);
        exit(0);
    }
    ///descobrir que parametros de entrada existem
    quais_sao_parametros_de_entrada(argc, argv, flags);

    if (existe(flags[MINUS_H])){ ///// funcao de help
        imprimir_sinopse_linha_de_comandos();
        exit(0);
    }
    if (existe(flags[MINUS_B])){ ///// funcao da flag -b
        entry->b = false;
    }
    else{
        entry->b = true;
    }

    if (existe(flags[MINUS_D])){ ///// funcao da flag -d
        entry->d = true;
    }
    else{
        entry->d = false;
    }

    if ( verifica_segundo_argumento_e_streamID(argv[1]) == 1){ //// verificar se o segundo argumento é o streamID
        printf("foi detectado erro no nome da stream. Inválido!\n");
        entry->servidor_de_raizes.res = udp_connect(ROOT_SERVER_IP,UDP_PORT_SERVER, &entry->servidor_de_raizes.fd_udp);
        send_dump(entry->servidor_de_raizes.fd_udp, entry->servidor_de_raizes.res);
        close_udp_socket(&entry->servidor_de_raizes.fd_udp, &entry->servidor_de_raizes.res);
        exit(0);
    }
    ////// guardar parametros do tipo int e verificar se sao positivos
    entry->tcp_sessions = guarda_arg_int(flags[MINUS_P],argv,TCP_SESSIONS);
    entry->bestpops = guarda_arg_int(flags[MINUS_N],argv,BESTPOPS);
    entry->tsecs =  guarda_arg_int(flags[MINUS_X],argv,TSECS);

    if (entry->tcp_sessions < 1 || entry->bestpops < 1 || entry->tsecs < 1){
        printf("Há um parametro que é nulo ou negativo, mas devia ser positivo\n");
        exit(0);
    }

    entry->bestpops_timer.tv_sec = BESTPOPSTIMER;
    entry->bestpops_timer.tv_nsec = 0;

    entry->bestpops_timer2.it_value = entry->bestpops_timer;
    entry->bestpops_timer2.it_interval = entry->bestpops_timer;

    entry->root_renew_timer.tv_sec = entry->tsecs;
    entry->root_renew_timer.tv_nsec = 0;

    entry->root_renew_timer2.it_value = entry->root_renew_timer;
    entry->root_renew_timer2.it_interval = entry->root_renew_timer;

    entry->pop_query_fd = timerfd_create(CLOCK_MONOTONIC, 0);
    if (entry->pop_query_fd == -1){
        perror("error pop_query_timerfd_create:");
        exit(0);
    }

    entry->root_renew_fd = timerfd_create(CLOCK_MONOTONIC, 0);
    if (entry->root_renew_fd == -1){
        perror("error pop_query_timerfd_create:");
        exit(0);
    }

    entry->filhos = (endereco *)malloc(sizeof(endereco) * (entry->tcp_sessions + 1 ) );/// esta dúvida do mais 1
    if (entry->filhos == NULL)
    {
        perror("Malloc : ");
        limpa_parametros_entrada(entry);
        exit(0);
    }
    //// inicializar os filhos
    for (i = 0; i < entry->tcp_sessions+1;i++)
    {
        entry->filhos[i].ip = NULL;
        entry->filhos[i].porto_tcp = NULL;
        entry->filhos[i].being_used = false;
    }
    ////// copiar o streamID, o ip e o porto da fonte
    if (identifica_streamID_ip_port_da_fonte(&entry->streamID, &entry->fonte.ip, &entry->fonte.porto_tcp, argv[1]) == ERROR_MALLOC){
        limpa_parametros_entrada(entry);
        exit(0);
    }
    ///////////////////to lower streamID
    for ( i = 0; entry->streamID[i] != ':' ; i++)
    {
        entry->streamID[i] = tolower(entry->streamID[i]);
    }

    //// copiar o meu ip
    if (existe(flags[MINUS_I])){
        entry->meu_endereco.ip = (char *)malloc(sizeof(char) * (strlen(argv[flags[MINUS_I] +1]) + 1));
        if (entry->meu_endereco.ip == NULL){
            perror("Malloc : ");
            limpa_parametros_entrada(entry);
            exit(0);
        }
        if ( sprintf(entry->meu_endereco.ip,"%s",argv[flags[MINUS_I]+1]) < 0){
            printf("error sprintf\n");
            limpa_parametros_entrada(entry);
            exit(0);
        }
    }
    else
    {
        printf("Foi detectado erro na chamada da funcao.Falta -i.Invalido!\n");
        limpa_parametros_entrada(entry);
        exit(0);
    }


    //// copiar o meu porto de entrada
    if (existe(flags[MINUS_T])){
        entry->meu_endereco.porto_tcp = (char *)malloc(sizeof(char) * (strlen(argv[flags[MINUS_T] +1]) + 1));
        if (entry->meu_endereco.porto_tcp == NULL){
            perror("Malloc : ");
            limpa_parametros_entrada(entry);
            exit(0);
        }
        if ( sprintf(entry->meu_endereco.porto_tcp,"%s",argv[flags[MINUS_T]+1]) < 0){
            printf("error sprintf\n");
            limpa_parametros_entrada(entry);
            exit(0);
        }
    }
    else /// se o porto de entrada for omitido
    {
        entry->meu_endereco.porto_tcp = (char *)malloc(sizeof(char) * (strlen(TCP_PORT)+1 ));
        if (entry->meu_endereco.porto_tcp == NULL){
            perror("Malloc : ");
            limpa_parametros_entrada(entry);
            exit(0);
        }
        if ( sprintf(entry->meu_endereco.porto_tcp,"%s",TCP_PORT) < 0){
            printf("error sprintf\n");
            limpa_parametros_entrada(entry);
            exit(0);
        }
    }


    ///// copiar porto UDP do meu servidor de acesso
    if (existe(flags[MINUS_U])){
        entry->meu_endereco.porto_udp = (char *)malloc(sizeof(char) * (strlen(argv[flags[MINUS_U] +1]) + 1));
        if (entry->meu_endereco.porto_udp == NULL){
            perror("Malloc : ");
            limpa_parametros_entrada(entry);
            exit(0);
        }
        if ( sprintf(entry->meu_endereco.porto_udp,"%s",argv[flags[MINUS_U]+1]) < 0){
            printf("error sprintf\n");
            limpa_parametros_entrada(entry);
            exit(0);
        }
    }
    else /// se o porto UDP do meu servidor de acesso for omitido
    {
        entry->meu_endereco.porto_udp = (char *)malloc(sizeof(char) * (strlen(UDP_PORT) +1 ));
        if (entry->meu_endereco.porto_udp == NULL){
            perror("Malloc : ");
            limpa_parametros_entrada(entry);
            exit(0);
        }
        if ( sprintf(entry->meu_endereco.porto_udp,"%s",UDP_PORT) < 0){
            printf("error sprintf\n");
            limpa_parametros_entrada(entry);
            exit(0);
        }
    }


    //////////// copiar ip e porto do servidor de raizes
    if (existe(flags[MINUS_S])){
        if ( decifra_ip_porto(&entry->servidor_de_raizes.ip, &entry->servidor_de_raizes.porto_udp, argv[flags[MINUS_S] +1]) == ERROR_MALLOC){
            perror("Malloc ou sprintf : ");
            limpa_parametros_entrada(entry);
            exit(0);
        }
    }
    else /////se o ip e o porto forem omitidos
    {
        entry->servidor_de_raizes.ip = (char *)malloc(sizeof(char) * (strlen(ROOT_SERVER_IP)+1));
        if (entry->servidor_de_raizes.ip == NULL){
            perror("Malloc : ");
            limpa_parametros_entrada(entry);
            exit(0);
        }
        if ( sprintf(entry->servidor_de_raizes.ip,"%s",ROOT_SERVER_IP) < 0){
            printf("error sprintf\n");
            limpa_parametros_entrada(entry);
            exit(0);
        }

        entry->servidor_de_raizes.porto_udp = (char *)malloc(sizeof(char) * (strlen(UDP_PORT_SERVER)+1));
        if (entry->servidor_de_raizes.porto_udp == NULL){
            perror("Malloc : ");
            limpa_parametros_entrada(entry);
            exit(0);
        }
        if (sprintf(entry->servidor_de_raizes.porto_udp,"%s",UDP_PORT_SERVER) < 0){
            printf("error sprintf\n");
            limpa_parametros_entrada(entry);
            exit(0);
        }
    }

}
/*
    funcao que indica quais sao os argumentos de entrada, e quais sao omitidos
*/
void quais_sao_parametros_de_entrada(int argc, char *argv[], int flags[])
{
    int j = 0;
    char *aux[10] = {"-i","-t","-u","-s","-p","-n","-x","-b","-d","-h"};

    for(j = 0; j < 10; j++)
    {
        flags[j] = procura_flag_do_parametro(aux[j], argv, argc);
    }

}

/*
    funcao que guarda a posicao das varias flags com as quais o programa foi lançado
*/
int procura_flag_do_parametro(char *aux, char *argv[], int max)
{
    int i = 0;
    for (i = 0; i < max; i++){
        if ( strcmp(argv[i],aux) == 0){
            return i;
        }
    }
    return -1;
}

/*
    funcao que imprime a sinopse da linha de comandos
*/
void imprimir_sinopse_linha_de_comandos()
{
    printf("iamroot [<streamID>] [-i <ipaddr>] [-t <tport>] [-u <uport>]\n\t\t"
    "[-s <rsaddr>[:<rsport>]]\n\t\t[-p <tcpsessions>]\n\t\t[-n <bestpops>] [-x <tsecs>]\n\t\t[-b] [-d] [-h]\n");
}
/*
    funcao que retorna se dada posicao existe ou nao
*/
bool existe(int a)
{
    if ( a != -1){
        return true;
    }
    else{
        return false;
    }
}
/*
    funcao que verifica se o segundo argumento é a streamID ou não
*/
int verifica_segundo_argumento_e_streamID(char argv[])
{
    int i = 0;

    /*  procura onde esta o ":", basicamente o ':' está no 2º argumento,
        se nao estiver é porque a streamID tem um espaço ou nao está no lugar correto*/
    for ( i = 0; i < strlen(argv); i++)
    {
        if ( argv[i] == ':'){
        return 0;
        }

    }
    return 1;
}
/*
    funcao que guarda ou um inteiro vindo da linha de comando ou o valor default previamente defenido
*/
int guarda_arg_int(int local, char *argv[], int por_omissao)
{
    if (existe(local)){
        return atoi(argv[local+1]);
    }
    else{
        return por_omissao;
    }
}

/*
    funcao que analisa a streamID e guarda o nome, o ip e o porto da fonte
*/
int identifica_streamID_ip_port_da_fonte(char **streamID, char **fonte_ip, char **fonte_port, char *string_original)
{
    const char double_dot[2] = ":";
    char *token = NULL;
    char aux[65];
    ///// guarda streamID
    *streamID = (char *)malloc(sizeof(char) * (strlen(string_original)+1));
    if (*streamID == NULL){
        perror("Malloc : ");
        exit(0);
    }
    if ( sprintf(*streamID,"%s",string_original) < 0){
        printf("error sprintf\n");
        return ERROR_MALLOC;
    }
    ////// se o nome tem mais de 63 carateres
    token = strtok(string_original,double_dot);
    if (strlen(token) > TAM_MAX){
        printf("nome do stream tem mais de 63 carateres\n");
        return ERROR_MALLOC;
    }
    ///// ver tamanho para guardar o ip da fonte
    token = strtok(NULL, double_dot);
    *fonte_ip = (char *)malloc(sizeof(char) * (strlen(token)+1));
    if (*fonte_ip == NULL){
        perror("Malloc : ");
        free(*streamID);
        return ERROR_MALLOC;
    }
    ///// ver tamanho para guardar o porto da fonte
    token = strtok(NULL, double_dot);
    *fonte_port = (char *)malloc(sizeof(char) * (strlen(token)+1));
    if (*fonte_port == NULL){
        perror("Malloc : ");
        free(*streamID);
        free(*fonte_ip);
        return ERROR_MALLOC;
    }
    ///// ler o ip e porto a partir do streamID
    if ( sscanf(*streamID,"%[^:]:%[^:]:%s",aux,*fonte_ip,*fonte_port) != 3){
        printf("error sscanf\n");
        return ERROR_MALLOC;
    }

    return 0;
}


/*
    funcao que analisa que tira um ip e porto de uma string original do estilo ip:porto
*/
int decifra_ip_porto(char **ip, char **port, char *string_original)
{
    int i = 0, flag = 0;
    const char double_dot[2] = ":";
    char *token = NULL;
    char *aux = NULL;

    aux = (char *)malloc(sizeof(char) * (strlen(string_original) + 1) );
    if (aux == NULL){
        return ERROR_MALLOC;
    }

    if ( sprintf(aux,"%s",string_original) < 0){
        free(aux);
        return ERROR_MALLOC;
    }

    for ( i = 0; i < strlen(aux); i++)
    {
        if ( aux[i] == ':'){
            flag = 1;//// tem ip e porto
            break;
        }
    }
    if (flag == 0){/// so tem ip, porto esta omitido

        *ip = (char *)malloc(sizeof(char) * (strlen(aux)+1));
        if (*ip == NULL){
            free(aux);
            return ERROR_MALLOC;
        }

        if ( sprintf(*ip,"%s",aux) < 0){
            free(aux);
            free(*ip);
            return ERROR_MALLOC;
        }

        *port = (char *)malloc(sizeof(char) * (strlen(UDP_PORT_SERVER)+1));
        if (*port == NULL){
            free(aux);
            free(*ip);
            return ERROR_MALLOC;
        }

        if ( sprintf(*port,"%s",UDP_PORT_SERVER) < 0){
            free(aux);
            free(*ip);
            free(*port);
            return ERROR_MALLOC;
        }
    }
    else{ //// tem porto e ip
        token = strtok(aux,double_dot);
        *ip = (char *)malloc(sizeof(char) * (strlen(token)+1));
        if (*ip == NULL){
            free(aux);
            return ERROR_MALLOC;
        }

        if ( sprintf(*ip,"%s",token) < 0){
            free(aux);
            free(*ip);
            return ERROR_MALLOC;
        }

        token = strtok(NULL, double_dot);
        *port = (char *)malloc(sizeof(char) * (strlen(token)+1));
        if (*port == NULL){
            free(aux);
            free(*ip);
            return ERROR_MALLOC;
        }

        if ( sprintf(*port,"%s",token) < 0){
            free(aux);
            free(*ip);
            free(*port);
            return ERROR_MALLOC;
        }
    }
    free(aux);

    return 0;
}

/*
    funcao que limpa a estrtura, manda remove ao servidor de raizes e limpa a lista de bestpops
*/
void limpa_parametros_entrada(parametros_entrada *entry)
{
    int i = 0;

    if(entry->iamroot){
        send_remove(entry);
    }

    close_udp_socket(&entry->servidor_de_raizes.fd_udp, &entry->servidor_de_raizes.res);
    close(entry->fonte.fd_tcp);
    close(entry->meu_endereco.fd_tcp);

    free(entry->streamID);
    free(entry->fonte.ip);
    free(entry->fonte.porto_tcp);
    free(entry->meu_endereco.ip);
    free(entry->meu_endereco.porto_tcp);
    free(entry->meu_endereco.porto_udp);
    free(entry->servidor_de_raizes.ip);
    free(entry->servidor_de_raizes.porto_udp);
    free(entry->servidor_de_acesso.ip);
    free(entry->servidor_de_acesso.porto_udp);

    limpa_lista_bestpops(entry->points_of_presence_of_tree_head);

    for (i = 0; i < entry->tcp_sessions+1;i++)
    {
        free(entry->filhos[i].ip);
        free(entry->filhos[i].porto_tcp);
        if (entry->filhos[i].being_used){
            close(entry->filhos[i].fd_tcp);
        }
    }
    free(entry->filhos);
}
/*
    limpa lista de bestpops
*/
void limpa_lista_bestpops(ponto_presenca *head)
{
    ponto_presenca *aux = head;
    ponto_presenca *aux1 = NULL;

    while( aux != NULL )
    {
        aux1 = aux;
        aux = aux->next;
        free(aux1->ip);
        free(aux1->porto_tcp);
        free(aux1);
    }
}
/*
    funcao que inicializa a lista de bestpops
*/
void init_points_of_presence_of_tree(parametros_entrada *entry)
{
    ////// no caso de ser chamada dentro do re_inite ja tiver ligacoes a jusante estabelecidas
    if ( (entry->tcp_sessions - entry->meu_filhos_ocupados) > 0){

        adiciona_point_of_presence(entry,&entry->points_of_presence_of_tree_head,entry->meu_endereco.ip, entry->meu_endereco.porto_tcp, entry->tcp_sessions - entry->meu_filhos_ocupados);
    }

    pop_query_needed(entry);
}
/*
    funcao que adiciona um ponto à lista
*/
void adiciona_point_of_presence(parametros_entrada *entry, ponto_presenca **head,char ip[], char porto_tcp[], int avail)
{
    ponto_presenca *aux = NULL, *new = NULL;


    if (*head == NULL) /// se for o primeiro
    {
        new = (ponto_presenca *)malloc(sizeof(ponto_presenca));
        if (new == NULL){
            perror("error malloc: ");
            limpa_parametros_entrada(entry);
            exit(1);
        }

        new->ip = NULL;
        new->porto_tcp = NULL;
        new->ip = (char *)malloc(sizeof(char) * ( strlen(ip) +1 ) );
        if ( new->ip == NULL){
            limpa_parametros_entrada(entry);
            exit(1);
        }
        strcpy(new->ip,ip);
        new->porto_tcp = (char *)malloc(sizeof(char) * ( strlen(porto_tcp) + 1) );
        if ( new->porto_tcp == NULL){
            limpa_parametros_entrada(entry);
            exit(1);
        }
        strcpy(new->porto_tcp,porto_tcp);
        new->avail = avail;
        new->next = NULL;
        *head = new;
        new = NULL;

    }
    else /// se a lista ja tiver elementos
    {

        aux = *head;
        /////// se o ponto ja existir basta actualizar o numero de avail
        if (strcmp(aux->ip,ip) == 0 && strcmp(aux->porto_tcp,porto_tcp) == 0){
            aux->avail = avail;
            return;
        }

        while (aux->next != NULL)
        {
            if (strcmp(aux->next->ip, ip) == 0 && strcmp(aux->next->porto_tcp,porto_tcp) == 0){
                aux->next->avail = avail;
                return;
            }
            aux = aux->next;
        }

        new = (ponto_presenca *)malloc(sizeof(ponto_presenca));
        if (new == NULL){
            limpa_parametros_entrada(entry);
            exit(1);
        }

        new->ip = NULL;
        new->porto_tcp = NULL;

        new->ip = (char *)malloc(sizeof(char) * (strlen(ip) + 1) );
        if ( new->ip == NULL){
            perror("error malloc: ");
            limpa_parametros_entrada(entry);
            exit(1);
        }
        strcpy(new->ip,ip);

        new->porto_tcp = (char *)malloc(sizeof(char) * (strlen(porto_tcp)+1) );
        if ( new->porto_tcp == NULL){
            limpa_parametros_entrada(entry);
            exit(1);
        }
        strcpy(new->porto_tcp,porto_tcp);

        new->avail = avail;
        new->next = NULL;

        aux->next = new;
        new = NULL;
    }

}
/*
    incrementa o queryID
*/
void incrementa(int *ID)
{
    if (*ID +1 == 65536)
    {
        *ID = 0;
    }
    else
    {
        (*ID)++;
    }

}


/*
    imprimir argumentos de entrada
*/
void imprime_argumentos_de_entrada(parametros_entrada entry)
{
    printf("my streamID is %s\n",entry.streamID);
    printf("my fonte_ip is %s\n",entry.fonte.ip);
    printf("my fonte_port is %s\n",entry.fonte.porto_tcp);
    printf("my ip is %s\n",entry.meu_endereco.ip);
    printf("my tcp_port is %s\n",entry.meu_endereco.porto_tcp);
    printf("my udp_port is %s\n",entry.meu_endereco.porto_udp);
    printf("o servidor de raizes onde me vou ligar é %s\n",entry.servidor_de_raizes.ip);
    printf("o servidor de raizes onde me vou ligar é %s\n",entry.servidor_de_raizes.porto_udp);
    printf("o servidor de acesso onde me vou ligar é %s\n",entry.servidor_de_acesso.ip);
    printf("o servidor de acesso onde me vou ligar é %s\n",entry.servidor_de_acesso.porto_udp);
    printf("my tcp_sessions is %d\n",entry.tcp_sessions);
    printf("my bestpops is %d\n",entry.bestpops);
    printf("my tempo_reg_per is %d\n",entry.tsecs);
    printf("b : %i\n",entry.b);
    printf("d : %i\n",entry.d);
}
/*
    funcao que le o que o utilizador escreveu na linha de parametros e modifica ou chamada a respetiva variavel ou funcao
*/
void ler_informacao_linha_comandos(parametros_entrada *entry)
{
    char msg[500]= {0};
    ssize_t aux = 0;
    int i = 0;
    fgets(msg,50,stdin);
    if(entry->d){printf("Mensagem de Debug:Lido do teclado é : %s\n",msg);}

    if (strcmp(msg,streams) == 0){ //// se escreveu streams
        aux = send_dump(entry->servidor_de_raizes.fd_udp, entry->servidor_de_raizes.res);
        if ( aux == TIMEOUT_UDP || aux == -1 ){
            limpa_parametros_entrada(entry);
            exit(0);
        }
    }
    else if (strcmp(msg,status) == 0){ //// se escreveu status
        imprime_status(entry);
    }
    else if (strcmp(msg,display_on) == 0){ //// se escreveu display on
        entry->b = true;
    }
    else if (strcmp(msg,display_off) == 0){ //// se escreveu display off
        entry->b = false;
    }
    else if (strcmp(msg,debug_on) == 0){ //// se escreveu debug on
        entry->d = true;
    }
    else if (strcmp(msg,debug_off) == 0){ //// se escreveu debug off
        entry->d = false;
    }
    else if (strcmp(msg,EXIT) == 0){ //// se escreveu exit
        entry->will_to_exit = true;
    }
    else if (strcmp(msg,TREE) == 0){ /// se escreveu tree, este comando so pode ser feito pela raiz
        if(entry->iamroot){
            ///////////escreve o streamID na linha de comandos
            write(1,entry->streamID,strlen(entry->streamID));

            if (entry->meu_filhos_ocupados > 0){////// se tiver filhos tem de enviar tree_query para eles
                if ( sprintf(msg," %s:%s (%d ",entry->meu_endereco.ip,entry->meu_endereco.porto_tcp, entry->tcp_sessions) < 0 ){
                    limpa_parametros_entrada(entry);
                    exit(0);
                }
                for( i = 0; i < entry->tcp_sessions; i++) ///// vai buscar o ipe porto dos filhos
                {
                    if(entry->filhos[i].being_used){
                        send_tree_query(entry,entry->filhos[i].ip,entry->filhos[i].porto_tcp,i);
                        strcat(msg,entry->filhos[i].ip);
                        strcat(msg,":");
                        strcat(msg,entry->filhos[i].porto_tcp);
                        strcat(msg," ");
                    }
                }
                strcat(msg,")\n");
                write(1,msg,strlen(msg));///// imprime a linha

            }
            else{ ////////// se nao tiver filhos imprime o seu ip e porto_tcp e o numero de pontos de acesso que tem
                if ( sprintf(msg," %s:%s (%d)\n",entry->meu_endereco.ip,entry->meu_endereco.porto_tcp, entry->tcp_sessions) < 0 ){
                    limpa_parametros_entrada(entry);
                    exit(0);
                }
                write(1,msg,strlen(msg));
            }

        }
        else /// caso nao seja raiz recebe a seguinte mensagem no ecra
        {
            printf("you are not root\n");
        }
    }
    else if (strcmp(msg,format_ascii) == 0){ /// se quer ver a data em formato ascii
        entry->f_ascii = true;
    }
    else if (strcmp(msg,format_hex) == 0){ /// se quer ver a data em formato hex
        entry->f_ascii = false;
    }
    else{
        printf("COMANDO INVÁLIDO!\n");
    }
}
/*
    imprime varias variaveis indicadas no enunciado, esta funcao é chamada quando o utilizador escreve status na linha de comandos
*/
void imprime_status(parametros_entrada *entry)
{
    int i = 0;
    printf("streamID:%s\n",entry->streamID);//// imprime streamID

    if (entry->stream_flowing){//// imprime se SF ou BS
        printf("Stream Flowing\n");
    }

    if (!entry->stream_flowing){
        printf("Broken Stream\n");
    }

    if (entry->iamroot){ //// indica se for root ou nao e imprime o servidor de acesso ou o endereco do nó a montante
        printf("I am root\n");
        printf("Servidor de acesso\nip:%s\nport:%s\n",entry->meu_endereco.ip,entry->meu_endereco.porto_udp);
    }

    if (!entry->iamroot){
        printf("I am not root\n");
        printf("Meu par a montante\nip:%s\nport:%s\n",entry->fonte.ip,entry->fonte.porto_tcp);
    }
    /*  varias indicações sobre o numero de tcp sessions que tem, aquelas que ainda estao livres
        e o endereco ip:porto da ligacoes a jusante, se houver*/
    printf("Meu endereço\nip:%s\ntcp port:%s\n",entry->meu_endereco.ip,entry->meu_endereco.porto_tcp);
    printf("Numero de sessoes suportadas: %d\n",entry->tcp_sessions);
    printf("Numero de sessões ocupadas: %d\n",entry->meu_filhos_ocupados);

    if (entry->meu_filhos_ocupados > 0){
        for ( i = 0;i < entry->tcp_sessions; i++){
            if (entry->filhos[i].being_used){
                printf("Meu par a jusante\nip:%s\nport:%s\n",entry->filhos[i].ip,entry->filhos[i].porto_tcp);
            }
        }
    }

}
/*
    funcao que liga-se a fonte e inicializa os proprios servidor, as flag sao para o caso de alguma coisa falhar e funcao chamar-se a si propria
    as flags representam o numero de tentativas que tem antes de sair do programa
*/
void mecanismo_de_init(parametros_entrada *entry, int flag_u, int flag_tcp)
{
    send_whoisroot(entry);
    ////// as flags significam o numero de tentativas
    if (flag_u == 3){
        printf("Tentei varias vezes, mas não dá para ligar ao servidor de acesso\nI am exiting!\n");
        limpa_parametros_entrada(entry);
        exit(1);
    }

    if (flag_tcp == 3){
        printf("Tentei varias vezes, mas não dá para ligar à fonte\nI am exiting!\n");
        limpa_parametros_entrada(entry);
        exit(1);
    }
    ///// o procedimento para root e nao-roots é diferente
    if (entry->iamroot)
    {
        if(entry->d){printf("Mensagem de debug: I am root\n");}
        ligar_a_fonte(entry);//// se der erro a funcao faz exit
        abrir_servidor_de_acesso(entry);
        ser_fonte(entry);
    }
    else
    {
        if(entry->d){printf("Mensagem de debug: I am not root\n");}
        if ( send_POPREQ(entry) == TIMEOUT_UDP ){
            printf("Servidor de acesso parece estar offline. Vou tentar mais uma vez\n");
            freeaddrinfo(entry->servidor_de_acesso.res);
            mecanismo_de_init(entry, flag_u + 1, flag_tcp);
            return;
        }
        if ( ligar_a_fonte(entry) == ERROR_TCP ){
            printf("Fonte parece estar offline. Vou tentar mais uma vez\n");
            mecanismo_de_init(entry, flag_u, flag_tcp + 1);
            return;
        }
        ser_fonte(entry);
        decifra_info_fonte(entry);
    }


    if(entry->d){
        printf("Mensagem de debug:Inicio\n");
        imprime_argumentos_de_entrada(*entry);
        printf("Mensagem de debug:Fim\n");
    }
}
/*
    parecida para à funcao de mecanismo_de_init, mas para o caso de a ligacao a montante ter ido abaixo,
    a pricipal diferenca é que nao chama a funcao ser fonte
*/
void mecanismo_de_re_init(parametros_entrada *entry, int flag_u, int flag_tcp)
{

    send_whoisroot(entry);
    if (flag_u == 3){
        printf("Tentei varias vezes, mas não dá para re-ligar ao servidor de acesso\nI am exiting!\n");
        limpa_parametros_entrada(entry);
        exit(1);
    }

    if (flag_tcp == 3){
        printf("Tentei varias vezes, mas não dá para ligar à fonte\nI am exiting!\n");
        limpa_parametros_entrada(entry);
        exit(1);
    }

    if (entry->iamroot){
        if(entry->d){printf("Mensagem de Debug:I am root\n");}
        ligar_a_fonte(entry);
        abrir_servidor_de_acesso(entry);
        if ( timerfd_settime(entry->pop_query_fd,0,&entry->bestpops_timer2,( struct itimerspec *)NULL) == -1){
            perror("error pop_query_timerfd_settime:");
            limpa_parametros_entrada(entry);
            exit(0);
        }
        if ( timerfd_settime(entry->root_renew_fd,0,&entry->root_renew_timer2,( struct itimerspec *)NULL) == -1){
            perror("error root_renew_timerfd_settime:");
            limpa_parametros_entrada(entry);
            exit(0);
        }
    }
    else{
        if(entry->d){ printf("Mensagem de Debug:I am not root\n");}
        if ( send_POPREQ(entry) == TIMEOUT_UDP){
            printf("Servidor de acesso parece estar offline. Vou tentar mais uma vez\n");
            freeaddrinfo(entry->servidor_de_acesso.res);
            mecanismo_de_re_init(entry , flag_u +1, flag_tcp);
            return;
        }
        if ( ligar_a_fonte(entry) == ERROR_TCP){
            printf("Fonte parece estar offline. Vou tentar mais uma vez\n");
            mecanismo_de_re_init(entry,flag_u,flag_tcp + 1);
            return;
        }
    }
    decifra_info_fonte(entry);

}
/*
    imprime data que a fonte enviou, tem a opcao de imprimir em ascii ou hex
*/
void imprime_data(parametros_entrada *entry, char data[], bool flag)
{
    char *data_hex = NULL;
    int i = 0;
    if (entry->b){
        if (entry->f_ascii){/////imprime ascii
            printf("%s",data);
        }
        else{///////////imprime hex

            data_hex = (char *)calloc( (strlen(data)*2 + 1) ,sizeof(char) );
            if (data_hex == NULL){
                perror("malloc");
                if(flag){free(data);}
                limpa_parametros_entrada(entry);
                exit(0);
            }
            for(i = 0; i < strlen(data) ; i++){///////////converte para hex
                if ( sprintf(data_hex + i*2, "%02X", data[i]) < 0){
                    if(flag){free(data);}
                    printf("erro do sprintf\n");
                    free(data_hex);
                    limpa_parametros_entrada(entry);
                    exit(0);
                }
            }
            printf("%s\n",data_hex);
            free(data_hex);
        }
    }
}
