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
#define CONEX_LIMIT 3		//Número máximo de espera para perda de conexão
#define TIMEOUT_MS 1000		//Tempo timeout
#define N_ROT 4				//Número de roteadores
#define MSG_SIZE 100		//Tamanho máximo da mensagem
#define QUEUE_SIZE 100		//Tamanho da fila dos roteadores
#define TRUE 1				//Verdadeiro
#define FALSE 0				//Falso
#define ERROR -1			//Estado de Erro
#define INFINITE 999999		//Infinito

typedef struct{				//Estrutura do Cabeçalho
	int type;				//Tipo do pacote
	int origin, dest;		//Origem e Destino
	int num_pack;			//número do pacote
}Packet_Header;

typedef struct{				//Pacotes de Dados
	Packet_Header header; 	//Cabeçalho
	char message[MSG_SIZE];	//Mensagem de dados
}Data_Packet;

typedef struct{   			//Pacote de Configuração
	Packet_Header header; 	//Cabeçalho
	int dist_cost[N_ROT]; 	//Vetor Distancia
	int dist_path[N_ROT];
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

typedef struct{
	int dist_cost[N_ROT];	//Vetor distancia
	int dist_path[N_ROT];	//Vetor caminho
	int last_rec;			//Contagem de atualização
	int link_cost;			//custo do enlace para o vizinho
	short is_neigh;			//bool para saber se é vizinho ou não
}Links;
