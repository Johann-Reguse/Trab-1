
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>

int main(int argc, char *argv[])
{
    int sockfd = 0, n = 0;
    char recvBuff[1024];
    char sendBuff[1024];
    struct sockaddr_in serv_addr;
    FILE *par;
    char test[1024];
    char nf[1024];

    if (argc != 2)
    {
        printf("\n Usage: %s <ip of server> \n", argv[0]);
        return 1;
    }

    memset(recvBuff, 0, sizeof(recvBuff));
    memset(sendBuff, 0, sizeof(sendBuff));
    memset(nf, 0, sizeof(nf));

    /*Cria o Socket */
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket");
        return 1;
    }

    /* Configura o IP de destino e porta na estrutura sockaddr_in */
    memset(&serv_addr, 0, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(5000);

    if (inet_pton(AF_INET, argv[1], &serv_addr.sin_addr) <= 0)
    {
        perror("inet_pton");
        return 1;
    }

    /* Conecta ao servidor. */
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("connect");
        return 1;
    }
    sleep(1);

    /* pede ao usuário o nome do arquivo*/
    printf("Qual o nome do arquivo que voce gostaria? ");
    fflush(stdout);
    scanf("%s", (char *)&nf);
    fflush(stdout);
    /* escreve no buffer o nome do arquivo */
    snprintf(sendBuff, strlen(nf) + 1, "%s", nf);
    fflush(stdout);
    n = sizeof(nf) + 1;
    sendBuff[n] = '\0';

    /*envia o nome do arquivo para o servidor*/
    send(sockfd, sendBuff, sizeof(sendBuff), 0);
    par = fopen(nf, "w+");
    int tfile;

    if (recv(sockfd, recvBuff, 4, 0) < 0)
    {
        printf("Erro ao abrir o arquivo no servidor");
        fflush(stdout);
        fclose(par);
        return 1;
    }
    else
    {
        tfile = atoi(recvBuff);
        if (tfile == 0)
        {
            printf("Arquivo inexistente no servidor");
            close(sockfd);
            return 1;
        }
        printf("\n O tamanho do arquivo é %d\n", tfile);
    }
    char hash[33];
    /*recebe o código hash*/
    if (recv(sockfd, recvBuff, 33, 0) < 0)
    {
        perror("hash");
    }
    else
    {
        sprintf(hash, "%s", recvBuff);
    }

    memset(recvBuff, 0, sizeof(recvBuff));
    memset(test, 0, sizeof(test));
    fseek(par, 0, SEEK_SET);
    /*recebe o arquivo */
    while ((n = recv(sockfd, recvBuff, sizeof(recvBuff), 0)) > 0)
    {

        if (strlen(recvBuff) < 1024)
        {
            //recvBuff[69]='&';
            fwrite(recvBuff, 1, strlen(recvBuff), par);
        }
        else
        {

            fwrite(recvBuff, 1, 1024, par);
        }

        memset(recvBuff, 0, sizeof(recvBuff));
    }
    printf("Aquivo recebido com sucesso!! \n");

    /*compara o hash dos arquivos*/
    fclose(par);
    FILE *fpipe;
    char command[1024] = "md5sum ";
    strcat(command, nf);

    if ((fpipe = popen(command, "r")) < 0)
    {
        perror("popen");
    }

    fgets(test, 33, fpipe);

    if (strcmp(test, hash) == 0)
    {
    }
    else
    {
        printf("Erro na verificação do código hash ------>test: %s\n------>hash: %s \n ", test, hash);
        remove(nf);
    }
    fclose(fpipe);
    close(sockfd);

    return 0;
}
