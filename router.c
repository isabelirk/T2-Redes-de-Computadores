#include "router.h"

pthread_t receiver_thread, update_links_thread, sender_thread;
pthread_mutex_t update_links_table = PTHREAD_MUTEX_INITIALIZER;
struct sockaddr_in si_me, si_other;

Router router[N_ROT];
Table router_table;
Links links_table[N_ROT];

int router_socket, id_router;								//Configurações do roteador
int pct_enum = 1, count_pct = 0, update_time = 0;			//Controles de Pacotes
Data_Packet pct_storage[QUEUE_SIZE];

void die(char *s){
	perror(s);
	exit(1);
}

void menu(){
	sleep(3);
	printf("\t┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓\n");
	printf("\t┃                           Roteador %02d                        ┃\n", id_router+1);
	printf("\t┣━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┫\n");
	printf("\t┃                     ➊ ─ Enviar mensagem ─ ➊                  ┃\n");
	printf("\t┣━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┫\n");
	printf("\t┃               ➋ ─ Ver historico de mensagens ─ ➋             ┃\n");
	printf("\t┣━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┫\n");
	printf("\t┃                         ⓿ ─ Sair ─ ⓿                         ┃\n");
	printf("\t┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛\n\n");
}

void show_messages(){
	for(int i = 0; i < count_pct; i++){
		printf("\t┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓\n");
		printf("\t┃      Historico de mensagens recebidas pelo roteador %02d        ┃\n", id_router+1);
		printf("\t┣━━━━━━━━━━━┳━━━━━━━━┳━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┫\n");    
		printf("\t┃ ID Origem ┃ Nº MSG ┃                 Mensagem                 ┃\n");
		printf("\t┗━━━━━━━━━━━┻━━━━━━━━┻━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛\n\n");
		printf("\t      %d         %02d     %40s \n", pct_storage[i].header.origin+1, pct_storage[i].header.num_pack, pct_storage[i].message);
	}
}

void print_dist(){
	printf("\t┏━━━━━━━━━━━━━━━━━━━┳━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┳━━━━━━━━━━━┓\n");
    printf("\t┃  Vértice Destino  ┃  Proximo vértice do Caminho  ┃   Custo   ┃\n");
    printf("\t┣━━━━━━━━━━━━━━━━━━━╋━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━╋━━━━━━━━━━━┫\n");
    for(int i = 0; i < N_ROT; i++){ 
		// if(i == id_router)
		// 	continue;
		if(router_table.path[i] == ERROR)
			printf("\t┃         %d         ┃               -              ┃     ∞     ┃\n", i+1);
		else
			printf("\t┃         %d         ┃               %d              ┃ %5d     ┃\n", i+1, router_table.path[i]+1, router_table.cost[i]);
    }
    printf("\t┗━━━━━━━━━━━━━━━━━━━┻━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┻━━━━━━━━━━━┛\n\n");
	sleep(3);
}

void clean_tables(){
	memset(links_table, ERROR, sizeof(Links) * N_ROT); 	//limpa a tabela router
	memset(router_table.path, ERROR, sizeof(int) * N_ROT);
	for(int i = 0; i < N_ROT; i++){
		links_table[i].link_cost = INFINITE;
		for(int j = 0; j < N_ROT; j++){
			links_table[i].dist_cost[j] = INFINITE;
		}
		links_table[i].is_neigh = FALSE;
	}

	router_table.cost[id_router] = links_table[id_router].dist_cost[id_router] = links_table[id_router].last_rec = 0;
	router_table.path[id_router] = id_router;
}

