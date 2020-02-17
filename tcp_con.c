#include "tcp_con.h"
/*
    ligar ao servidor tcp do qual sou cliente
*/

int ligar_a_fonte(parametros_entrada *entry)
{
    int n = 0;
    struct addrinfo hints, *res;
    int fd = 0;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags=AI_NUMERICHOST|AI_NUMERICSERV;

    n = getaddrinfo(entry->fonte.ip,entry->fonte.porto_tcp, &hints, &res);
    if (n != 0 )
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(n));
        limpa_parametros_entrada(entry);
        exit(1);
    }

    fd = socket(res-> ai_family, res->ai_socktype, res->ai_protocol);
    if (fd == -1 )
    {
        perror("Error2: ");
        freeaddrinfo(res);
        limpa_parametros_entrada(entry);
        exit(1);
    }
    n = connect(fd, res->ai_addr, res->ai_addrlen);
    if (n == -1 )
    {
        if (entry->iamroot){
            perror("Error3_ligar_fonte: ");
            freeaddrinfo(res);
            close(entry->fonte.fd_tcp);
            limpa_parametros_entrada(entry);
            exit(1);
        }
        else{
            close(entry->fonte.fd_tcp);
            freeaddrinfo(res);
            return ERROR_TCP;
        }
    }
    entry->fonte.fd_tcp = fd;

    freeaddrinfo(res);
    return 0;
}
/*
    ser servidor tcp para que outras iamroot se possam ligar a mim
*/
void ser_fonte(parametros_entrada *entry)
{
    int n = 0;
    struct addrinfo hints, *res;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE|AI_NUMERICSERV;

    n = getaddrinfo(NULL, entry->meu_endereco.porto_tcp, &hints, &res);
    if(n != 0){
		perror("ERROR1: ");
        limpa_parametros_entrada(entry);
		exit(1);
	}

    entry->meu_endereco.fd_tcp = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if(entry->meu_endereco.fd_tcp == -1) {
		perror("ERROR2: ");
        limpa_parametros_entrada(entry);
		exit(1);
	}

    n = bind(entry->meu_endereco.fd_tcp, res->ai_addr, res->ai_addrlen);
    if(n == -1){
		perror("ERROR3_ser_fonte: ");
        limpa_parametros_entrada(entry);
		exit(1);
	}

    if(listen(entry->meu_endereco.fd_tcp,5)==-1){
        perror("ERROR4: ");
        limpa_parametros_entrada(entry);
        exit(1);

    }
    freeaddrinfo(res);
}

void fazer_accept_tcp(parametros_entrada *entry)
{
    socklen_t addrlen = 0;
    ssize_t n;
    int i = 0, newfd = 0;
    bool flag = false;
    struct sockaddr_in addr = {0};
    //// faz accept
    newfd = accept(entry->meu_endereco.fd_tcp, (struct sockaddr*)&addr, &addrlen);
    if(newfd == -1){
        perror("ERRO5: ");
        exit(1);
    }

    for (i = 0;i< entry->tcp_sessions;i++)
    {
        if(!entry->filhos[i].being_used)//// ver se ha filhos livres
        {
            flag = true;
            entry->filhos[i].fd_tcp = newfd;
            entry->filhos[i].being_used = true;

            if ( send_welcome (i, entry) == ERROR_WRITE){ /// envia welcome
                if(entry->d){printf("Mensagem de Debug:Connection a jusante lost\n");}
                jusante_lost(entry,i);
                break;
            }

            if (entry->stream_flowing)
            {
                n = manda_para_um_jusante(entry,STREAM_FLOWING,i); //// envia stream flowing, se po stream estiver a fluir
                if (n == -1)
                {
                    if(entry->d){printf("Mensagem de Debug:Connection a jusante lost\n");}
                    jusante_lost(entry,i);
                    break;
                }
                if(entry->d){printf("Mensagem de Debug:Enviei %s\n",STREAM_FLOWING);}
            }

            if(entry->d){printf("Mensagem de Debug:Connection a jusante done\n");}
            entry->meu_filhos_ocupados +=1;
            break;
        }
    }
    if (!flag)//// neste caso significa que este chegou um bocadinho atrasado e vai levar com o redirect para ultimo que se conectou
    {
        send_redirect(entry,newfd);
    }
}

