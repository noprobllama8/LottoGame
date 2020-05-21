#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "library.h"

#define TRUE 1

bool bloccato;
int tentativi, mode;
char sessionId[11];

//struttura delle ruote giocate
struct ruote{
	char nome_ruota[10];
	struct ruote *next;
};

//struttura dei numeri giocati
struct numeri{
	int numero;
	struct numeri *next;
};

//struttura degli importi giocati
struct importi{
	float importo;
	int tipo;						//1 = estratto - 2 = ambo - 3 = terno - 4 = quaterna - 5 = cinquina
	struct importi *next;
};

/*Menu del gioco del lotto*/
void welcome(){
	printf("\n********************************GIOCO DEL LOTTO********************************\n");
	printf("Sono disponibili i seguenti comandi:\n\n");
	printf("1) !help <comando> --> mostra i dettagli di un comando\n");
	printf("2) !signup <username> <password> --> crea un nuovo utente\n");
	printf("3) !login <username> <password> --> autentica un utente\n");
	printf("4) !invia_giocata g --> invia una giocata g al server\n");
	printf("5) !vedi_giocate tipo --> visualizza le giocate precedenti dove tipo = (0,1)\n");
	printf("                          e permette di visualizzare le giocate passate '0'\n");
	printf("                          oppure le giocate attive '1' (ancora non estratte)\n");
	printf("6) !vedi_estrazione <n> <ruota> --> mostra i numeri delle ultime n estrazioni\n");
	printf("				    sulla ruota specificata\n");
	printf("7) !vedi_vincite --> richiede al server tutte le vincite del client\n");
	printf("8) !esci --> termina il client\n");
	printf("*******************************************************************************\n");	
}

//funzione di assegnamento della lettera per ogni comando
char AssegnaLettera(char *cmd){
	if(!strcmp("!help", cmd)){
		return 'h';
	}else if(!strcmp("!signup", cmd)){
		return 's';
	}else if(!strcmp("!login", cmd)){
		return 'l';
	}else if(!strcmp("!invia_giocata", cmd)){
		return 'i';
	}else if(!strcmp("!vedi_giocate", cmd)){
		return 'g';
	}else if(!strcmp("!vedi_estrazione", cmd)){
		return 'e';
	}else if(!strcmp("!vedi_vincite", cmd)){
		return 'v';
	}else if(!strcmp("!esci", cmd)){
		return 'q';
	}else{
		return 'z';
	}
}

//funzione di riempimento della lista delle ruote
bool fillListaRuote(struct ruote **r0, char *ruota){
	struct ruote *q, *p;
	
	//controllo se il client ha gia' inserito quella ruota
	if(*r0 != NULL){
		for(q = *r0; q != 0; q = q->next){
			if(!strcmp(q->nome_ruota, ruota))
				return true;
		}
	}
	
	for(q = *r0; q != 0; q = q->next)
		p = q;
		
	q = (struct ruote*)malloc(sizeof(struct ruote));
	
	strcpy(q->nome_ruota, ruota);
	q->next = NULL;

	if(*r0 == 0)
		*r0 = q;
	else
		p->next = q;
	
	return false;
}

//funzione di riempimento della lista dei numeri
bool fillListaNumeri(struct numeri **n0, int numero){
	struct numeri *q, *p;
	
	//controllo se il client ha gia' inserito quel numero
	if(*n0 != 0){
		for(q = *n0; q != 0; q = q->next){
			if(q->numero == numero)
				return 1;
		}
	}
	
	for(q = *n0; q != 0; q = q->next)
		p = q;
		
	q = (struct numeri*)malloc(sizeof(struct numeri));
	
	q->numero = numero;
	q->next = NULL;

	if(*n0 == 0)
		*n0 = q;
	else
		p->next = q;
	
	//printf("%d->", q->numero);
	
	return 0;
}