void create_router(){ //função que cria os sockets para os roteadores
	memset((char *) &si_other, 0, sizeof(si_other));
	si_other.sin_family = AF_INET;
	si_other.sin_addr.s_addr =  htonl(INADDR_ANY);
	
	FILE *config_file = fopen("roteadores.config", "r");

	if(!config_file)
		die("\t Não foi possivel abrir o arquvio de configuração dos roteadores! ");

	for (int i = 0; fscanf(config_file, "%d %d %s", &router[i].id, &router[i].port, router[i].ip) != EOF; i++);
	fclose(config_file);

	printf("\t┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓\n");
	printf("\t┃                  Informações do Roteador %02d                  ┃\n", id_router+1);
	printf("\t┣━━━━━━━━━━━━━┳━━━━━━━━━━━┳━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┫\n");    
	printf("\t┃ ID Roteador ┃   Porta   ┃           Endereço de IP           ┃\n");
	printf("\t┣━━━━━━━━━━━━━╋━━━━━━━━━━━╋━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┫\n");
	printf("\t┃     %02d      ┃  %6d   ┃  %32s  ┃\n", router[id_router].id,  router[id_router].port,  router[id_router].ip);
	printf("\t┗━━━━━━━━━━━━━┻━━━━━━━━━━━┻━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛\n\n");
	sleep(2);

	if((router_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == ERROR)
		die("\t Erro ao criar socket! ");

	memset((char *) &si_me, 0, sizeof(si_me));
	
	si_me.sin_family = AF_INET;
	si_me.sin_port = htons(router[id_router].port);
	si_me.sin_addr.s_addr = htonl(INADDR_ANY);

	if(bind(router_socket, (struct sockaddr *) &si_me, sizeof(si_me)) == ERROR)
		die("\t Erro ao conectar o socket a porta! ");
}

void read_links(){ //função que lê os enlaces
	int x, y, cost;
	
	FILE *file = fopen("enlaces.config", "r");

	if (file){
		for (int i = 0; fscanf(file, "%d %d %d", &x, &y, &cost) != EOF; i++){
			if((x-1) == id_router){
				links_table[y-1].is_neigh = TRUE;		
				links_table[y-1].link_cost = cost;
				links_table[x-1].dist_cost[y-1] = cost;
				links_table[x-1].dist_path[y-1] = y-1; 
				router_table.cost[y-1] = cost;
				router_table.path[y-1] = y-1; 
			}
			else if((y-1) == id_router){
				links_table[x-1].is_neigh = TRUE;		
				links_table[x-1].link_cost = cost;		
				links_table[y-1].dist_cost[x-1] = cost;
				links_table[y-1].dist_path[x-1] = x-1; 
				router_table.cost[x-1] = cost;
				router_table.path[x-1] = x-1; 
			}	
		}
		fclose(file);
	}
}

void send_links(){
	int i;
	Config_Packet message_out;
	message_out.header.type = 0;
	message_out.header.origin = id_router;

	memcpy(message_out.dist_cost, links_table[id_router].dist_cost, sizeof(int)*N_ROT);
	memcpy(message_out.dist_path, links_table[id_router].dist_path, sizeof(int)*N_ROT);

	printf("\t┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓\n");
	printf("\t┃ Enviando meu vetor distancia                                 ┃\n");
	printf("\t┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛\n");

	for(i = 0; i < N_ROT; i++){
		if(links_table[i].is_neigh){
			message_out.header.num_pack = pct_enum;
			message_out.header.dest = i;
			pct_enum++; 								//atualiza a quantidade de mensagem que foram enviadas

			si_other.sin_port = htons(router[i].port);
			if(inet_aton(router[i].ip, &si_other.sin_addr) == 0)
				die("\t Erro ao tentar encontrar o IP destino inet_aton() ");
			else{
				if(sendto(router_socket, &message_out, sizeof(Config_Packet), 0, (struct sockaddr*) &si_other, sizeof(si_other)) == ERROR)
				die("\t Erro ao enviar a mensagem! sendto() ");
			}
		}
	}
}

void update_dist(){

	int ver = FALSE, link_cost = INFINITE;
	time_t clk = time(NULL);

	for(int i = 0; i < N_ROT; i++){
		if(!links_table[i].is_neigh)
			continue;
		link_cost = links_table[i].link_cost;
		for(int j = 0; j < N_ROT; j++){
			if((links_table[id_router].dist_cost[j] > links_table[i].dist_cost[j] + link_cost)){
				pthread_mutex_lock(&update_links_table);
				links_table[id_router].dist_cost[j] = links_table[i].dist_cost[j] + link_cost;
				links_table[id_router].dist_path[j] = i;
				router_table.path[j] = i;
				router_table.cost[j] = links_table[i].dist_cost[j] + link_cost;
				pthread_mutex_unlock(&update_links_table);
				clk = time(NULL);
				ver = TRUE;
			}
		}
	}

	if(ver){
		update_time += 15;
		printf("\t┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓\n");
		printf("\t┃ Vetor Distancia alterado em: %s", ctime(&clk));
		printf("\t┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛\n");
		print_dist();
		send_links();

	}
}

void *update_links(void *data){
	sleep(20);

	while(1){
		sleep(10);
		update_time = 0;
		for(int i = 0; i < N_ROT; i++){
			if(links_table[i].is_neigh || links_table[i].last_rec > 3){
				pthread_mutex_lock(&update_links_table);
				links_table[i].last_rec++;
				printf("\tEsperando receber %d %d\n", i+1, links_table[i].last_rec);

				if(links_table[i].last_rec > CONEX_LIMIT){
					links_table[id_router].dist_cost[i] = INFINITE;
					links_table[i].is_neigh = FALSE;
					for(int j = 0; j < N_ROT; j++){
						if(links_table[id_router].dist_path[j] == i){
							links_table[id_router].dist_cost[j] = INFINITE;
							links_table[id_router].dist_path[j] = ERROR;
						}
					}
					pthread_mutex_unlock(&update_links_table);
					send_links();
					update_dist();
				}
				else{
					pthread_mutex_unlock(&update_links_table);
					send_links();
				}
			}
		}
	}
}

void send_message(Data_Packet message_out){//função que enviar mensagem
	int timeouts = 0;
	int next = router_table.path[message_out.header.dest];
	printf("\t┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓\n");
	printf("\t┃ Enviando mensagem n°%02d para o roteador de ID %02d...           ┃\n", message_out.header.num_pack, next+1);
	printf("\t┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛\n\n");
	
	si_other.sin_port = htons(router[next].port); //enviando para o socket

	if(inet_aton(router[next].ip, &si_other.sin_addr) == 0)
		die("\t Erro ao tentar encontrar o IP destino inet_aton() ");

	else{
		do{
			if(message_out.header.origin == id_router)
				router[id_router].waiting_ack = TRUE;
			
			if(sendto(router_socket, &message_out, sizeof(Data_Packet), 0, (struct sockaddr*) &si_other, sizeof(si_other)) == ERROR)
				die("\t Erro ao enviar a mensagem! sendto() ");

			int cont = 1;

			while(cont++ != TIMEOUT_MS && router[id_router].waiting_ack){
				if(!router[id_router].waiting_ack)
					break;
				else
					usleep(10000);
			}

			if(router[id_router].waiting_ack){
				timeouts++;
				printf("\t┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓\n");
				printf("\t┃ Tempo de envio esgotado, tentando enviar novamente...        ┃\n");
				printf("\t┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛\n\n");
			}
		}while(timeouts < TIMEOUT_MAX && router[id_router].waiting_ack);

		if(message_out.header.origin == id_router){
			if(!router[id_router].waiting_ack){
				printf("\t┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓\n");
				printf("\t┃ ACK Confirmada! A mensagem foi recebida pelo destinatário... ┃\n");
				printf("\t┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛\n\n");
			}
			else{
				router[id_router].waiting_ack = FALSE;
				printf("\t┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓\n");
				printf("\t┃ Envio de mensagem cancelado! Número de tentativas excedido.. ┃\n");
				printf("\t┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛\n\n");
			}
		}
	}	
}

void create_message(){						//função cria mensagem
	int destination;
	Data_Packet message_out; 

	do{ 									//verificação do roteador destino
		printf("\t┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓\n");
		printf("\t┃ Digite o ID do roteador para enviar a mensagem...            ┃\n");
		printf("\t┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛\n\n\t ");
		scanf("%d", &destination);
		destination = destination - 1;
		if(destination < 0 || destination >= N_ROT){
			printf("\t┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓\n");
			printf("\t┃ O ID informado é inválido. Por favor digite novamente...     ┃\n");
			printf("\t┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛\n\n\t ");
		}
	}while(destination < 0 || destination >= N_ROT);

	printf("\t┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓\n");
	printf("\t┃ Digite o conteudo da mensagem a ser enviada para o roteador: ┃\n");
	printf("\t┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛\n\n\t  ");

	__fpurge(stdin); 						//limpar o buffer de algum lixo de memória
	fgets(message_out.message, MSG_SIZE, stdin);

	message_out.header.num_pack = pct_enum;
	message_out.header.origin = id_router;
	message_out.header.dest = destination;
	message_out.header.type = 1;

	pct_enum++; 						

	send_message(message_out);
}

void *receiver(void *data){ //função da thread receiver
	int slen = sizeof(si_other);
	int next, pct_type, pct_dest;
	char buffer[sizeof(Data_Packet)];

	while(1){
		if((recvfrom(router_socket, &buffer, sizeof(buffer), 0, (struct sockaddr *) &si_me, &slen)) == ERROR)
			die("\tErro ao receber mensagem! recvfrom() ");

		pct_type = *(int *) buffer;
		pct_dest = ((Data_Packet *) buffer)->header.dest; 
		
		if(pct_dest == id_router){
			if(pct_type == 0){
				Config_Packet message_in = *(Config_Packet *)buffer;
				printf("\t┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓\n");
				printf("\t┃ Vetor Distancia - MSG Nº %02d recebido do roteador com ID %02d...┃\n", message_in.header.num_pack, message_in.header.origin+1);
				printf("\t┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛\n\t  ");
				
				pthread_mutex_lock(&update_links_table);
				links_table[message_in.header.origin].last_rec = 0;
				links_table[message_in.header.origin].is_neigh = TRUE;
				memcpy(links_table[message_in.header.origin].dist_cost, message_in.dist_cost, sizeof(int)*N_ROT);
				memcpy(links_table[message_in.header.origin].dist_path, message_in.dist_path, sizeof(int)*N_ROT);

				for(int i = 0; i < N_ROT; i++)
					printf(message_in.dist_cost[i] == INFINITE ? "∞ " : "%d ", message_in.dist_cost[i]);
				printf("\n");
				pthread_mutex_unlock(&update_links_table);
				update_dist();
			}
			else if(pct_type == 1){
				Data_Packet message_in = *(Data_Packet *)buffer;
				printf("\t┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓\n");
				printf("\t┃ Mensagem Nº %02d recebido do roteador com ID %02d...             ┃\n", message_in.header.num_pack, message_in.header.origin+1);
				printf("\t┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛\n");
				printf("\t Mensagem: %50s \n", message_in.message);
				
				memcpy((pct_storage+(count_pct*sizeof(int))), &message_in, sizeof(Data_Packet));
				count_pct++;

				Ack_Packet ack_reply;
				ack_reply.header.type = 2;
				ack_reply.header.origin = id_router;
				ack_reply.header.dest = message_in.header.origin;
				ack_reply.header.num_pack = pct_enum;
				pct_enum++;

				si_other.sin_port = htons(router[ack_reply.header.dest].port); 

				if(sendto(router_socket, &ack_reply, sizeof(Ack_Packet), 0, (struct sockaddr*) &si_other, sizeof(si_other)) == ERROR)
					die("\tErro ao enviar a mensagem de ack! sendto() ");
			}
			else if(pct_type == 2){
				Ack_Packet message_in = *(Ack_Packet *)buffer;
				if(router[id_router].waiting_ack)
					router[id_router].waiting_ack = FALSE;
			}
		}
		else{
			Data_Packet message_in = *(Data_Packet *)buffer;
			printf("\t┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓\n");
			printf("\t┃ Encaminhado mensagem nº%02d do roteador %02d para o roteador %02d..┃\n", message_in.header.num_pack, id_router+1, router_table.path[message_in.header.dest]+1);
			printf("\t┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛\n\n");
			sleep(2);
			send_message(message_in);
		}
	}
}

void *main(){
	do{ 									//verificação do roteador destino
		printf("\t┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓\n");
		printf("\t┃ Digite o ID do roteador que deseja iniciar...                ┃\n");
		printf("\t┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛\n\n\t ");
		scanf("%d", &id_router);
		id_router = id_router - 1;
		if(id_router < 0 || id_router >= N_ROT){
			printf("\t┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓\n");
			printf("\t┃ O ID informado é inválido. Por favor digite novamente...     ┃\n");
			printf("\t┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛\n\n\t ");
		}
	}while(id_router < 0 || id_router >= N_ROT);

	clean_tables();														//função que limpa as tabelas de lixo

	read_links(); 														//função que lê do arquivo enlaces.config

	create_router(); 													//função que lê e cria os roteadores do arquivo roteadores.config

	print_dist();														//função que mostra os caminhos e o custo pra os destinos

	send_links();														//envia vetor distancia pela primeira vez

	pthread_create(&receiver_thread, NULL, receiver, NULL); 			//cria thread do receiver, que irá escutar na porta determinada

	pthread_create(&update_links_thread, NULL, update_links, NULL);		//cria thread do update_links, que enviará e verificará esporadicamente os vetores distancia

	while(1){ 															//thread main, aproveitada para ser sender
		int op;
		menu();
		scanf("%d", &op);
		switch(op){
			case 0:					//sair
				exit(0);
				break;
			case 1: 				//enviar mensagem
				create_message();
				break;
			case 2: 				//ver mensagens anteriores
				show_messages();
				break;
			default:				//mensagem de erro
				printf("\t┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓\n");
				printf("\t┃ Opção inválida, digite novamente..┃\n");
				printf("\t┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛\n\n\t  ");
				break;
		}
	}
}
    