void decifra_info_fonte(parametros_entrada *entry)
{
    ssize_t bytes = 0;
    char code[4] = {0}, data_size[5] = {0};
    char msg[201] = {0},msg1[201] = {0};
    char *data = NULL;
    char *send = NULL;
    char *tree_ip = NULL, *tree_port = NULL;
    int i = 0, retval = 0, max_s = 0;
    fd_set readfds;
    struct  timeval timeout;

    timeout.tv_sec = 10;
    timeout.tv_usec = 0;

    bytes = le_letra_a_letra(200,entry->fonte.fd_tcp,msg,"\n");
    if (bytes == -1 || bytes == 0) ///// erro ou eof
    {
        montante_lost(entry);
        mecanismo_de_re_init(entry,0,0);
        return;
    }
    max_s = min(3,strlen(msg));//// como nao guardamos o \n se for SF ou BS sao apenas 2 carateres, para todos os outros casos sao 3
    for ( i = 0; i < max_s; i++)
    {
        code[i] = msg[i];
    }
    if (strcmp(code,WELCOME) == 0){  //// se mensagem for welcome
        if(entry->d){printf("Mensagem de Debug:Recebi %s\n",code);}
        if ( send_new_pop(entry) == -1){ ///// se houve erro com comunicao com a fonte
            /////// o reestabelecimento foi feito dentro da funcao
            ////// por isso voltamos a main
            return;
        }
        else{/////////dado que existe a possibilidade de isto ficar em loop contamos o tempo entre receber o WE e o SF
            FD_ZERO(&readfds);
            FD_SET(entry->fonte.fd_tcp, &readfds);
            retval = select(entry->fonte.fd_tcp + 1, &readfds, (fd_set*)NULL, (fd_set*)NULL, &timeout);

            if (retval == 0){ ////// se nao recebeu o SF é porque pode estar em loop
                montante_lost(entry);
                printf("existe a possibilidade de termos ficado em loop.\n");
                mecanismo_de_re_init(entry,0,0);
                return;
            }
            else if( FD_ISSET(entry->fonte.fd_tcp, &readfds) ){
                bytes = recebe_mensagem_tcp(entry->fonte.fd_tcp,code,3);
                if (bytes == -1 || bytes == 0) ///// erro ou eof
                {
                    montante_lost(entry);
                    mecanismo_de_re_init(entry,0,0);
                    return;
                }
                if (strcmp(code,STREAM_FLOWING) == 0){ //// se mensagem for stream flowing
                    entry->stream_flowing = true;
                    if(entry->d){printf("Mensagem de Debug:Recebi %s",code);}
                    return;
                }
                else{////////////////// recebemos lixo
                    montante_lost(entry);
                    mecanismo_de_re_init(entry,0,0);
                    return;
                }

            }
            else{
                printf("Problema com o select");
                limpa_parametros_entrada(entry);
                exit(1);
            }

        }

    }
    else if ( strcmp(code,DATA) == 0){ //// se mensagem for data
        if(entry->d){printf("Mensagem de Debug:recebi %s\n",code);}
        /////////ler o tamanho da data
        for (i = 0; i < 4 ; i++){
            data_size[i] = msg[i+3];
        }
        i = (int)strtol(data_size, NULL, 16);

        /////////alocaçao de memoria
        bytes = 0;
        data = (char *)calloc((i+1), sizeof(char) );
        if ( data == NULL){
            perror("malloc:");
            limpa_parametros_entrada(entry);
            exit(0);
        }

        ////////// copia a data do socket para o vector data
        //////////// porque la em cima so leu ate ao primeiro \n
        //////////// quando falamos de receber data existem dois \n separados
        bytes = le_letra_a_letra(i,entry->fonte.fd_tcp,data,"\n");
        if (bytes == -1 || bytes == 0) ///// erro ou eof
        {
            free(data);
            montante_lost(entry);
            mecanismo_de_re_init(entry,0,0);
            return;
        }
        ///////////////imprimir para o ecra(ascii/hex) ou nao
        imprime_data(entry,data, true);

        /////////////////manda para jusante
        send = (char *)calloc( (strlen(data) + 11), sizeof(char) );
        if ( send == NULL){
            perror("malloc:");
            free(data);
            limpa_parametros_entrada(entry);
            exit(0);
        }
        if (sprintf(send,"%s%04x\n%s\n",DATA,i,data) < 0){
            free(send);
            free(data);
            limpa_parametros_entrada(entry);
            exit(0);
        }
        free(data);
        manda_para_todos_jusante(entry,send,0);
        free(send);

    }
    else if (strcmp(code,"SF") == 0){ //// se mensagem for stream flowing
        entry->stream_flowing = true;

        if(entry->d){printf("Mensagem de Debug:recebi %s\n",code);}

        if (entry->meu_filhos_ocupados > 0) {
            manda_para_todos_jusante(entry,STREAM_FLOWING,0);
        }
    }
    else if (strcmp(code,"BS") == 0){ //// se mensagem for broken stream
        entry->stream_flowing = false;

        if(entry->d){printf("Mensagem de Debug:recebi %s\n",code);}

        if (entry->meu_filhos_ocupados > 0) {
            manda_para_todos_jusante(entry,BROKEN_STREAM,0);
        }
    }
    else if (strcmp(code,REDIRECT) == 0){ ////// se mensagem for redirect

        if(entry->d){printf("Mensagem de Debug:recebi %s\n",code);}

        close(entry->fonte.fd_tcp);
        free(entry->fonte.ip);
        free(entry->fonte.porto_tcp);
        entry->fonte.ip = NULL;
        entry->fonte.porto_tcp = NULL;

        ////// a memoria alocada para ip e porto é feita aqui dentro
        if ( decifra_ip_porto(&entry->fonte.ip, &entry->fonte.porto_tcp, msg+3) == ERROR_MALLOC ) {
            limpa_parametros_entrada(entry);
        }
        if(entry->d){printf("Mensagem de Debug:Nova fonte ip:%s\n",entry->fonte.ip);}
        if(entry->d){printf("Mensagem de Debug:Nova fonte porto:%s\n",entry->fonte.porto_tcp);}
        if ( ligar_a_fonte(entry) == ERROR_TCP){
            mecanismo_de_init(entry,0,0);
            return;
        }
    }
    else if(strcmp(code,POP_QUERY) == 0){ ////// se mensagem for pop_query
        if(entry->d){printf("Mensagem de Debug:recebi %s\n",code);}
        strcat(msg,"\n");
        recebi_pop_query(entry,msg);
    }
    else if (strcmp(code,TREE_QUERY) == 0){ /////////////// se a mensagem for um tree_query
        if(entry->d){printf("Mensagem de Debug:recebi %s\n",code);}

        /////////////// tirar o ip e o porto do msg
        if (decifra_ip_porto(&tree_ip,&tree_port,msg+3) == ERROR_MALLOC){
            limpa_parametros_entrada(entry);
            exit(1);
        }
        //////////////// comparar com o meu endereço
        if (strcmp(tree_ip,entry->meu_endereco.ip) == 0 && strcmp(tree_port,entry->meu_endereco.porto_tcp) == 0){
            /////////////// se for o meu mandar tree reply
            send_tree_reply(entry);
        }
         ////////// ver se o id e porto é de algum filho meu, se for mandar a mensagem so para ele
        else if (entry->meu_filhos_ocupados > 0)
        {
            for (i = 0; i < entry->tcp_sessions; i++)
            {
                if (entry->filhos[i].being_used && strcmp(tree_ip,entry->filhos[i].ip) == 0 && strcmp(tree_port,entry->filhos[i].porto_tcp) == 0)
                {
                    send_tree_query(entry,tree_ip,tree_port,i);
                    free(tree_ip);
                    free(tree_port);
                    return;
                }

            }
            if (sprintf(msg1,"%s%s:%s\n",TREE_QUERY,tree_ip,tree_port) < 0){
                limpa_parametros_entrada(entry);
                exit(0);
            }
            manda_para_todos_jusante(entry,msg1,0);
        }
        free(tree_ip);
        free(tree_port);
        return;
    }
    else{ /////////////////////////////////   se for eu o root e tiver que fazer o data
        if (entry->iamroot){
            if(!entry->stream_flowing){
                entry->stream_flowing= true;
                manda_para_todos_jusante(entry,STREAM_FLOWING,0);
            }

            imprime_data(entry,msg,false);
            data = (char *)malloc(sizeof(char) * (strlen(msg) +11 ) );
            if ( data == NULL){
                perror("malloc:");
                limpa_parametros_entrada(entry);
                exit(0);
            }
            ///// porque as cenas que vem da fonte nao cumprem o protocolo definido no enunciado falta o data
            if (sprintf(data,"%s%04x\n%s",DATA, (int)strlen(msg),msg) < 0){
                free(data);
                limpa_parametros_entrada(entry);
                exit(0);
            }

            manda_para_todos_jusante(entry,data,0);
            free(data);
            data = NULL;

        }
    }
}
/*
    controi e envia a mensagemd de welcome
*/
int send_welcome(int local, parametros_entrada *entry)
{
    ssize_t n = 0;
    char buffer[200] = {0};

    if (sprintf(buffer,"%s%s\n",WELCOME,entry->streamID) < 0){
        limpa_parametros_entrada(entry);
        exit(0);
    }

    if(entry->d){printf("Mensagem de Debug:%s\n",buffer);}

    n = manda_para_um_jusante(entry,buffer,local);
    if (n == -1){
        return ERROR_WRITE;
    }

    return n;
}
/*
    controi e envia a mensagem redirect
*/
void send_redirect(parametros_entrada *entry, int fd)
{
    ssize_t n = 0, n_bytes_sum = 0;
    int i = random() % entry->tcp_sessions;
    char msg[250] = {0};
    int tam = 0;
    char *ip = NULL, *porto = NULL;

    ////// este é para o caso de a variavel la em cima na funcao que chama esta estiver atualizada, mas nos ainda nao lemos o
    ////// o new_pop quer isto dizer que o being used esta positivo, mas como ainda nao recebemos o newpop o ponteiro do ip
    ////// e porto do filho estao a null, ora para este caso, enviamos de novo o nosso ip e porto da
    ///// podiamos ter enviado o ip:porto de outro filho, mas e se apenas aceitarmos uma sessao tcp
    ///// desta forma, achamos esta ser a solucao mais geral, para o problema de recerbermos uma conecao entre aceitarmos uma ligacao
    //// tcp e recebermos o new_pop do respetivo iamroot

    if(entry->filhos[i].ip != NULL && entry->filhos[i].porto_tcp != NULL )
    {
        ip = entry->filhos[i].ip;
        porto = entry->filhos[i].porto_tcp;
    }
    else
    {
        ip = entry->meu_endereco.ip;
        porto = entry->meu_endereco.porto_tcp;
    }

    if (sprintf(msg,"%s%s:%s\n",REDIRECT,ip,porto) < 0){
        close(fd);
        limpa_parametros_entrada(entry);
        exit(0);
    }
    tam = strlen(msg);

    if(entry->d){printf("Mensagem de Debug:%s\n",msg);}
    /////////aqui nao da para usar a funcao manda para um jusante porque este fd nao esta no vetor do filhos do entry
    while (n_bytes_sum != tam)
    {
        n = write(fd, msg + n_bytes_sum, tam -n_bytes_sum );
        if (n == -1)
        {
            close(fd);
            return ;
        }
        n_bytes_sum += n;
    }
    close(fd);
    ip = NULL;
    porto = NULL;
}
/*
    funcao que recebe um mensagem tcp de um fd com um tamanho tam e guarda no vector buffer
*/
ssize_t recebe_mensagem_tcp(int fd, char buffer[],ssize_t tam)
{

    ssize_t n = 0, n_bytes_sum = 0;

    while (n_bytes_sum != tam)
    {
        n = read(fd, buffer + n_bytes_sum , tam - n_bytes_sum);
        if (n == -1 || n == 0){ ///// erro ou eof
            return n;
        }
        n_bytes_sum += n;
    }
    return n;

}

