
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <pthread.h>

struct client_data
{
    int sk;
    struct sockaddr_in *client_addr;
};

void *client_handle(void *cd)
{
    struct client_data *client = (struct client_data *)cd;
    char sendBuff[1024];
    char recvBuff[1024];
    char dir[1024] = {"./filedir/"};
    int n;
    FILE *fp;

    memset(sendBuff, 0, sizeof(sendBuff));
    memset(recvBuff, 0, sizeof(recvBuff));

    /* Imprime IP e porta do cliente. */
    printf("Received connection from %s:%d\n", inet_ntoa(client->client_addr->sin_addr), ntohs(client->client_addr->sin_port));
    fflush(stdout);

    sleep(1);

    /* Recebe o nome do arquivo do cliente*/
    while ((n = recv(client->sk, recvBuff, sizeof(recvBuff), 0)) > 0)
    {
        int f = 0;
        /*Abre o arquivo com o nome recebido do cliente*/
        strcat(dir, recvBuff);
        int cont = 0;
        while (cont < sizeof(recvBuff))
        {
            if (recvBuff[cont] == '\0')
            {
                f = 1;
            }
            cont++;
        }
        if (f == 1)
        {
            break;
        }
    }

    printf("\n\n%s\n\n", dir);
    fp = fopen(dir, "r");
    int posip;
    /*garante a abertura do arquivo e envia o tamanho do arquivo*/
    if (fp == NULL)
    {
        printf("Erro ao abrir o arquivo\n");
        fflush(stdout);
        close(client->sk);
        return NULL;
    }
    else
    {
        printf("Arquivo aberto com sucesso!\n");
        fflush(stdout);
        fseek(fp, 0, SEEK_END);
        posip = ftell(fp);
        sprintf(sendBuff, "%d", posip);
        fflush(stdout);
        send(client->sk, sendBuff, 32, 0);
        printf("\nEnviado o tamanho do arquivo! Tamanho = %s\n", sendBuff);
        fflush(stdout);
    }
    memset(sendBuff, 0, sizeof(sendBuff));
    char command[1024] = "md5sum ";
    /*Envia o código hash */
    FILE *fpipe;
    strcat(command, dir);
    //printf("\n---->%s<----\n", command);

    if ((fpipe = popen(command, "r")) < 0)
    {
        perror("popen");
    }

    fgets(sendBuff, 33, fpipe);
    //printf("o tamanho do pacote é %d", sizeof(sendBuff));

    if (send(client->sk, sendBuff, 33, 0) < 0)
    {
        perror("hash");
    }

    /*Envia o arquivo em pacotes de 1024 */
    fseek(fp, 0, SEEK_SET);
    while (feof(fp) == 0)
    {
        memset(sendBuff, 0, sizeof(sendBuff));
        n = fread(sendBuff, 1, 1024, fp);
        printf("\n---->%d", n);

        if (send(client->sk, sendBuff, n, 0) == -1)
        {
            printf("Erro ao enviar a menssagem");
            fflush(stdout);
        }
    }
    printf("Todo o arquivo foi enviado! \n");
    fflush(stdout);

    if (fputs(recvBuff, stdout) == EOF)
    {
        perror("fputs");
    }

    //snprintf(sendBuff, sizeof(sendBuff), "%.24s\r\n", ctime(&ticks));

    /* Envia resposta ao cliente. */
    //send(client->sk, sendBuff, strlen(sendBuff)+1, 0);

    /* Fecha conexão com o cliente. */
    close(client->sk);

    free(client->client_addr);
    free(client);
    printf("Conexão com o cliente encerrada \n");

    return NULL;
}

int main(int argc, char *argv[])
{
    int listenfd = 0;
    struct sockaddr_in serv_addr;
    int addrlen;
    struct client_data *cd;
    pthread_t thr;

    /* Cria o Socket: SOCK_STREAM = TCP */
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket");
        return 1;
    }

    memset(&serv_addr, 0, sizeof(serv_addr));

    /* Configura servidor para receber conexoes de qualquer endereço:
	 * INADDR_ANY e ouvir na porta 5000 */
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(5000);

    /* Associa o socket a estrutura sockaddr_in */
    if (bind(listenfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("bind");
        return 1;
    }

    /* Inicia a escuta na porta */
    listen(listenfd, 10);

    while (1)
    {
        cd = (struct client_data *)malloc(sizeof(struct client_data));
        cd->client_addr = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));
        addrlen = sizeof(struct sockaddr_in);

        /* Aguarda a conexão */
        cd->sk = accept(listenfd, (struct sockaddr *)cd->client_addr, (socklen_t *)&addrlen);

        pthread_create(&thr, NULL, client_handle, (void *)cd);
        pthread_detach(thr);
    }
}
