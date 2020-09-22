#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <signal.h>
#include <math.h>
#include "library.h"

#define USERLOG 100

int main(int argc, char const *argv[]){
	
	struct sockaddr_in my_addr, cl_addr;
	socklen_t addrlen;
	
	//strutture locali di appoggio per la costruzione delle varie liste
	struct user *busers;
	struct ruote *r;
	struct numeri *num;
	struct importi *impo;
	struct estrazioni *ex, *p;
	struct vincita *v;
	struct giocata *g, *gio, *vecchie, *nuove;
	
	int new_fd , ret, fdmax, i, periodo, numero, cont, w, j, k, flag, conteggio;
	double importo, estratto, ambo, terno, quaterna, cinquina;
	bool check, ipbloccato;
    char buffer[1024], c, username[1024], password[1024], id[11], cl_ip[16], ch, ruota[11];
	char cl_sk[16], nb[3];
	
	int n_giorno, giorno, mese, anno, hour, min, sec;
	int ng, day, month, year, ora, minuti, secondi;
	
	int** m;
	char* ex_ruote[11] = {"Bari", "Cagliari", "Firenze", "Genova", "Milano",
						  "Napoli", "Palermo", "Roma", "Torino", "Venezia", "Nazionale"};
			  
    fd_set readfds, masterfds;
	time_t t;
	struct tm *l_time;

	if(argc < 2){
		perror("Specificare la porta!\n");
		exit(1);
	}else if(argc == 2){
		periodo = 5 * 60;
	}else if(argc == 3){
		periodo = atoi(argv[2]) * 60;
	}
	
	FD_ZERO(&masterfds);
	
	c = '\0';
	check = 0;
	ipbloccato = 0;
	n_giorno = 0;
	ng = 0;
	giorno = 0;
	day = 0;
	mese = 0;
	month = 0;
	anno = 0;
	year = 0;
	hour = 0;
	ora = 0;
	min = 0;
	minuti = 0;
	sec = 0;
	secondi = 0;
	
	m = (int**)malloc(11 * sizeof(int*));
	m[0] = (int*)malloc(11 * 5 * sizeof(int));
	for(j = 0; j < 11; j++)
		m[j] = m[0] + j * 5;

	vecchie = 0;
	nuove = 0;
	users = 0;
	vincite = 0;
	ex = 0;

	//printf("[PADRE] 1 - vecchie : %p\n", vecchie);
	
	//Il processo figlio termina forzatamente se anche il processo padre viene terminato forzatamente
	signal(SIGINT, intHandler);
		
	if(pipe(piped) < 0) {
		perror("piped");
		exit(1);
	}

	pid = fork();

	if(pid == -1){
		printf("Impossibile creare processo figlio!\n");
		exit(1);
	}else if(pid == 0){
		//Processo figlio
		close(piped[0]);
		
		while(1){
			
			time(&t);
			l_time = localtime(&t);
			strftime(buffer, 1024, "%u", l_time);
			n_giorno = atoi(buffer);
				
			//L'estrazione verrà effettuata solamente il Martedì, Giovedì e Sabato
			if(n_giorno == 2 || n_giorno == 4 || n_giorno == 6){

				//sleep(periodo);
				
				strftime(buffer, 1024, "%d", l_time);
				giorno = atoi(buffer);
		
				strftime(buffer, 1024, "%m", l_time);
				mese = atoi(buffer);
		
				strftime(buffer, 1024, "%Y", l_time);
				anno = atoi(buffer);
		
				strftime(buffer, 1024, "%H", l_time);
				hour = atoi(buffer);
		
				strftime(buffer, 1024, "%M", l_time);
				min = atoi(buffer);
		
				strftime(buffer, 1024, "%S", l_time);
				sec = atoi(buffer);			
				
				for(j = 0; j < 11; j++){
					for(k = 0; k < 5; k++){
						m[j][k] = rand()%90 + 1;
									
						//Con questo controllo l'estrazione e' senza ripetizione
						//per avere in un estrazione tutti i numeri diversi
						for(w = 0; w < k; w++){
							if(m[j][k] == m[j][w]){
								k--;
								break;
							}
						}
					}
				}
			
				//Creazione del file di estrazione
				createFileEstrazioni(n_giorno, giorno, mese, anno, hour, min, sec, m);
				
				//Invio al processo padre del timestamp e della matrice dell'estrazione
				pipeToFather(piped[1], n_giorno, giorno, mese, anno, hour, min, sec, m);
				
				sleep(periodo);
			}
		}
	}else{
		//Processo padre
		close(piped[1]);
		
		sock = socket(AF_INET, SOCK_STREAM, 0);

		if(sock < 0){
			perror("Socket non creato!\n");
			exit(1);
		}

		memset(&my_addr, 0, sizeof(my_addr));
		my_addr.sin_family = AF_INET;
		my_addr.sin_port = htons(atoi(argv[1]));
		inet_pton(AF_INET, "127.0.0.1", &my_addr.sin_addr);
	
		ret = bind(sock, (struct sockaddr*)&my_addr, sizeof(my_addr));
	
		FD_SET(sock, &masterfds);				//Aggiunta del sock al set
		FD_SET(piped[0], &masterfds);			//Aggiunta del piped[0] al set
		
		fdmax = (piped[0] > sock)?piped[0]:sock;   //Tengo traccia del maggiore

		if(ret < 0){
			perror("[LOG] Bind errata!\n");
			exit(1);
		}else{
			printf("[LOG] Bind effettuata con successo!\n");
		}
	
		ret = listen(sock, USERLOG);

		if(ret < 0){
			perror("[LOG] Listen errata!\n");
			exit(1);
		}else{
			printf("[LOG] Server in ascolto\n");
		}
		
		busers = buildListUsers(&users);
		
		//riempimento dello storico degli user registrati
		openFileRegister(&busers);
		
		//riempimento dello storico delle giocate
		openFileGiocate(&vecchie);
		
		//riempimento dello storico delle vincite
		openFileVincite(&vincite);
		
		//riempimento dello storico delle estrazioni
		openFileEstrazioni(ex, ex_ruote, m);
		
		while(1){
			memset(&buffer, '\0', sizeof(buffer));
			readfds = masterfds;
			select(fdmax+1, &readfds, NULL, NULL, NULL);

			for(i = 0; i <= fdmax; i++){
				
				if(FD_ISSET(i, &readfds)){					//Trovato un descrittore pronto
								
					if(i == sock){
						
						printf("[LOG] Richiesta una nuova connessione\n");
						
						//Far partire la accept
						addrlen = sizeof(cl_addr);
						new_fd = accept(i, (struct sockaddr *)&cl_addr, &addrlen);
					
						memset(&cl_sk, '\0', sizeof(cl_sk));
						memset(&cl_ip, '\0', sizeof(cl_ip));
					
						inet_ntop(AF_INET, &(cl_addr.sin_addr), cl_sk, INET_ADDRSTRLEN);

				        	//apertura del file degli utenti bloccati
						ipbloccato = openFileBlocked(cl_sk);					
						
						//Inviare messaggio di errore al client (mettere un flag se trovo l'ip)
						sendToClientFlag(new_fd, ipbloccato);
					
						if(ipbloccato){
							close(new_fd);
						}else{
							
							//Sblocco del client nel caso di blocco precedente e poi scaduto
							UnlockClient(cl_sk, busers);
							
							FD_SET(new_fd, &masterfds);			//Aggiungo il nuovo socket	
							if(new_fd == -1){
								printf("[LOG] Impossibile creare una nuova socket\n");
								continue;
							}
					
							printf("[LOG] La connessione richiesta e' stata confermata\n");
					
							if(new_fd > fdmax){
								fdmax = new_fd;					//Aggiorno il massimo
							}
						}
						
					}else if(i == piped[0]){
						
						//Lettura dalla pipe del nome del giorno inviato dal processo figlio
						read(piped[0], &n_giorno, sizeof(n_giorno));
						
						if(n_giorno == 2 || n_giorno == 4 || n_giorno == 6){	
							read(piped[0], &giorno, sizeof(giorno));
							read(piped[0], &mese, sizeof(mese));
							read(piped[0], &anno, sizeof(anno));
							read(piped[0], &hour, sizeof(hour));
							read(piped[0], &min, sizeof(min));
							read(piped[0], &sec, sizeof(sec));
					
							for(j = 0; j < 11; j++){
								for(k = 0; k < 5; k++){
									read(piped[0], &m[j][k], sizeof(m[j][k]));
								}
							}
						
							//Inserimento dell'estrazione avvenuta nella lista delle estrazioni
							fillListaEstrazioni(&ex, n_giorno, giorno, mese, anno, hour, min, sec, ex_ruote, m);
							
							//Assegnamento della giocata
							//Passaggio dalle giocate nuove alle giocate vecchie al momento dell'estrazione
							AssegnaGiocata(&vecchie, &vincite, nuove, ex_ruote, m, n_giorno, giorno, mese, anno, hour, min, sec);

							//deallocazione della struttura nuove
							g = nuove;
							
							while(g){
								gio = g;
								g = g->next;
								free(gio);
							}
							nuove = 0;
						}						
					}else{		
					
						r = 0;
						num = 0;
						impo = 0;
						
						c = recvFromClientChar(i);
						
						RichiestaFromClientCmd(c);			//in base alla lettera inviata dal client il server stampa a video la richiesta effettuata				
						
						switch(c){
							case 's' : 
								// Comando !signup
							
								//Il server riceve dal client l'username per la registrazione
								recvFromClient(i, buffer);          
								strcpy(username, buffer);
								
								//Controllo sulla validità dell'username
								check = checkUsername(&busers, username);
					
								//Il server riceve dal client la password per la registrazione
								recvFromClient(i, buffer);			
								strcpy(password, buffer);
								
								//Invio del flag check
								sendToClientFlag(i, check);
					
								if(!check){
									//L'username è valido quindi il server avendo già inviato il flag al client notifica l'avvenuta registrazione
									//Il server salva username e password nella lista degli utenti registrati, inoltre salva le credenziali nel file apposito
									registerUsers(cl_sk, &busers, username, password, 1);
								}else{
									//L'username non è valido quindi il server avendo già inviato notifica al client che l'username inserito è già presente
									printf("[LOG] L'username inviato e' gia' in uso\n");
								}
								break;
							case 'l' :
								// Comando !login
								
								//Il server riceve dal client l'username per il login
								recvFromClient(i, buffer);         
								strcpy(username, buffer);
								
								//Il server riceve dal client la password per il login
								recvFromClient(i, buffer);		
								strcpy(password, buffer);
								
								//Controllo sulla validità dell'username e della password
								check = checkUsPsw(&users, username, password);
								
								//Invio del flag check
								sendToClientFlag(i, check);
							
								if(check){
									memset(&id, '\0', sizeof(id));
									//Il client ha inserito i dati di login corretti quindi il server gli invia l'ID di sessione
									loginUsers(i, &users, username, password, id);
								}else{
									printf("[LOG] Login fallito.\n Decremento di tentativi.\n");
									//Se il client effettua 3 tentativi sbagliati, l'ip del client verrà bloccato per 30 minuti
									blockUser(i, users, cl_sk);
								}
								break;
							case 'i' :
								// Comando !invia_giocata 

								memset(&buffer, '\0', sizeof(buffer));
							
								do{
									recvFromClient(i, buffer);				//Al server arriva una ruota
									if((strcmp(buffer, "-n")) != 0){
										//Inserimento nella lista delle ruote
										strcpy(ruota, buffer);
										fillListaRuote(&r, ruota);
									}
								
								}while((strcmp(buffer, "-n")) != 0);
								
								do{
									recvFromClient(i, buffer);				//Al server arriva un numero
									//Se il client non inserisce un comando che non sia -n e -i continua a inserire numeri
									//Il controllo del comando sbagliato lo farà direttamente il client prima di inviare 
									if(((strcmp(buffer, "-i")) != 0) && ((strcmp(buffer, "-n")) != 0)){
										//Inserimento nella lista dei numeri
										numero = atoi(buffer);
										fillListaNumeri(&num, numero);
									}
								}while((strcmp(buffer, "-i")) != 0);
							
								cont = 1;
							
								do{	
									recvFromClient(i, buffer);				//gli arriva un importo
									//Il client invierà una stringa 'fine' al server per notificargli
									//che ha premuto invio ed gli ha inviato il comando
									//Se non riceve tale stringa continua a inserire gli importi nella lista
									if((strcmp(buffer, "fine")) != 0){
										//Inserimento nella lista degli importi
										importo = atof(buffer);
										fillListaImporti(&impo, importo, cont);
										
										//Ad ogni inserimento si ottiene il valore numerico del tipo dell'importo
										cont++;
									}
								}while((strcmp(buffer, "fine")) != 0);
								
								memset(&buffer, '\0', sizeof(buffer));
							
								//Current timestamp 
								time(&t);
								l_time = localtime(&t);
								strftime(buffer, 1024, "%u", l_time);
								ng = atoi(buffer);
								strftime(buffer, 1024, "%d", l_time);
								day = atoi(buffer);
								strftime(buffer, 1024, "%m", l_time);
								month = atoi(buffer);
								strftime(buffer, 1024, "%Y", l_time);
								year = atoi(buffer);
								strftime(buffer, 1024, "%H", l_time);
								ora = atoi(buffer);
								strftime(buffer, 1024, "%M", l_time);
								minuti = atoi(buffer);
								strftime(buffer, 1024, "%S", l_time);
								secondi = atoi(buffer);
						
								//Inserimento della giocata, inviata dal client, nella lista con il relativo timestamp di invio
								registerGiocata(&nuove, username, ng, day, month, year, ora, minuti, secondi, r, num, impo, 0);
								
								//Notifica di avvenuta registrazione della giocata al client 
								sendToClientFlag(i, 1);
							
								break;
							case 'g' :
								//Comando !vedi_giocate
								
								memset(&buffer, '\0', sizeof(buffer));
								
								flag = recvFromInt(i);
								
								j = 0;
								
								//In base al flag inviato con il comando dal client
								//si effetuerà il conteggio degli elementi presenti nella lista
								//0 -> vecchie
								//1 -> nuove

								if(!flag){
									g = vecchie;
									//printf("[PADRE] 3 - vecchie : %p\n", vecchie);
									while(g){
										if(!strcmp(g->username, username)){
											j++;
										}
										g = g->next;
									}
								}else{
									g = nuove;
									while(g){
										if(!strcmp(g->username, username)){
											j++;
										}
										g = g->next;
									}
								}
								
								//Invio della quantià delle giocate al client
								sendToInt(i, j);
								
								//giocate passate
								if(!flag){
									g = vecchie;
									//printf("[PADRE] 4 - vecchie : %p\n", vecchie);
									while(g){
										//scorro la lista delle giocate
										//in base all'username 
										if(!strcmp(g->username, username)){
											//Invio delle giocate vecchie
											sendToClientGiocate(i, g);
										}
										g = g->next;
									}
								}else{
									g = nuove;
									while(g){
										//scorro la lista delle giocate
										//in base all'username 
										if(!strcmp(g->username, username)){
											//Invio delle giocate nuove
											sendToClientGiocate(i, g);
										}
										g = g->next;
									}
								}						
							
								break;
							case 'e' :
								//Comando !vedi_estrazione
						
								p = ex;
								
								memset(&buffer, '\0', sizeof(buffer));
								memset(&nb, '\0', sizeof(nb));
								memset(&ruota, '\0', sizeof(ruota));
								
								//Riceve dal client un carattere che notifica al server se è stato 'inviato il comando' senza l'opzione della ruota
								ch = recvFromClientChar(i);
								
								recvFromClient(i, buffer);								//gli arriva n
								
								numero = atoi(buffer);									//conversione strint to int
								
								cont = 0;
								
								while(p){
									//in cont è presente il valore delle estrazioni effettuate dal server
									//dall'accensione ad ora in cui sono passati periodo minuti tra l'una e l'altra
									cont++;				
									p = p->next;
								}
								
								//il server invia il numero delle estrazioni presenti in questo momento al client
								sendToInt(i, cont);
								
								p = ex;
								w = 0;
								
								//se ch è diverso da \n mi aspetto di ricevere la ruota dal client
								//e allo stesso tempo il server deve inviare le n estrazioni
								//di quella ruota al client altrimenti tutte le estrazioni
								if(ch != '\n'){
									
									recvFromClient(i, buffer);
									strcpy(ruota, buffer);
									
									if(numero >= cont){
										//scorrendo la lista ne stampo cont
										while(p){
											for(j = 0; j < 11; j++){
												//invio solo quei numeri della ruota inviata dal client
												if(!strcmp(p->nome_ruote[j], ruota)){
													for(k = 0; k < 5; k++){
														//invio i numeri
														sendToInt(i, p->g_vincenti[j][k]);
													}
												}
											}
											p = p->next;
										}
									}else{
										while(p && w < numero){
											for(j = 0; j < 11; j++){
												//invio solo quei numeri della ruota inviata dal client
												if(!strcmp(p->nome_ruote[j], ruota)){
													for(k = 0; k < 5; k++){
														//invio i numeri
														sendToInt(i, p->g_vincenti[j][k]);
													}
												}
											}
											w++;
											p = p->next;
										}
									}
								}else{
									if(numero >= cont){
										
										//scorrendo la lista ne stampo cont
										while(p){
											for(j = 0; j < 11; j++){
												//invio la ruota
												sendTo(i, p->nome_ruote[j]);
												for(k = 0; k < 5; k++){
													//invio i numeri
													sendToInt(i, p->g_vincenti[j][k]);
												}
											}
											p = p->next;
										}
									}else{
										
										while(p && w < numero){
											for(j = 0; j < 11; j++){
												//invio la ruota
												sendTo(i, p->nome_ruote[j]);
												for(k = 0; k < 5; k++){
													//invio i numeri
													sendToInt(i, p->g_vincenti[j][k]);
												}
											}
											w++;
											p = p->next;
										}
									}
								}
								
								break;
							case 'v' :
								//Comando !vedi_vincite
								
								v = vincite;
								conteggio = 0;
								
								//Conteggio del numero delle vincite
								while(v){
									if(!strcmp(v->username, username))
										conteggio++;
									v = v->next;
								}
								
								sendToInt(i, conteggio);
								
								conteggio = 0;
								v = vincite;
							
								while(v){
									if(!strcmp(v->username, username)){
										
										//Invio del timestamp, dell'estrazione in cui è avvenuta la vincita, al client
										sendToInt(i, v->giorno);
										sendToInt(i, v->mese);
										sendToInt(i, v->anno);
										sendToInt(i, v->ora);
										sendToInt(i, v->min);
										
										//Invio della ruota
										sendTo(i, v->nome_ruota);
										
										for(num = v->num; num; num = num->next){
											conteggio++;
										}
										
										//Invio quantita' numeri
										sendToInt(i, conteggio);
										
										//Invio dei numeri
										for(num = v->num; num; num = num->next){
											sendToInt(i, num->numero);
										}
										
										conteggio = 0;
										for(impo = v->impo; impo; impo = impo->next){
											if(impo->importo > 0.0){
												conteggio++;
											}
										}
										
										sendToInt(i, conteggio);
										
										conteggio = 0;
										
										for(impo = v->impo; impo; impo = impo->next){
											if(impo->importo > 0.0){
												sendToInt(i, impo->tipo);
												gcvt(impo->importo, 11, buffer);
												sendTo(i, buffer);
											}
										}
									}
									v = v->next;
								}
									
								//Conteggio delle vincite a carico dell'utente loggato precedentemente					
								conteggio = 0;
								for(v = vincite; v; v = v->next){
									if(!strcmp(v->username, username)){
										conteggio++;
									}
								}
								
								sendToInt(i, conteggio);
								
								/*

								//Il client riceve altri valori da quelli inviati dal server
								//Solo in questo caso la send e la receive non funzionano correttamente

								if(conteggio > 0){
									
									v = vincite;
									estratto = 0.0;
									ambo = 0.0;
									terno = 0.0;
									quaterna = 0.0;
									cinquina = 0.0;
									
									//Invio delle somme vinte per ogni tipo
									while(v){
										if(!strcmp(v->username, username)){
											for(impo = v->impo; impo; impo = impo->next){
												if(impo->tipo == 1){
													estratto += impo->importo;
												}else if(impo->tipo == 2){
													ambo += impo->importo;
												}else if(impo->tipo == 3){
													terno += impo->importo;
												}else if(impo->tipo == 4){
													quaterna += impo->importo;
												}else if(impo->tipo == 5){
													cinquina += impo->importo;
												}
											}
										}
										v = v->next;
									}

									memset(&buffer, '\0', sizeof(buffer));
									
									//Conversione e invio delle variabili somma calcolate sopra					
									if(estratto > 0)
										gcvt(estratto, 8, buffer);
									else
										strcpy(buffer, "0.000000");
									sendTo(i, buffer);
		
									memset(&buffer, '\0', sizeof(buffer));

									if(ambo > 0)
										gcvt(ambo, 8, buffer);
									else
										strcpy(buffer, "0.000000");					
									sendTo(i, buffer);

									memset(&buffer, '\0', sizeof(buffer));

									if(terno > 0)
										gcvt(terno, 8, buffer);
									else
										strcpy(buffer, "0.000000");
									sendTo(i, buffer);

									memset(&buffer, '\0', sizeof(buffer));
									
									if(quaterna > 0)
										gcvt(quaterna, 8, buffer);
									else
										strcpy(buffer, "0.000000");
									sendTo(i, buffer);

									memset(&buffer, '\0', sizeof(buffer));
									
									if(cinquina > 0)
										gcvt(cinquina, 8, buffer);
									else
										strcpy(buffer, "0.000000");
									sendTo(i, buffer);
	
									memset(&buffer, '\0', sizeof(buffer));
								}

								*/
								break;
							case 'q' :
								//Comando !esci
								check = logoutUsers(&users, username);
								sendToClientFlag(i, check);
								if(check){
									close(i);
								}
								break;
							default :
								printf("Ricevuto comando errato!\n");
								break;
						}//fine switch
					} //fine else (i == sock)
				}//fine FD_ISSET
			}//fine for(i = 0; i <= fdmax; i++)
		}
		close(sock);
		close(piped[1]);
	}	
	return 0;
}