//funzione di riempimento della lista degli importi
bool fillListaImporti(struct importi **i0, float importo, int tipo){
	struct importi *q, *p;
	
	for(q = *i0; q != 0; q = q->next)
		p = q;
		
	q = (struct importi*)malloc(sizeof(struct importi));
	
	q->importo = importo;
	//printf("importo (filllista): %f\n", q->importo);
	q->tipo = tipo;
	q->next = NULL;

	if(*i0 == 0)
		*i0 = q;
	else
		p->next = q;
	
	return 0;
}

//funzione di controllo sulla ruota
bool checkRuota(char *ruota){
	
	if(!strcmp(ruota, "Bari")){
		return 1;
	}else if(!strcmp(ruota, "Cagliari")){
		return 1;
	}else if(!strcmp(ruota, "Firenze")){
		return 1;
	}else if(!strcmp(ruota, "Genova")){
		return 1;
	}else if(!strcmp(ruota, "Milano")){
		return 1;
	}else if(!strcmp(ruota, "Palermo")){
		return 1;
	}else if (!strcmp(ruota, "Roma")){
		return 1;
	}else if(!strcmp(ruota, "Torino")){
		return 1;
	}else if(!strcmp(ruota, "Venezia")){
		return 1;
	}else if(!strcmp(ruota, "Nazionale")){
		return 1;
	}else{
		return 0;
	}
}

//funzione di stampa delle istruzioni per i comandi passati al comando !help
void stampaIstruzioneHelp(char *command){
	if(!strcmp("signup", command)){
		printf("crea un nuovo utente\n");
	}else if(!strcmp("login", command)){
		printf("autentica un utente\n");
	}else if(!strcmp("invia_giocata", command)){
		printf("invia una giocata g al server\n");
	}else if(!strcmp("vedi_giocate", command)){
		printf("visualizza le giocate precedenti dove tipo = (0,1) e permette di visualizzare le giocate passate '0' oppure le giocate attive '1' (ancora non estratte)\n");
	}else if(!strcmp("vedi_estrazione", command)){
		printf("mostra i numeri delle ultime n estrazioni sulla ruota specificata\n");
	}else if(!strcmp("vedi_vincite", command)){
		printf("richiede al server tutte le vincite del client\n");
	}else if(!strcmp("esci", command)){
		printf("termina il client");
	}else{
		printf("comando errato\n");
	}
}

//funzione di invio al server di un carattere corrispondente al comando
void sendToServerChar(int sd, char c){
	char dim[2];
	int ret;
	
	memset(&dim, '\0', sizeof(dim));
	dim[0] = c;
	
	ret = send(sd, (void*)dim, 2, 0);
	
	if(ret < 0){
		perror("Send errata!\n");
		exit(1);
	}/*else{
		printf("Carattere del comando inviato!\n");
	}*/
}

//funzione per ricevere dal server il flag di controllo 
//sull'username nel comando signup, e su entrambi username
//e password
bool recvFromServerFlag(int sd){
	int ret;
	char dim[2];
	
	memset(&dim, '\0', sizeof(dim));
	
	ret = recv(sd, (void*)dim, 2, 0);
	
	if(ret < 0){
		perror("Receive errata!\n");
		exit(1);
	}/*else{
		printf("Flag ricevuto : %s\n", dim);
	}*/
	
	if(dim[0] == '0')
		return 0;
	else 
		return 1;
}

//funzione per ricevere dal server una stringa
void recvFromServer(int sd, int stato){
	int ret, len;
	char dim[2], buffer[1024];
	float i;
	
	memset(&dim, '\0', sizeof(dim));
	
	ret = recv(sd, (void*)dim, 2, 0);
	
	len = atoi(dim);				//string -> int
	
	if(ret < 0){
		perror("Receive errata!\n");
		exit(1);
	}/*else{
		printf("Lunghezza ricevuta : %d\n", len);
	}*/
	
	memset(&buffer, '\0', sizeof(buffer));
	
	ret = recv(sd, (void*)buffer, len, 0);
	if(ret < 0){
		perror("Receive errata!\n");
		exit(1);
	}/*else{
		printf("%s\n", buffer);
	}*/
	
	if(stato == 1){
		strcpy(sessionId, buffer);
	}else if(stato == 2){
		printf("%s ", buffer);
	}else if(stato == 3){
		i = atof(buffer);
		printf("%f ", i);
	}
}

