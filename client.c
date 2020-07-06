#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "library.h"

int main(int argc, char const *argv[]){
	struct sockaddr_in sv_addr;
	int sd, ret;
	char buffer[1024], c;

	if(argc < 2){
		perror("Non si è specificato l'indirizzo e la porta del server\n");
		exit(1);
	}else if(argc < 3){
		perror("Non si è specificata la porta del server\n");
		exit(1);
	}
	
	sd = socket(AF_INET, SOCK_STREAM, 0);
	if(sd < 0){
		perror("Socket non creato!\n");
		exit(1);
	}

	memset(&sv_addr, 0, sizeof(sv_addr));
	sv_addr.sin_family = AF_INET;
	sv_addr.sin_port = htons(atoi(argv[2]));
	inet_pton(AF_INET, argv[1], &sv_addr.sin_addr);
	
	//Richiesta di connessione con il server 
	ret = connect(sd, (struct sockaddr*)&sv_addr, sizeof(sv_addr));
	if(ret < 0){
		perror("Impossibile connettersi con il server %s sulla porta %s!\n",argv[1],argv[2]);
		exit(1);
	}
	
	//Dopo aver effettuato la richiesta di connessione al server, il client riceve il flag bloccato
	//dal server che gli notifica o meno se è appunto bloccato o meno
	bloccato = recvFromServerFlag(sd);
	
	//Se il client è bloccato la socket viene chiusa subito
	if(bloccato){
		printf("Sei stato bloccato aspetta ancora!\n");
	}else{
		//Altrimenti la connessione viene instaurata con successo e il client può interagire con l'applicazione
		printf("Connessione al server %s (porta %s) effettuata con successo\n\n",argv[1],argv[2]);
		
		memset(&buffer, '\0', sizeof(buffer));
	
		//Il client visualizza il menù dell'applicazione con tutti i comandi che si possono effettuare
		welcome();
	
		while(1){
		
			printf("> ");
		
			scanf("%s", buffer);
			
			//Assegnamento del carattere corrispondente al comando appena letto dalla scanf
			c = AssegnaLettera(buffer);
			
			//Invio al server del carattere corrisponde al comando
			SendToServerCmd(sd, c);
		}
	}	
	
	close(sd);
	return 0;
}