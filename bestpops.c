#include "bestpops.h"
/*
    envia pop query
*/
void send_pop_query(parametros_entrada *entry, int queryID, int bestpops)
{
    char msg[25] = {0};
    int i = random() % 2;
    if ( sprintf(msg,"%s%04x %d\n",POP_QUERY,queryID,bestpops) < 0){
        limpa_parametros_entrada(entry);
        exit(0);
    }

    if(entry->d){printf("Mensagem de Debug:%s\n",msg);}

    manda_para_todos_jusante(entry,msg,i);/////// se alguma conecao com um dos jusante falhar esta funcao trata disso

}
/*
    todo o procedimento acerca da rececao de pop reply
*/
void recebi_pop_query(parametros_entrada *entry, char msg[])
{
    ///// char msg ja tem la o code "PQ "
    int bestpops_msg = 0, queryID = 0;
    char query[5] = {0};
    int livres = entry->tcp_sessions - entry->meu_filhos_ocupados;

    decifra_pop_query(msg,&bestpops_msg,query);

    queryID = (int)strtol(query, NULL, 16);

    if (livres == 0){
        ///////////guarda o registo na lista de pedidos
        regista_pedido(queryID,bestpops_msg,&entry->pedido_efetuado);
        ///////////// envia pop_query para jusante
        send_pop_query(entry,queryID,bestpops_msg);
    }

    else
    {

        if (bestpops_msg <= livres){
            ///////////////atualiza o queryID no registo do pedido efectuados
            regista_pedido(queryID,0,&entry->pedido_efetuado);
            ////////////// nao envia nada pra baixo
            ////////////// envia pop_reply com livres pra cima
            send_pop_reply(entry,queryID,entry->meu_endereco.ip, entry->meu_endereco.porto_tcp,livres);

        }
        else
        {
            //////////guarda o registo na lista de pedidos com bestpops - livres
            regista_pedido(queryID,bestpops_msg - livres,&entry->pedido_efetuado);
            /////////// envia msg com bestpops - livres para baixo
            send_pop_query(entry,queryID,bestpops_msg - livres);
            ///////////// envia pop_reply com livres(no campo avail_tcp_sessions) pra cima
            send_pop_reply(entry,queryID,entry->meu_endereco.ip, entry->meu_endereco.porto_tcp,livres);
        }
    }
}
/*
    le os varios parametros que vem no pop query
*/
void decifra_pop_query(char msg[], int *bp, char query[5])
{
    int i = 3;
    char bestpops[10] = {0};
    ////// comeca no i = 3 para igonrar o code "PQ "
    for(i = 3; i < 7;i++)
    {
        query[i-3]=msg[i];
    }
    ///// comeca no 8 por causa do code + 4 carateres do queryID + SP
    i = 8;
    while(msg[i] != '\n')
    {
        bestpops[i-8] = msg[i];
        i++;
    }

    *bp = atoi(bestpops);
}
/*
    todo o procedimento acerca da rececao de pop reply
*/
void recebi_pop_reply(parametros_entrada *entry, char msg[])
{
    ///// char msg ja tem la o code "PR " e o \n
    char query[5]={0};
    int i = 0;
    char *ip_msg = NULL, *porto_tcp_msg = NULL;
    pedido resposta = {0};
    for(i = 3; i < 7;i++)
    {
        query[i-3]=msg[i];
    }

    ///// passa de hex para dec e ao mesmo tempo de char para int
    resposta.queryID = (int)strtol(query, NULL, 16);
    decifra_pop_reply(msg,&ip_msg,&porto_tcp_msg,&resposta.bestpops);

    if ( retira_registo(entry,resposta,resposta.queryID) != 0)///// se returna 0 significa que nao preciso deste pop
    {
        if (entry->iamroot)///// guarda na lista de pops
        {
            if(entry->d){printf("Mensagem de Debug:Vou guardar o bestpop\n");}
            if(entry->d){printf("Mensagem de Debug:Ip: %s,Porto: %s\n",ip_msg,porto_tcp_msg);}
            adiciona_point_of_presence(entry,&entry->points_of_presence_of_tree_head,ip_msg, porto_tcp_msg,resposta.bestpops);
        }
        else ///// envia pra montante com o mesmo numero de avail que estava na resposta
        {
            send_pop_reply(entry,resposta.queryID,ip_msg, porto_tcp_msg,resposta.bestpops);
        }
    }
    free(ip_msg);
    free(porto_tcp_msg);

}
/*
    le os varios parametros que vem no pop reply
*/
void decifra_pop_reply(char msg_original[], char **ip, char **porto,int *avail)
{
    char code[4] = {0};
    char query[5] = {0};
    char aux[100] = {0};

    if ( sscanf(msg_original,"%s %s %s %d",code,query,aux,avail) != 4){
        printf("error sscanf\n");
    }

    decifra_ip_porto(ip, porto, aux);

}
/*
    emvia pop reply, faz a concatenação
*/
void send_pop_reply(parametros_entrada *entry, int ID, char ip[],char porto[],int avail)
{
    char msg[150] = {0};

    if ( sprintf(msg,"%s%04x %s:%s %d\n",POP_REPLY,ID,ip,porto,avail) < 0){
        limpa_parametros_entrada(entry);
        exit(0);
    }

    if(entry->d){printf("Mensagem de Debug:%s\n",msg);}
    mensagem_para_montante(entry, msg); /////// se alguma conecao com montante falhar esta funcao trata disso

}
/*
    funcao que atualiza o registo e analisa se este pop que chegou vai ser utilizado ou se ja nao faz parte do bestpops
*/
int retira_registo(parametros_entrada *entry, pedido resposta, int ID_msg)
{
    if(entry->pedido_efetuado.queryID == resposta.queryID && entry->pedido_efetuado.bestpops > 0)
    {
        if(entry->pedido_efetuado.bestpops >= resposta.bestpops){ //////apenas decrementa o numero de tcp sessions que vai aguardar
            entry->pedido_efetuado.bestpops -= resposta.bestpops;
        }
        else
        {
            entry->pedido_efetuado.bestpops = 0;
        }
        return 1; ///// returnar 1 significa que quero usar este pop

    }

    else
    {
        return 0; //////// ///// returnar 0 significa que nao quero usar este pop
    }

}
/*
    funcao que atualiza o registo de pop query efectuado
*/
void regista_pedido(int ID, int bp, pedido *solicitacao)
{
    solicitacao->bestpops = bp;
    solicitacao->queryID = ID;
}
/*
    funcao que analisa que é preciso fazer um pop query, e se for, efectua
*/
void pop_query_needed(parametros_entrada *entry)
{
    if (entry->iamroot && entry->meu_filhos_ocupados > 0 && conta_avail(entry->points_of_presence_of_tree_head) <= entry->bestpops){
        regista_pedido(entry->queryID,entry->bestpops,&entry->pedido_efetuado);
        send_pop_query(entry,entry->queryID,entry->bestpops);
        incrementa(&entry->queryID);
    }
}
/*
    conta o numero de tcp sessions livres na lista de bestpops
*/
int conta_avail(ponto_presenca *head)
{
    int soma = 0;
    ponto_presenca *aux = head;

    while (aux != NULL)
    {
        soma += aux->avail;
        aux = aux->next;
    }

    return soma;

}