////// envia tree query onde cada filho recebe o seu proprio tree query
void send_tree_query(parametros_entrada *entry, char tree_ip[],char tree_port[], int local)
{
    char msg[200] = {0};
    ssize_t bytes = 0;

    if (sprintf(msg,"%s%s:%s\n",TREE_QUERY,tree_ip,tree_port) < 0){
        limpa_parametros_entrada(entry);
        exit(0);
    }
    bytes = manda_para_um_jusante(entry,msg,local);
    if (bytes == -1 )
    {
        if(entry->d) {printf("Connection a jusante lost\n");}
        jusante_lost(entry,local);
    }
    return;

}
/*
    controi e envia tree reply
*/
void send_tree_reply(parametros_entrada *entry)
{
    char *msg = NULL;
    int tam = 0, i = 0;
    ssize_t bytes = 0;

    tam = strlen(TREE_REPLY) + strlen(entry->meu_endereco.ip) + strlen(entry->meu_endereco.porto_tcp) + 15;/// 3 do " " : e \n mais 12 (para o int)

    if (entry->meu_filhos_ocupados > 0)
    {
        for(i = 0; i < entry->tcp_sessions; i++)
        {
            if (entry->filhos[i].being_used)
            {
                tam = tam + strlen(entry->filhos[i].ip) + strlen(entry->filhos[i].porto_tcp) + 2;///devido ao : e \n
            }
        }
    }

    msg = (char *)malloc(sizeof(char) * (tam +1) ); ///// para o \n final
    if (msg == NULL)
    {
        limpa_parametros_entrada(entry);
        exit(0);
    }
    if (sprintf(msg,"%s%s:%s %d\n",TREE_REPLY,entry->meu_endereco.ip,entry->meu_endereco.porto_tcp,entry->tcp_sessions) < 0){
        free(msg);
        limpa_parametros_entrada(entry);
        exit(0);
    }
    if (entry->meu_filhos_ocupados > 0)
    {
        for(i = 0; i < entry->tcp_sessions; i++)
        {
            if (entry->filhos[i].being_used)
            {
                strcat(msg,entry->filhos[i].ip);
                strcat(msg,":");
                strcat(msg,entry->filhos[i].porto_tcp);
                strcat(msg,"\n");
            }
        }
    }
    strcat(msg,"\n");

    if(entry->d){printf("Mensagem de Debug:%s\n",msg);}

    bytes = mensagem_para_montante(entry,msg);

    if(entry->d){printf("Mensagem de Debug:Foram enviados %d bytes\n",(int)bytes);}

    free(msg);

}
/*
    funcao que manda apenas para um jusante, e trata do procedimento se ele se desconectar
*/
ssize_t manda_para_um_jusante(parametros_entrada *entry, char buffer[], int local)
{

    ssize_t n_bytes_sum = 0,n_bytes = 0;
    int tam = strlen(buffer);

    while (n_bytes_sum != tam )
    {
        n_bytes = write(entry->filhos[local].fd_tcp, buffer + n_bytes_sum, tam - n_bytes_sum);
        if (n_bytes == -1 )
        {
            return ERROR_WRITE;
        }
        n_bytes_sum += n_bytes;
    }
    return n_bytes_sum;

}
/*
    funcao que envia informacao para todos os jusante ativos, e trata do procedimento de um deles se desconectar
    o maneira é a maneira como ele percorre a lista
    0 significa que vai do 0 ate ao tcp_sessions
    1 significa que vai do tcp_sessions para 0
*/
void manda_para_todos_jusante(parametros_entrada *entry, char buffer[], int maneira)
{
    int i = 0;
    ssize_t n_bytes = 0;

    if (maneira == 0)
    {
        for (i = 0;i< entry->tcp_sessions;i++)
        {
            if(entry->filhos[i].being_used)
            {
                n_bytes = manda_para_um_jusante(entry,buffer,i);

                if (n_bytes == -1 )
                {
                    if(entry->d) {printf("Connection a jusante lost\n");}
                    jusante_lost(entry,i);
                    break;
                }

            }
        }
    }
    else
    {
      for (i = entry->tcp_sessions - 1 ;i >= 0 ;i--)
      {
          if(entry->filhos[i].being_used)
          {
              n_bytes = manda_para_um_jusante(entry,buffer,i);

              if (n_bytes == -1 )
              {
                  if(entry->d) {printf("Connection a jusante lost\n");}
                  jusante_lost(entry,i);
                  break;
              }

          }
      }
    }


}
/*
    funcao que envia uma mensagem para montante, e trata de restabelecer a ligacao se montante for a baixo
*/
ssize_t mensagem_para_montante(parametros_entrada *entry, char msg[])
{
    ssize_t n = 0,n_sum = 0;
    int tam = strlen(msg);

    while(n_sum != tam)
    {
        n = write(entry->fonte.fd_tcp, msg + n_sum, tam - n_sum);
        if (n == -1 || n == 0 )
        {
            montante_lost(entry);
            mecanismo_de_re_init(entry,0,0);
            return 0;
        }
        n_sum += n;
    }
    return n_sum;

}
/*
    funcao que controi e envia um new pop
*/
ssize_t send_new_pop(parametros_entrada *entry)
{
    char msg[200] = {0};
    ssize_t n = 0;
    if ( sprintf(msg,"%s%s:%s\n",NEW_POP,entry->meu_endereco.ip,entry->meu_endereco.porto_tcp) < 0){
        limpa_parametros_entrada(entry);
        exit(0);
    }

    if(entry->d){printf("Mensagem de Debug:%s\n",msg);}
    n = mensagem_para_montante(entry,msg);
    return n;


}
/*
    funcao que le de um fd carater carater, ate ou encontrar um carater final ou atingir o maximo
*/
ssize_t le_letra_a_letra(int max,int fd, char msg[], char final[])
{
    int i = 0;
    ssize_t bytes_sum = 0, bytes = 0;
    char aux[2] = {0};

    for(i =0; i< max ;i++){
        msg[i] = 0;
    }
    i = 0;

    while (i < max)
    {
        bytes = recebe_mensagem_tcp(fd,aux,1);
        if (bytes == -1 || bytes == 0) ///// erro ou eof
        {
            return bytes;
        }
        if ( aux[0] != final[0] ){ ///// chegamos ao fim ou nao
            msg[i] = aux[0];
            i++;
            bytes_sum += bytes;
        }
        else{
            bytes_sum += 1;
            break;
        }
    }
    return bytes_sum;
}
/*
    funcao que recebe e decifra todas as informações provenientes de jusante, NP, TR, PR
*/
void decifra_info_jusante(parametros_entrada *entry,int local)
{
    char code[4] = {0}, buffer[100] = {0}, aux[3000] = {0};
    int i = 0;
    ssize_t bytes = 0;
    char *tree_ip = NULL, *tree_port = NULL;
    ////// a funcao tira o \n do socket, mas o nao guarda no vector

    if ( le_letra_a_letra(100,entry->filhos[local].fd_tcp,buffer,"\n") <= 0){
        if(entry->d) {printf("Mensagem de Debug:Connection a jusante lost\n");}
        jusante_lost(entry,local);
    }

    for (i = 0; i < 3 ; i++)
    {
        code[i] = buffer[i];
    }

    if ( strcmp(code,NEW_POP) == 0 ){ //////// se a mensagem for um new_pop
        if(entry->d){printf("Mensagem de Debug:Recebi %s\n",code);}

        if ( decifra_ip_porto(&entry->filhos[local].ip, &entry->filhos[local].porto_tcp, buffer+3) ==  ERROR_MALLOC){
            perror("Malloc : ");
            limpa_parametros_entrada(entry);
            exit(0);
        }
    }

    else if ( strcmp(code,POP_REPLY) == 0 ){ ///////////// se a mensagem for um pop_reply
        if(entry->d){printf("Mensagem de Debug:Recebi %s\n",code);}
        strcat(buffer,"\n");
        recebi_pop_reply(entry,buffer);

    }

    else if ( strcmp(code,TREE_REPLY) == 0 && entry->iamroot){ //// se a mensagem for um tree reply e eu for a raiz

        if(entry->d){printf("Mensagem de Debug:Recebi %s\n",code);}

        printf("Resposta ao comando tree :%s\n",buffer+3);


        /////// ja lemos o \n antes, agora temos de ler os varios ip:port dos filhos e a funcao
        ////// a funcao de ler letra a letra tira o \n, mas esta mensagem acaba com duplo \n
        ////// e é por isso que temos um while ate termos bytes == 1
        while (true)
        {
            bytes = le_letra_a_letra(100,entry->filhos[local].fd_tcp,buffer,"\n");
            if ( bytes <= 0){
                if(entry->d) {printf("Mensagem de Debug:Connection a jusante lost\n");}
                jusante_lost(entry,local);
            }

            if (bytes == 1){break;}

            printf("Resposta ao comando tree :%s\n",buffer);

            if ( decifra_ip_porto(&tree_ip,&tree_port,buffer) == ERROR_MALLOC){
                perror("Malloc : ");
                limpa_parametros_entrada(entry);
                exit(0);
            }
            send_tree_query(entry,tree_ip,tree_port,local);
            free(tree_ip);
            free(tree_port);
        }
    }
    else if ( strcmp(code,TREE_REPLY) == 0 && !entry->iamroot)
    {
        if(entry->d){printf("Mensagem de Debug:Recebi %s\n",code);}
        //// mensagem vem a primeira linha completa sem \n
        strcpy(aux,buffer);
        strcat(aux,"\n");
        ///// enquanto nao aparecer o ultimo \n vai lendo
        while (true)
        {
            bytes = le_letra_a_letra(100,entry->filhos[local].fd_tcp,buffer,"\n");
            if ( bytes <= 0){
                if(entry->d) {printf("Mensagem de Debug:Connection a jusante lost\n");}
                jusante_lost(entry,local);
            }
            if (bytes == 1){

                strcat(aux,"\n");
                break;
            }
            strcat(aux,buffer);
            strcat(aux,"\n");

        }
        bytes = mensagem_para_montante(entry,aux);
    }


}
/*
    parte do procedimento quando perde-se ligacao a montante, fechar socket, free nome, ip, e começo da tentativa de re-estabelecer ligacao
*/
void montante_lost(parametros_entrada *entry)
{
    char *aux = NULL;
    entry->stream_flowing = false;
    manda_para_todos_jusante(entry, BROKEN_STREAM,0);

    ///// limpar a memoria antiga
    close(entry->fonte.fd_tcp);
    free(entry->fonte.ip);
    free(entry->fonte.porto_tcp);

    ///// criar aux para guardar o streamID, para depois reutilizar a funcao identifica_streamID_ip_port_da_fonte
    aux = (char *)malloc(sizeof(char) * (strlen(entry->streamID)+1) );
    if (aux == NULL){
        perror("Malloc : ");
        exit(0);
    }
    strcpy(aux,entry->streamID);
    free(entry->streamID);

    entry->fonte.ip = NULL;
    entry->fonte.porto_tcp = NULL;
    entry->streamID = NULL;
    /////// saber quais sao o ip e porto da fonte
    if ( identifica_streamID_ip_port_da_fonte(&entry->streamID, &entry->fonte.ip, &entry->fonte.porto_tcp, aux) == ERROR_MALLOC){
        free(aux);
        limpa_parametros_entrada(entry);
        exit(0);
    }
    free(aux);

}
/*
    todo o procedimento quando perde-se uma ligacao a jusante
*/
void jusante_lost(parametros_entrada *entry, int local)
{
    close(entry->filhos[local].fd_tcp);
    entry->filhos[local].being_used = false;
    entry->meu_filhos_ocupados -=  1;
    if (entry->iamroot){
        adiciona_point_of_presence(entry,&entry->points_of_presence_of_tree_head,entry->meu_endereco.ip,entry->meu_endereco.porto_tcp,entry->tcp_sessions - entry->meu_filhos_ocupados);
    }
    free(entry->filhos[local].ip);
    free(entry->filhos[local].porto_tcp);
    entry->filhos[local].ip = NULL;
    entry->filhos[local].porto_tcp = NULL;
}