//funzione di invio al server, in base al carattere del comando, delle informazioni
//necessarie per completare l'operazione 
void SendToServerCmd(int sd, char c){	
	char command[16], username[1024], password[1024], opzione[2], buffer[1024], ch, ruota[11];
	bool check;
	int n, cont, i, j, k, w, y, h, z, x/*, o*/, numero, contN, tipo;
	int numb, flag;
	float importo;

	char rts[7];
	
	//puntatori alle strutture : servono per inserire gli elementi immessi dal client
	struct ruote *r;
	struct numeri *num;
	struct importi *impo;
	
	r = (struct ruote*)malloc(sizeof(struct ruote));
	r = NULL;
	
	num = (struct numeri*)malloc(sizeof(struct numeri));
	num = NULL;
	
	impo = (struct importi*)malloc(sizeof(struct importi));
	impo = NULL;
	
	memset(&command, '\0', sizeof(command));
	memset(&opzione, '\0', sizeof(opzione));
	memset(&buffer, '\0', sizeof(buffer));
	memset(&username, '\0', sizeof(username));
	memset(&password, '\0', sizeof(password));
	
	switch(c){
		case 'h' :
			// !help
			ch = getchar();
			if(ch == '\n'){
				welcome();
			}else{
				scanf("%s", command);
				stampaIstruzioneHelp(command);
			}
			break;
		case 's' :
			// !signup
			scanf("%s", username);
			scanf("%s", password);
			
			sendToServerChar(sd, c);
			sendTo(sd, username);
			sendTo(sd, password);
			
			check = recvFromServerFlag(sd);
			if(check)
				printf("Username gia' in uso\n");
			break;
		case 'l' :
			// !login
			scanf("%s", username);
			scanf("%s", password);
			
			//printf("username : %s\n", username);
			//printf("password : %s\n", password);
			
			sendToServerChar(sd, c);
			sendTo(sd, username);
			sendTo(sd, password);
			
			check = recvFromServerFlag(sd);
			if(!check){
				//mode = 0;
				/*tentativi = recvFromInt(sd);
				if(tentativi > 0){
					printf("Hai ancora %d tentativi!\n", tentativi);
				}else{
					printf("I tentativi sono arrivati a 0! Sei stato bloccato!\n");
					exit(1);
				}*/
			}else{
				recvFromServer(sd, 1);
				printf("sid : %s, Ora sei online!\n", sessionId);
			}
				
			break;
		case 'i' :
			// !invia_giocata
			
			if(sessionId[0] != '\0'){
				scanf("%s", opzione);
				
				if(!strcmp("-r", opzione)){
					do{
						scanf("%s", buffer);
						if(checkRuota(buffer)){
							check = fillListaRuote(&r, buffer);
							if(check){
								printf("Hai gia' inserito la ruota!\nRiprova a rinviare la giocata.\n");
								return;
							}else{
								continue;
							}
						}
						else if((buffer[0] == '-') && (buffer[1] != 'n')){
							printf("L'opzione che hai inviato non e' corretta!\n");
							printf("Riprova a rinviare la giocata.\n");
							return;
						}else if((strcmp("-n", buffer)) != 0){
							printf("La ruota che hai inserito non e' corretta!\n");
							printf("Riprova a rinviare la giocata.\n");
							return;
						}
					}while((strcmp("-n", buffer)) != 0);
					
					cont = 0;
					contN = 0;

					//gestione numeri
					do{
						//scanf dei numeri
						scanf("%s", buffer);
						
						n = atoi(buffer);
							
						//controllo che siano le ruote da inserire
						if(((n >= 1) && (n <= 90)) && (cont <= 10)){
							//metterli in una lista
							check = fillListaNumeri(&num, n);
							if(check){
								printf("Hai gia' inserito un numero!\nRiprova a rinviare la giocata.\n");
								return;
							}else{
								contN++;
								continue;
							}
						}else if((buffer[0] == '-') && (buffer[1] != 'i')){
							printf("L'opzione che hai inviato non e' corretta!\n");
							printf("Riprova a rinviare la giocata.\n");
							return;
						}else if((strcmp("-i", buffer)) != 0){
							printf("I numeri che devi inserire devono essere compresi tra 1 e 90!\n");
							printf("Riprova a rinviare la giocata.\n");
							return;
						}
						
						if(cont > 10){
							printf("Hai inserito piu' di 10 numeri! Bisogna inserire massimo 10 numeri.\n");
							return;
						}
						
						cont++;
					}while((strcmp("-i", buffer)) != 0);		//chiedo di inserire finchè l'utente non inserisce -i
					
					//usciti dal ciclo l'ultima scanf ha preso la stringa -i 
					
					cont = 1;
					
					//gestione importi
					do{
						if(cont <= contN){
							//scanf degli importi 
							scanf("%s", buffer);

							importo = atof(buffer);
						
							//invio importi
							if(importo < 0.0){
								printf("Non puoi inserire importi negativi!\nRiprova a inviare la giocata!\n");
								return;
							}else if(cont > 5){
								printf("Hai inserito troppi importi! Devi inserire un massimo di 5 importi.\n");
								return;
							}else if((ch != '\n') && (cont <= 5)){
								fillListaImporti(&impo, importo, cont);
								cont++;
							}
						
							ch=getchar();
						}else{
							printf("Hai inserito piu' importi che numeri.\n");
							return;
						}
					}while(ch != '\n');
									
					sendToServerChar(sd, c);
					
					//puntatori alle strutture : per scorrere le liste e mandare i dati al server
					struct ruote *p = r;
					struct numeri *q = num;
					struct importi *s = impo;
					
					//scorrimento della lista
					//invio delle ruote al server					
					while(p){
						sendTo(sd, p->nome_ruota);
						p = p->next;
					}
					
					sendTo(sd, "-n");
					
					//scorrimento della lista
					//invio dei numeri al server 
					memset(&buffer, '\0', sizeof(buffer));
					
					while(q){
						sprintf(buffer, "%d", q->numero);
						sendTo(sd, buffer);
						q = q->next;
					}
					
					sendTo(sd, "-i");
					
					//scorrimento della lista
					//invio degli importi al server
					
					while(s){
						gcvt(s->importo, 5, rts);
						//invio l'importo
						sendTo(sd, rts);
						s = s->next;
					}

					sendTo(sd, "fine");
					
					check = recvFromServerFlag(sd);
					
					if(check){
						printf("L'invio della giocata e' avvenuta con successo!\n");
					}
					
					
				}else{
					printf("Hai digitato l'opzione errata!\nIl formato deve essere : \n!invia_giocata -r ruote -n numeri -i importi\n");
					return;
				}
			}else{
				printf("Devi effettuare prima il login per inviare la giocata.\n");
				printf("Se non sei ancora registrato basta digitare il comando !signup e inserire username e password.\n");
				return;
			}				
			break;
		case 'g' :
			// !vedi_giocate
			memset(&buffer, '\0', sizeof(buffer));
			y = 1;
			
			//aspetto lo 0 o l'1
			scanf("%s", buffer);
			
			flag = atoi(buffer);
			
			if(flag < 0 || flag > 1){
				printf("Devi digitare 0 o 1. Riprova a inviare il comando.\n");
				return;
			}
			
			//invio il char del comando
			sendToServerChar(sd, c);
			
			//invio il flag
			sendToInt(sd, flag);
			
			//riceve dal servere le giocate ancora in corso o le giocate passate 
			//in base al flag inviato dal client stesso
			//ricevo dal server il numero delle giocate effettuate
			k = recvFromInt(sd);
				
			for(i = 0; i < k; i++){
				printf("%d) ", y);
		
				//ricevo le ruote
				j = recvFromInt(sd);
				for(w = 0; w < j; w++){
					recvFromServer(sd, 2);
				}
				
				//ricevo i numeri
				j = recvFromInt(sd);
				for(w = 0; w < j; w++){
					n = recvFromInt(sd);
					printf("%d ", n);
				}
			
				//ricevo gli importi
				j = recvFromInt(sd);
				for(w = 0; w < j; w++){
					printf("* ");
					recvFromServer(sd, 3);
					recvFromServer(sd, 2);
				}
				printf("\n");
				y++;
			}
			break;
		case 'e' :
			// !vedi_estrazione n ruota
			memset(&buffer, '\0', sizeof(buffer));
			cont = 0;
			
			if(sessionId[0] != '\0'){
				//leggo il numero
				scanf("%s", buffer);
				
				//che carattere ha premuto il client
				ch = getchar();
					
				numero = atoi(buffer);
				
				//il client non ha premuto invio quindi l'applicazione aspetta la ruota
				if(ch != '\n'){
					scanf("%s", ruota);
					if(!checkRuota(ruota)){
						printf("La ruota che hai inserito non e' quella corretta. Riprova a inviare il comando\n");
						return;
					}
				}
				
				if(numero < 0){
					printf("Hai inserito un numero negativo. Riprova a inviare il comando\n");
					return;
				}
						
				//nel caso in cui ch = invio
				//mando il carattere al client
				sendToServerChar(sd, c);
				
				//invio il carattere dopo la scanf per comunicare al server
				//che il client ha inserito anche la ruota
				sendToServerChar(sd, ch);
				
				//mando il numero
				sendTo(sd, buffer);
				
				//ricevo cont dal server
				cont = recvFromInt(sd);
				
				k = 0;
				
				//mando la ruota
				if(ch != '\n'){
					sendTo(sd, ruota);
					
					//il client si aspetta di ricevere tutte le estrazioni di quella ruota
					if(numero >= cont){
						while(k < cont){
							printf("%s	", ruota);
							for(j = 0; j < 5; j++){
								numb = recvFromInt(sd);
								printf("%d ", numb);
							}
							printf("\n");
							k++; 
						}
					}else{
						while(k < numero){
							printf("%s	", ruota);
							for(j = 0; j < 5; j++){
								numb = recvFromInt(sd);
								printf("%d ", numb);
							}
							printf("\n");
							k++;
						}
					}
				}else{
					
					//il client aspetta di ricevere tutte le estrazioni
					if(numero >= cont){
						while(k < cont){
							for(i = 0; i < 11; i++){
								recvFromServer(sd, 2);
								for(j = 0; j < 5; j++){
									numb = recvFromInt(sd);
									printf("%d ", numb);
								}
								printf("\n");
							}
							k++; 
						}
					}else{
						while(k < numero){
							for(i = 0; i < 11; i++){
								recvFromServer(sd, 2);
								for(j = 0; j < 5; j++){
									numb = recvFromInt(sd);
									printf("%d ", numb);
								}
								printf("\n");
							}
							k++;
						}
					}
					printf("****************************************\n");
				}
			}else{
				printf("Devi effettuare prima il login per inviare la giocata.\n");
				printf("Se non sei ancora registrato basta digitare il comando !signup e inserire username e password.\n");
				return;
			}		
			break;
		case 'v' :
			// !vedi_vincite
			if(sessionId[0] != '\0'){
				
				sendToServerChar(sd, c);
				
				//numero delle giocate del client di username <username>
				i = recvFromInt(sd);
				
				for(j = 0; j < i; j++){
					
					/*printf("Estrazione del ");
					o = recvFromInt(sd);
					printf("%d-", o);
					o = recvFromInt(sd);
					printf("%d-", o);
					o = recvFromInt(sd);
					printf("%d ", o);
					o = recvFromInt(sd);
					printf("alle ore %d:", o);
					o = recvFromInt(sd);
					printf("%d\n", o);*/
					
					//numero delle ruote giocate dal client di username <username>
					k = recvFromInt(sd);
					
					for(w = 0; w < k; w++){
						
						//numero dei numeri giocati dal client di username <username> risultati vincenti
						y = recvFromInt(sd);
						
						if(y > 0){
							recvFromServer(sd, 2);
						
							for(h = 0; h < y; h++){
								numb = recvFromInt(sd);
								printf("%d ", numb);
							}
							
							//numero di importi
							z = recvFromInt(sd);
							
							printf("\t>>\t");
							
							for(x = 0; x < z; x++){
								tipo = recvFromInt(sd);
								
								if(y == 1){
									if(tipo == 1)
										printf("Estratto : ");
								}else if(y == 2){
									if(tipo == 1)
										printf("Estratto : ");
									if(tipo == 2)
										printf(" Ambo : ");
								}else if(y == 3){
									if(tipo == 1)
										printf("Estratto : ");
									if(tipo == 2)
										printf(" Ambo : ");
									if(tipo == 3)
										printf(" Terzina : ");
								}else if(y == 4){
									if(tipo == 1)
										printf(" Estratto : ");
									if(tipo == 2)
										printf(" Ambo : ");
									if(tipo == 3)
										printf(" Terzina : ");
									if(tipo == 4)
										printf(" Quaterna : ");
								}else if(y == 5){
									if(tipo == 1)
										printf(" Estratto : ");
									if(tipo == 2)
										printf(" Ambo : ");
									if(tipo == 3)
										printf(" Terzina : ");
									if(tipo == 4)
										printf(" Quaterna : ");
									if(tipo == 5)
										printf(" Cinquina : ");
								}
								check = recvFromServerFlag(sd);
								if(check)
									recvFromServer(sd, 3);
							}
							printf("\n");
						}
					}
					printf("*********************************************************************\n");
				}
				printf("\n");
				
				printf("Vincite su ESTRATTO : ");
				recvFromServer(sd, 3);
				printf("\n");
				printf("Vincite su AMBO : ");
				recvFromServer(sd, 3);
				printf("\n");
				printf("Vincite su TERNO : ");
				recvFromServer(sd, 3);
				printf("\n");
				printf("Vincite su QUATERNA : ");
				recvFromServer(sd, 3);
				printf("\n");
				printf("Vincite su CINQUINA : ");
				recvFromServer(sd, 3);
				printf("\n");
				printf("\n");
				
			}
			break;
		case 'q' :
			// !esci
			sendToServerChar(sd, c);
			check = recvFromServerFlag(sd);
			if(check){
				if(sessionId[0] != '\0'){
					printf("Log out effettuato con successo!\n");
					memset(&sessionId, '\0', sizeof(sessionId));
					close(sd);
					exit(1);
				}
			}else{
				printf("Log out non effettuato con successo! Riprova!\n");
			}
			break;
		default :
			printf("Comando errato!\n");
			return;
	}
}

