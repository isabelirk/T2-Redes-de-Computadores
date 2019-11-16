#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio_ext.h>
#include <semaphore.h>  
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <sys/time.h>

#define TIMEOUT_MAX 10		//Quantidade Maxima de Reenvios
#define TIMEOUT_MS 100		//Tempo timeout
#define N_ROT 4				//número de roteadores
#define MSG_SIZE 100		//tamanho da mensagem
#define QUEUE_SIZE 100		//tamanho da fila dos roteadores
#define TRUE 1
#define FALSE 0
#define INFINITE 999999

typedef struct{				//Estrutura do Cabeçalho
	int type;				//Tipo do pacote
	int num_pack;			//número do pacote
	int origin, dest;		//Origem e Destino
}Packet_Header;

typedef struct{				//Pacotes de Dados
	Packet_Header header; 	//Cabeçalho
	char message[MSG_SIZE];	//Mensagem de dados
}Data_Packet;

typedef struct{   			//Pacote de Configuração
	Packet_Header header; 	//Cabeçalho
	int dists_cost[N_ROT]; 	//Vetor Distancia
}Config_Packet;

typedef struct{   			//Pacote de Confirmação
	Packet_Header header; 	//Cabeçalho
	int num_confirm_pack;  	//Número do pacote que foi confirmado
}Ack_Packet;

typedef struct{   			//Estrutura de cada roteador
	int id;         		//Identificador do roteador
	int port;       		//Porta do roteador
	int num_waiting_ack;   	//Número do pacote que está esperando confirmação
	short waiting_ack;     	//Booleana ack
	char ip[32];    		//Endereço IP do roteador
}Router;

typedef struct{				//Estrutura da tabela de roteamento
	int cost[N_ROT];		//Custo até o destino
	int path[N_ROT];		//Próximo Salto
}Table;