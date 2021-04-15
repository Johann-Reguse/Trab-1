#include <unistd.h>
#include<stdio.h>
#include<stdlib.h>
#include<semaphore.h>
#include<pthread.h>

// Definição do número de filósofos na mesa
#define N 5

// Definir do estado que cada filósofo pode ficar
#define PENSAR 0
#define FOME 1
#define COMER 2

//Definição de qual filósofo está a direita/esquerda de cada um 
#define ESQUERDA (nfilosofo+(N-1))%N //Olhar para o garfo a esquerda
#define DIREITA (nfilosofo+1)%N  //Olhar o garfo a direita

//Declaração das funções
void *filosofo(void *num);
void agarraGarfo(int);
void deixaGarfo(int);
void testar(int);

//Variáveis globais 
pthread_mutex_t mutex;
sem_t S[N]; //inicializacao do semáforo
int estado[N];
int nfilosofo[N];

void *filosofo(void *num)
{
   while(1)
   {
      int *i = num;
      sleep(1);
      agarraGarfo(*i);
      sleep(1);
      deixaGarfo(*i);
   }
}

void agarraGarfo(int nfilosofo)
{
   pthread_mutex_lock(&mutex);
   estado[nfilosofo] = FOME;
   printf("Filosofo %d ta com fominha.\n", nfilosofo+1);
   //+1 para imprimir filosofo 1 e nao filosofo 0
   testar(nfilosofo);
   pthread_mutex_unlock(&mutex);
   sem_wait(&S[nfilosofo]);
   sleep(1);
}

void deixaGarfo(int nfilosofo)
{
   pthread_mutex_lock(&mutex);
   estado[nfilosofo] = PENSAR;
   printf("Filosofo %d soltou os garfos %d e %d.\n", nfilosofo+1, ESQUERDA+1, nfilosofo+1);
   printf("Filosofo %d esta filosofando.\n", nfilosofo+1);
   testar(ESQUERDA);
   testar(DIREITA);
   pthread_mutex_unlock(&mutex);
}

void testar(int nfilosofo)
{
   if(estado[nfilosofo] == FOME && estado[ESQUERDA] != COMER && estado[DIREITA] != COMER)
   {
      estado[nfilosofo] = COMER;
      sleep(2);
      printf("Filosofo %d pegou os garfos %d e %d.\n", nfilosofo+1, ESQUERDA+1, nfilosofo+1);
      printf("Filosofo %d esta se deliciando.\n", nfilosofo+1);
      sem_post(&S[nfilosofo]);
   }
}

int main() {
    pthread_mutex_init(&mutex, NULL);

   for(int count = 0; count < N; count++){
       nfilosofo[count] = count;
    }


   int i;
    pthread_t thread_id[N]; //identificadores das threads
   
   for(i=0;i<N;i++)
      sem_init(&S[i],0,0);
   for(i=0;i<N;i++)
   {
      pthread_create(&thread_id[i], NULL, filosofo, &nfilosofo[i]);
      //criar as threads
      printf("Filosofo %d esta filosofando.\n",i+1);
   }
   for(i=0;i<N;i++)
   pthread_join(thread_id[i],NULL); //para
                                    //fazer a junção das threads
   return(0);
}