int main(int argc, char const *argv[]){
	struct sockaddr_in sv_addr;
	int sd, ret;
	char buffer[1024], c;
	
	//data = (struct user*)malloc(sizeof(struct user));

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
	}/*else{
		printf("Socket creato : %d!\n", sd);
	}*/

	memset(&sv_addr, 0, sizeof(sv_addr));
	sv_addr.sin_family = AF_INET;
	sv_addr.sin_port = htons(atoi(argv[2]));
	inet_pton(AF_INET, argv[1], &sv_addr.sin_addr);
	
	
	//if(il client non è bloccato)
	ret = connect(sd, (struct sockaddr*)&sv_addr, sizeof(sv_addr));
	if(ret < 0){
		perror("Connect errata!\n");
		exit(1);
	}
	
	bloccato = recvFromServerFlag(sd);
	
	if(bloccato){
		printf("Sei stato bloccato aspetta ancora!\n");
	}else{
		printf("Connessione al server %s (porta %s) effettuata con successo\n\n",argv[1],argv[2]);
		
		memset(&buffer, '\0', sizeof(buffer));
	
		welcome();
	
		while(TRUE){
		
			printf("> ");
		
			scanf("%s", buffer);
			//printf("%s\n", buffer);
			c = AssegnaLettera(buffer);
			//printf("%c\n", c);
			SendToServerCmd(sd, c);
		}
	}	
	
	close(sd);
	return 0;
}
