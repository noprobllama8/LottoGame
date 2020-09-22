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

/*******************************************************************/
//Funzione di invio del buffer sia da client a server che viceversa
void sendTo(int sd, char *buffer){
	
	int len, ret;
	char dim[2];

	//printf("[sendTo] buffer - %s\n", buffer);
	
	len = strlen(buffer)+1;					//lunghezza del buffer

	//printf("[sendTo] len - %d\n", len);
	
	memset(&dim, '\0', sizeof(dim));
	sprintf(dim, "%d", len);            	//conversione int to string

	//printf("[sendTo] dim - %s\n", dim);
	
	ret = send(sd, (void*)dim, 2, 0);
	if(ret < 0){
		perror("[LOG] Send errata della lunghezza nella funzione sendTo!\n");
		exit(1);
	}
	
	ret = send(sd, (void*)buffer, len, 0);
	if(ret < 0){
		perror("[LOG] Send errata della stringa nella funzione sendTo!\n");
		exit(1);
	}
}

//Funzione per l'invio di un intero sia da client a server che viceversa
void sendToInt(int sd, int i){
	int ret;
	char dim[4];
	
	memset(&dim, '\0', sizeof(dim));
	
	sprintf(dim, "%d", i);

	ret = send(sd, (void*)dim, 4, 0);
	if(ret < 0){
		perror("[LOG] Send errata dell'intero nella funzione sendToInt!\n");
		exit(1);
	}
}

//Funzione per la ricezione di un intero sia da client a server che viceversa
int recvFromInt(int sd){
	int ret, i;
	char dim[4];
	
	memset(&dim, '\0', sizeof(dim));
	
	ret = recv(sd, (void*)dim, 4, 0);
	if(ret < 0){
		perror("[LOG] Recv errata dell'intero di recvFromInt!\n");
		exit(1);
	}else{
		//Conversione string to int
		i = atoi(dim);
	}
	return i;
}
/*********************************************************************/

/**************************Funzioni Server****************************/
//Funzione per gestire la terminazione anche del processo figlio
//ad ogni terminazione del processo padre nel main
void intHandler(int sign) {
	//Chiusura del canale pipe di scrittura 
	close(piped[1]);
	//Chiusura del socket
	close(sock);
	//Terminazione del processo figlio quando si lancia il comando 
	//CTRL+C a riga di comando per terminare il processo padre
	kill(pid, SIGTERM);
}

//Funzione di invio al client del flag di controllo 
void sendToClientFlag(int sd, bool flag){
	int ret;
	char dim[2], charboolean;
	
	memset(&dim, '\0', sizeof(dim));
	
	if(flag){
		charboolean = '1';
	}else{
		charboolean = '0';
	}
	
	dim[0] = charboolean;

	ret = send(sd, (void*)dim, 2, 0);
	if(ret < 0){
		perror("[LOG] Send errata di sendToClientFlag!\n");
		exit(1);
	}
}

//Funzione di invio al client dell'intera giocata effettuata
void sendToClientGiocate(int sd, struct giocata *g){
	
	int j = 0;
	char buffer[1024];
	
	struct ruote *r;
	struct numeri *num;
	struct importi *impo;
	
	r = g->r;
	num = g->num;
	impo = g->impo;
	
	memset(&buffer, '\0', sizeof(buffer));
	
	//Scorrimento della lista delle ruote per contare il numero di elementi
	while(r){
		j++;
		r = r->next;
	}
											
	//Invio del numero delle ruote al client
	sendToInt(sd, j);
	
	r = g->r;
											
	//Invio delle ruote al client
	while(r){
		sendTo(sd, r->nome_ruota);
		r = r->next;
	}
							
	j = 0;
		
	//Scorrimento della lista dei numeri per contare il numero di elementi
	while(num){
		j++;
		num = num->next;
	}
											
	//Invio del numero dei numeri
	sendToInt(sd, j);
								
	j = 0;
	num = g->num;
											
	//Invio dei numeri al client
	while(num){
		sendToInt(sd, num->numero);
		num = num->next;
	}
	
	//Scorrimento della lista degli importi per contare il numero di elementi	
	while(impo){
		j++;
		impo = impo->next;
	}
										
	//Invio del numero di importi
	sendToInt(sd, j);
	
	impo = g->impo;
											
	//Invio degli importi
	while(impo){
		gcvt(impo->importo, 5, buffer);
		//invio l'importo
		sendTo(sd, buffer);
				
		//Invio della stringa corrispondente al tipo intero
		//1 -> estratto 2 -> ambo etc. 
		if(impo->tipo == 1){
			sendTo(sd, "estratto");
		}else if(impo->tipo == 2){
			sendTo(sd, "ambo");
		}else if(impo->tipo == 3){
			sendTo(sd, "terno");
		}else if(impo->tipo == 4){
			sendTo(sd, "quaterna");
		}else if(impo->tipo == 5){
			sendTo(sd, "cinquina");
		}										
		impo = impo->next;
	}
}

//Funzione di ricezione del server dal client di un carattere corrispondente al comando
char recvFromClientChar(int sd){
	char dim[2], c;
	int ret;
	
	memset(&dim, '\0', sizeof(dim));
	
	ret = recv(sd, (void*)dim, 2, 0);
	
	if(ret < 0){
		perror("[LOG] Recv errata di recvFromClientChar!\n");
		exit(1);
	}else{
		c = dim[0];
	}

	return c;
}

//Funzione di ricezione del server dal client
void recvFromClient(int sd, char *buffer){
	int ret, len;
	char dim[2];
	
	memset(&dim, '\0', sizeof(dim));
	
	ret = recv(sd, (void*)dim, 2, 0);
	len = atoi(dim);
	if(ret < 0){
		perror("[LOG] Recv errata della lunghezza della stringa di recvFromClient!\n");
		exit(1);
	}
	
	ret = recv(sd, (void*)buffer, len, 0);
	if(ret < 0){
		perror("[LOG] Recv errata della stringa di recvFromClient!\n");
		exit(1);
	}
}

//Funzione di controllo dell'username al momento della registrazione
bool checkUsername(struct user **p0, char *username){
	struct user *p;
	
	for(p = *p0; p; p = p->next){
		//Controllo se l'username inserito è già inserito
		if(!strcmp(p->username, username))
			return 1;
	}
	
	return 0;
}

//Funzione di controllo dell'username e della password al momento del login
bool checkUsPsw(struct user **p0, char *username, char *password){
	struct user *p;

	for(p = *p0; p; p = p->next){
		//Controllo sull'username inserito al momento del login 
		if(!strcmp(p->username, username)){
			//Controllo sulla password inserita al momento del login
			if(!strcmp(p->password, password)){
				return 1;
			}
		}
	}
	
	return 0;
}

//Funzione per la costruzione della lista degli user
struct user* buildListUsers(struct user **p0){
	struct user *user = (struct user*)malloc(sizeof(struct user));
	struct user *p;
		
	if(!user)
        return NULL;

    user->next = NULL;

    if(!*p0){
        *p0 = user;
	}
    else {
        p = *p0;
        while(p->next != NULL) {
            p = p->next;
        }
        p->next = user;
    }
	return user;
}

/*Funzione per la generazione di una stringa alfanumerica
di 10 caratteri per l'id session del client che effettua
il login*/
void generaIDsession(char *sessionId){
	int i;
	
	//0-9 = 48-57 , a-z = 97-122
	char str[36] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 
				  'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 
				  'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 
				  'u', 'v', 'w', 'x', 'y', 'z'}; 
	
	//Composizione random della stringa alfanumerica 
	for(i = 0; i < 10; i++){
		sessionId[i] = str[rand() % 35];
	}
}

//Funzione per il login dell'user
void loginUsers(int sd, struct user **p0, char *username, char *password, char *id){
	bool check;
	struct user *p;
	
	check = 0;
		
	for(p = *p0; p; p = p->next){											//scorro la lista
		if(!strcmp(p->username, username)){									//se il campo username all'interno dell'elemento della lista è uguale all'username passato come parametro
			if(!strcmp(p->password, password)){								//e se il campo password all'interno dell'elemento della lista è uguale alla password passata come parametro
				if(p->online == 0){											//e se il campo online all'interno dell'elemento della lista è ancora a false
					if(p->blocked == 0){									//e se il campo bloccato all'interno dell'elemento della lista è ancora a false
						generaIDsession(id);								//genero l'id session
						strcpy(p->sessionId, id);							//lo copio all'interno del campo id dell'elemento della lista
						p->online = 1;										//pongo il campo online dell'elemento corrispondente a true
						printf("[LOG] Utente %s ha effettuato il login. E' online.\n", p->username);
						check = 1;
					}
				}
			}
		}
	}
	
	sendToClientFlag(sd, check);
	
	if(check){
		sendTo(sd, id);		//invio del session id al client
	}else{
		sendTo(sd, "Hai gia' effettuato il login.\n");
	}
}

//Funzione per il logout dell'user
bool logoutUsers(struct user **p0, char *username){
	struct user *p;
		
	for(p = *p0; p; p = p->next){										//scorro la lista
		if(!strcmp(p->username, username)){								//se il campo username all'interno dell'elemento della lista è uguale all'username passato come parametro
			if(p->online == 1){										    //e se il campo online all'interno dell'elemento della lista è ancora a false
				if(p->sessionId[0] != '\0'){
					if(p->blocked == 0){								//e se il campo bloccato all'interno dell'elemento della lista è ancora a false
						memset(&p->sessionId, '\0', sizeof(p->sessionId));
						p->online = 0;									//pongo il campo online dell'elemento corrispondente a true
						printf("[LOG] L'utente %s ha effettuato il logout, E' offline.\n", p->username);
						return 1;
					}
				}
			}
		}
	}
	
	return 0;
}

//funzione di creazione del file registro
void createFileRegister(struct user *p0){
		
	FILE *file = fopen("FileRegistri/UsersRegister.txt", "a");
	if(file == NULL){
		printf("[LOG] Impossibile creare il file!\n");
		exit(1);		
	}
	
	fprintf(file, "%s\n%s\n%s\n", p0->cl_ip, p0->username, p0->password);
	
	printf("[LOG] Un nuovo utente è stato registrato\n");
	
	fclose(file);
}

//funzione di creazione del file registro degli utenti bloccati
void createFileBlocked(char *cl_addr, time_t seconds){
	
	FILE *file = fopen("FileRegistri/ClientBlocked.txt", "a");
	if(file == NULL){
		printf("[LOG] Impossibile creare il file!\n");
		exit(1);
	}
	
	fprintf(file, "%s\n%ld\n", cl_addr, seconds);	
	
	printf("[LOG] Un utente e' stato bloccato\n");
	
	fclose(file);
}

//funcione di creazione del file giocate
void createFileGiocate(struct giocata *g0){

	int cont;
	
	FILE *file = fopen("FileRegistri/UsersGiocate.txt", "a");
	if(file == NULL){
		printf("[LOG] Impossibile creare il file!\n");
		exit(1);
	}
	
	struct giocata *p;
	struct ruote *q;
	struct numeri *s;
	struct importi *t;

	for(p = g0; p != NULL; p = p->next){
		fprintf(file, "%s\n%d %d %d %d %d %d %d\n", p->username, p->nome_giorno, p->giorno, p->mese, p->anno, p->ora, p->min, p->sec);
		
		//conteggio preliminarmente il numero delle ruote
		cont = 0;
		for(q = g0->r; q != NULL; q = q->next){
			cont++;
		}
		//lo stampo sul file
		fprintf(file, "%d\n", cont);
		for(q = g0->r; q != NULL; q = q->next){
			fprintf(file, "%s\n", q->nome_ruota);
		}
		//conteggio preliminarmente il numero dei numeri
		cont = 0;
		for(s = g0->num; s != NULL; s = s->next){
			cont++;
		}
		//lo stampo sul file
		fprintf(file, "%d\n", cont);
		for(s = g0->num; s != NULL; s = s->next){
			fprintf(file, "%d\n", s->numero);
		}
		//conteggio preliminarmente il numero degli importi
		cont = 0;
		for(t = g0->impo; t != NULL; t = t->next){
			cont++;
		}
		//lo stampo sul file
		fprintf(file, "%d\n", cont);
		for(t = g0->impo; t != NULL ; t = t->next){
			fprintf(file, "%f\n", t->importo);
		}
	}
		
	printf("[LOG] La giocata è stata archiviata\n");
	
	fclose(file);
}

//funcione di creazione del file vincite
void createFileVincite(struct vincita *v0){
	int cont;
	
	FILE *file = fopen("FileRegistri/UsersVincite.txt", "a");
	if(file == NULL){
		printf("Impossibile creare il file!\n");
		exit(1);
	}
	
	struct vincita *p;
	struct numeri *s;
	struct importi *t;

	for(p = v0; p; p = p->next){
		fprintf(file, "%s\n%d %d %d %d %d\n", p->username, p->giorno, p->mese, p->anno, p->ora, p->min);
		
		fprintf(file, "%s\n", p->nome_ruota);

		//conteggio preliminarmente il numero dei numeri
		cont = 0;
		for(s = v0->num; s; s = s->next){
			cont++;
		}
		//lo stampo sul file
		fprintf(file, "%d\n", cont);
		for(s = v0->num; s; s = s->next){
			fprintf(file, "%d\n", s->numero);
		}
		//conteggio preliminarmente il numero degli importi
		cont = 0;
		for(t = v0->impo; t; t = t->next){
			cont++;
		}
		//lo stampo sul file
		fprintf(file, "%d\n", cont);
		for(t = v0->impo; t; t = t->next){
			fprintf(file, "%f\n", t->importo);
		}
	}
		
	printf("[LOG] Nuova vincita avvenuta\n");
	
	fclose(file);
}

//Funzione di creazione del file estrazioni
void createFileEstrazioni(int n_giorno, int giorno, int mese, int anno, int hour, int min, int sec, int **m){
	int i, j;
	
	FILE *file = fopen("FileRegistri/Estrazioni.txt", "a");
	if(file == NULL){
		printf("[LOG] Impossibile creare il file!\n");
		exit(1);
	}
	
	
	fprintf(file, "%d %d %d %d %d %d %d\n", n_giorno, giorno, mese, anno, hour, min, sec);
	
	for(i = 0; i < 11; i++){
		for(j = 0; j < 5; j++){
			fprintf(file, "%d ", m[i][j]);;
		}
		fprintf(file, "\n");
	}
	
	printf("[LOG] Nuova estrazione avvenuta\n");
	
	fclose(file);
}

//Funzione della registrazione della giocata e riempimento della lista giocate
void registerGiocata(struct giocata **g0, char *username, int n_giorno, int giorno, int mese, int anno, int hour, int min, int sec, struct ruote *r0, struct numeri *n0, struct importi *i0, bool mode){
	
	struct giocata *q, *p;
	
	for(q = *g0; q != NULL; q = q->next){
		p = q;
	}
	
	q = (struct giocata*)malloc(sizeof(struct giocata));
	//Non è stato possibile allocare l'area di memoria per q
	if(!q){
		return;
	}	
	
	strcpy(q->username, username);
	
	q->nome_giorno = n_giorno;
	q->giorno = giorno;
	q->mese = mese;
	q->anno = anno;
	q->ora = hour;
	q->min = min;
	q->sec = sec;
	
	q->r = r0;
	q->num = n0;
	q->impo = i0;
	
	q->next = NULL;

	if(*g0 == 0){
		*g0 = q;
	}else{
		p->next = q;
	}
	
	if(mode)
		createFileGiocate(q);

	//free(q);
}

//Funzione per la registrazione del client
void registerUsers(char *cl_addr, struct user **p0, char *username, char *password, bool stato){
	struct user *q, *p;
	
	for(q = *p0; q; q = q->next)
		p = q;
	
	q = (struct user*)malloc(sizeof(struct user));
	
	memset(&((q)->sessionId), '\0', sizeof((q)->sessionId));
	//Al momento della registrazione memorizzo l'ip che servira'
	//al momento del login per un eventuale blocco
	strcpy((q)->cl_ip, cl_addr);
	strcpy((q)->username, username);
	strcpy((q)->password, password);
	
	(q)->tentativi = 3;
	(q)->online = 0;
	(q)->blocked = 0;
	(q)->next = NULL;

	if(*p0 == 0)
		*p0 = q;
	else
		p->next = q;
	
	if(stato){
		createFileRegister(q);
		printf("[LOG] L'utente %s si e' registrato\n", q->username);
	}

	//free(q);
}

//Funzione per la registrazione della vincita del dato user
void registerVincita(struct vincita **v0, char *username, int giorno, int mese, int anno, int hour, int min, char *ruota, struct numeri *n0, struct importi *i0, bool mode){
	struct vincita *q, *p;
	/*struct numeri *num, *nrem;
	struct importi *impo, *irem;*/
	
	for(q = *v0; q != 0; q = q->next)
		p = q;
	
	q = (struct vincita*)malloc(sizeof(struct vincita));
	
	strcpy(q->username, username);
	
	//Timestamp di estrazione
	q->giorno = giorno;
	q->mese = mese;
	q->anno = anno;
	q->ora = hour;
	q->min = min;

	strcpy(q->nome_ruota, ruota);
	q->num = (struct numeri*)malloc(sizeof(struct numeri));
	q->num = n0;
	q->impo = (struct importi*)malloc(sizeof(struct importi));
	q->impo = i0;
	q->next = NULL;

	if(*v0 == 0)
		*v0 = q;
	else
		p->next = q;
	
	if(mode)
		createFileVincite(q);

	//deallocazione
	/*num = q->num;
	
	while(num){
		nrem = num;
		num = num->next;
		free(nrem);
	}
	q->num = 0;

	impo = q->impo;
	while(impo){
		irem = impo;
		impo = impo->next;
		free(irem);
	}
	q->impo = 0;

	free(q);*/
}

//Funzione di riempimento della lista delle ruote
void fillListaRuote(struct ruote **r0, char *ruota){
	struct ruote *q, *p;
	
	for(q = *r0; q != 0; q = q->next)
		p = q;
		
	q = (struct ruote*)malloc(sizeof(struct ruote));
	
	strcpy(q->nome_ruota, ruota);
	q->next = NULL;

	if(*r0 == 0)
		*r0 = q;
	else
		p->next = q;

	//free(q);
}

//Funzione di riempimento della lista dei numeri
void fillListaNumeri(struct numeri **n0, int numero){
	struct numeri *q, *p;
	
	for(q = *n0; q != 0; q = q->next)
		p = q;
		
	q = (struct numeri*)malloc(sizeof(struct numeri));
	
	q->numero = numero;
	q->next = NULL;

	if(*n0 == 0)
		*n0 = q;
	else
		p->next = q;

	//free(q);
}

//Funzione di riempimento della lista dei numeri
void fillListaImporti(struct importi **i0, double importo, int tipo){
	struct importi *q, *p;
	
	for(q = *i0; q; q = q->next)
		p = q;
		
	q = (struct importi*)malloc(sizeof(struct importi));
	
	q->importo = importo;
	q->tipo = tipo;
	q->next = NULL;

	if(*i0 == 0)
		*i0 = q;
	else
		p->next = q;

	//free(q);
}

//Funzione di riempimento della lista delle estrazioni
void fillListaEstrazioni(struct estrazioni **e0, int n_giorno, int giorno, int mese, int anno, int hour, int min, int sec, char **ruota, int **m){
	struct estrazioni *q, *p;
	int i, j;
	
	for(q = *e0; q != 0; q = q->next)
		p = q;
		
	q = (struct estrazioni*)malloc(sizeof(struct estrazioni));
	
	q->nome_giorno = n_giorno;
	q->giorno = giorno;
	q->mese = mese;
	q->anno = anno;
	q->ora = hour;
	q->min = min;
	q->sec = sec;
	
	for(i = 0; i < 11; i++){
		q->nome_ruote[i] = ruota[i];
		for(j = 0; j < 5; j++){
			q->g_vincenti[i][j] = m[i][j];
		}
	}
	
	q->next = NULL;

	if(*e0 == 0)
		*e0 = q;
	else
		p->next = q;

	//free(q);
}

//Funzione di log delle richieste del client
void RichiestaFromClientCmd(char c){	
	switch(c){
		case 'h' :
			break;
		case 's' :
			printf("[LOG] Richiesta di registrazione\n");
			break;
		case 'l' :
			printf("[LOG] Richiesta di login\n");
			break;
		case 'i' :
			printf("[LOG] Richiesta di invia la giocata\n");
			break;
		case 'g' :
			printf("[LOG] Richiesta di visualizzazione delle giocate effettuate\n");
			break;
		case 'e' :
			printf("[LOG] Richiesta di visualizzazione dell'estrazione\n");
			break;
		case 'v' :
			printf("[LOG] Richiesta di visualizzazione delle vincite effettuate\n");
			break;
		case 'q' :
			printf("[LOG] Richiesta di logout\n");
			break;
		default :
			printf("[LOG] Richiesta errata!\n");
			break;
	}
}

//Funzione che calcola il fattoriale di un numero
int fatt(int n){
	if(n == 0)
		return 1;
	return n * fatt(n-1);
}

//Funzione che calcola le combinazioni senza ripetizioni di n su k numeri
double combinazioniNoRep(int n, int k){
	
	double combNoRip = 0;
	
	int fattN = fatt(n);
	int fattK = fatt(k);
	int fattNmenoK = fatt(n-k);
	combNoRip = (double) (fattN) / (fattK * fattNmenoK);
	
	return combNoRip;
}

//Funzione per l'invio da parte del processo figlio, attraverso la pipe, dell'estrazione al processo padre
void pipeToFather(int piped, int n_giorno, int giorno, int mese, int anno, int hour, int min, int sec, int **m){
	int i, j;
	
	write(piped, &n_giorno, sizeof(n_giorno));
	write(piped, &giorno, sizeof(giorno));
	write(piped, &mese, sizeof(mese));
	write(piped, &anno, sizeof(anno));
	write(piped, &hour, sizeof(hour));
	write(piped, &min, sizeof(min));
	write(piped, &sec, sizeof(sec));
	
	for(i = 0; i < 11; i++){
		for(j = 0; j < 5; j++){
			write(piped, &m[i][j], sizeof(m[i][j]));
		}
	}
}

//Funzione per la registrazione della vincita
void AssegnaVincita(struct giocata **vecchie, struct vincita **vincite, char **ruota, int **m){
	
	struct giocata *g;
	struct ruote *r;
	struct numeri *num0, *num;
	struct importi *impo0, *impo;
	int contaN, j, k, cnum;
	double importo, comb;

	g = *vecchie;
	while(g->next != NULL){
		g = g->next;	
	}
	
	for(r = g->r; r; r = r->next){

		contaN = 0;
		cnum = 0;
		num0 = 0;
		impo0 = 0;
			
		//Ricavo dell'indice j così da sapere la riga della matrice dei numeri vincenti
		for(j = 0; j < 11; j++){
			if(!strcmp(r->nome_ruota, ruota[j])){
				break;
			}
		}

		//Conteggio dei numeri giocati che sono vincenti
		for(num = g->num; num != 0; num = num->next){
			for(k = 0; k < 5; k++){
				if(num->numero == m[j][k]){
					contaN++;
				}
			}												
		}
		
		if(contaN > 0){
			for(num = g->num; num != 0; num = num->next){
				for(k = 0; k < 5; k++){
					if(num->numero == m[j][k]){
						cnum++;
						fillListaNumeri(&num0, num->numero);
					}
				}												
			}

			importo = 0.0;
			comb = 0.0;
			
			for(impo = g->impo; impo != 0; impo = impo->next){
				comb = combinazioniNoRep(cnum, impo->tipo);
				
				//L'utente ha indovinato un numero su 5
				if(cnum == 1){
					if(impo->tipo == 1 && impo->importo > 0.0){
						importo = impo->importo * 11.23 * cnum;
					}else{
						importo = 0.0;
					}
				//L'utente ha indovinato due numeri su 5
				}else if(cnum == 2){
					if(impo->tipo == 1 && impo->importo > 0.0){
						importo = impo->importo * 11.23 * cnum;
					}else if(impo->tipo == 2 && impo->importo >  0.0){
						importo = impo->importo * 250.00;
					}else{
						importo = 0.0;
					}
				//L'utente ha indovinato tre numeri su 5
				}else if(cnum == 3){
					if(impo->tipo == 1 && impo->importo > 0.0){
						importo = impo->importo * 11.23 * cnum;
					}else if(impo->tipo == 2 && impo->importo > 0.0){
						importo = (impo->importo * 250.00) / (comb);
					}else if(impo->tipo == 3 && impo->importo > 0.0){
						importo = impo->importo * 4500.00;
					}else{
						importo = 0.0;
					}
				//L'utente ha indovinato quattro numeri su 5
				}else if(cnum == 4){
					if(impo->tipo == 1 && impo->importo > 0.0){
						importo = impo->importo * 11.23 * cnum;
					}else if(impo->tipo == 2 && impo->importo > 0.0){
						importo = (impo->importo * 250.00) / (comb);
					}else if(impo->tipo == 3 && impo->importo > 0.0){
						importo = (impo->importo * 4500.00) / (comb);
					}else if(impo->tipo == 4 && impo->importo > 0.0){
						importo = impo->importo * 120000.00;
					}else{
						importo = 0.0;
					}
				//L'utente ha indovinato cinque numeri su 5
				}else if(cnum == 5){
					if(impo->tipo == 1 && impo->importo > 0.0){
						importo = impo->importo * 11.23 * cnum;
					}else if(impo->tipo == 2 && impo->importo > 0.0){
						importo = (impo->importo * 250.00) / (comb);
					}else if(impo->tipo == 3 && impo->importo > 0.0){
						importo = (impo->importo * 4500.00) / (comb);
					}else if(impo->tipo == 4 && impo->importo > 0.0){
						importo = (impo->importo * 120000.00) / (comb);
					}else if(impo->tipo == 5 && impo->importo > 0.0){
						importo = impo->importo * 6000000.00;
					}else{
						importo = 0.0;
					}
				}
				//Inserimento dei dati calcolati nella lista degli importi
				fillListaImporti(&impo0, importo, impo->tipo);
			}
			registerVincita(vincite, g->username, g->giorno, g->mese, g->anno, g->ora, g->min, r->nome_ruota, num0, impo0, 1);							
		}
	}
}

//Funzione di assegnamento della giocata da nuova (ancora in corso) a vecchia (passata)
//e di assegnamento della vincita tra quelle vecchie
void AssegnaGiocata(struct giocata **vecchie, struct vincita **v, struct giocata *nuove, char **ex_ruote, int** m, int n_giorno, int giorno, int mese, int anno, int hour, int min, int sec){
	struct giocata *g;
	
	for(g = nuove; g != NULL; g = g->next){		
		registerGiocata(vecchie, g->username, n_giorno, giorno, mese, anno, hour, min, sec, g->r, g->num, g->impo, 1);
		AssegnaVincita(vecchie, v, ex_ruote, m);
	}
}

//Funzione per il blocco del client al suo terzo tentativo di accesso tramite login
void blockUser(int i, struct user *u0, char *cl_sk){

	struct user *p;
	
	for(p = u0; p; p = p->next){
		//Ricerca dell'ip nella lista users, se e' uguale a quello del socket
		if(!strcmp(cl_sk, p->cl_ip)){
			//Decrementare tentativi del client che è collegato
			if(p->tentativi > 0){
				p->tentativi--;
				//Invio tentativi al client così che possa tenerne traccia
				sendToInt(i, p->tentativi);
			}
			
			//Quando arriva a 0
			if(p->tentativi == 0){
			
				//Viene settato il flag di blocco
				p->blocked = 1;
				
				//Current timestamp 
				time_t seconds;
				seconds = time(NULL);
				
				p->secBlocco = seconds;

				//Scrivere ip e timestamp nel file bloccati.txt
				createFileBlocked(cl_sk, seconds);
			
				//Invio messaggio al client del timestamp
				printf("I tentativi di %s sono arrivati a 0!\n", p->cl_ip);
				close(i);
				return;
			}else{
				return;
			}
		}
	}
}

//Funzione di apertura del file delle giocate e riempimento iniziale della lista delle giocate
void openFileGiocate(struct giocata **g0){
	char username[1024], buffer[1024], ruota[11];
	
	struct ruote *r;
	struct numeri *num;
	struct importi *impo;
	
	int ng, day, month, year, ora, minuti, secondi, n, numero, j;
	double importo;	
	
	//Apertura del file delle giocate in lettura
	FILE *file = fopen("FileRegistri/UsersGiocate.txt", "r");
	if(file == NULL){
		printf("Impossibile creare il file delle giocate!\n");
		exit(1);
	}

	while(1){	
		r = 0;
		num = 0; 
		impo = 0;
		
		//Lettura dal file di username e timestamp
		int l = fscanf(file, "%s", username);
		if(l == EOF)
			break;
									
		l = fscanf(file, "%s", buffer);
		if(l == EOF)
			break;
		ng = atoi(buffer);
		
		l = fscanf(file, "%s", buffer);
		if(l == EOF)
			break;
		day = atoi(buffer);
		
		l = fscanf(file, "%s", buffer);
		if(l == EOF)
			break;
		month = atoi(buffer);
		
		l = fscanf(file, "%s", buffer);
		if(l == EOF)
			break;
		year = atoi(buffer);
		
		l = fscanf(file, "%s", buffer);
		if(l == EOF)
			break;
		ora = atoi(buffer);
		l = fscanf(file, "%s", buffer);
		
		if(l == EOF)
			break;
		minuti = atoi(buffer);
		
		l = fscanf(file, "%s", buffer);
		if(l == EOF)
			break;
		secondi = atoi(buffer);
		
		//Lettura del numero delle ruote
		l = fscanf(file, "%s", buffer);
		if(l == EOF)
			break;						
		n = atoi(buffer);
		for(j = 0; j < n; j++){
			//Lettura delle ruote
			l = fscanf(file, "%s", ruota);
			if(l == EOF)
				break;
			fillListaRuote(&r, ruota);
		}
		
		//Lettura del numero dei numeri
		l = fscanf(file, "%s", buffer);
		if(l == EOF)
			break;
		n = atoi(buffer);
		for(j = 0; j < n; j++){
			//Lettura dei numeri
			l = fscanf(file, "%s", buffer);
			if(l == EOF)
				break;
			numero = atoi(buffer);
			fillListaNumeri(&num, numero);
		}
		
		//Lettura del numero degli importi
		l = fscanf(file, "%s", buffer);
		if(l == EOF)
			break;
		n = atoi(buffer);
		for(j = 0; j < n; j++){
			//Lettura degli importi
			l = fscanf(file, "%s", buffer);
			if(l == EOF)
				break;				
			importo = atof(buffer);
			fillListaImporti(&impo, importo, j+1);
		}
		
		registerGiocata(g0, username, ng, day, month, year, ora, minuti, secondi, r, num, impo, 0);

		if(l == EOF)
			break;
	}
	
	fclose(file);
}

/*Funzione di apertura del file degli utenti registrati
e riempimento iniziale della lista degli utenti*/
void openFileRegister(struct user **u0){
	
	char username[1024], password[1024], cl_ip[1024];	
	
	memset(&username, '\0', sizeof(username));
	memset(&password, '\0', sizeof(password));
	memset(&cl_ip, '\0', sizeof(cl_ip));
	
	FILE *file = fopen("FileRegistri/UsersRegister.txt", "r");
	if(file == NULL){
		printf("[LOG] Impossibile creare il file degli utenti registrati!\n");
		exit(1);
	}

	while(1){
		int l = fscanf(file, "%s%s%s", cl_ip, username, password);
		if(l == EOF)
			break;
		//Riempimento della lista con i campi corrispondenti trovati nel file
		registerUsers(cl_ip, u0, username, password, 0);
	}

	fclose(file);
}

/*Funzione di apertura del file degli utenti bloccati
e controllo su un eventuale sbloccaggio*/
bool openFileBlocked(char *cl_sk){
	char buffer[1024], cl_ip[16], *str;
	long secondi;
	time_t seconds;
	
	FILE *file = fopen("FileRegistri/ClientBlocked.txt", "r");
	if(file == NULL){
		printf("[LOG] Impossibile creare il file degli utenti bloccati!\n");
		exit(1);
	}

	//Current timestamp 
	seconds = time(NULL);

	while(1){
		int l = fscanf(file, "%s", cl_ip);
		if(l == EOF)
			break;
		
		l = fscanf(file, "%s", buffer);
		if(l == EOF)
			break;
		//Conversione di una stringa in long int
		secondi = strtol(buffer, &str, 1024);
	
		//Controllo se cl_addr si trova all'interno del file
		if(!strcmp(cl_sk, cl_ip)){
			//e' passata mezz'ora?
			if((seconds - secondi) < (30*60)){
				printf("[LOG] Client con indirizzo %s e' ancora bloccato.\n", cl_ip);
				return 1;
			}
		}
	}

	fclose(file);
	
	return 0;
}

/*Funzione di apertura del file delle vincite di tutti i client
e riempimento iniziale della lista delle vincite*/
void openFileVincite(struct vincita **v0){
	
	char username[1024], buffer[1024], ruota[11];
	
	struct numeri *num;
	struct importi *impo;
	
	int day, month, year, hour, min, n, numero, j;
	double importo;	

	FILE *file = fopen("FileRegistri/UsersVincite.txt", "r");
	if(file == NULL){
		printf("[LOG] Impossibile creare il file delle vincite!\n");
		exit(1);
	}
	
	while(1){	
		num = 0;
		impo = 0;
		
		//Lettura dal file di username e timestamp
		int l = fscanf(file, "%s", username);
		if(l == EOF)
			break;
		
		l = fscanf(file, "%s", buffer);
		if(l == EOF)
			break;
		day = atoi(buffer);
		
		l = fscanf(file, "%s", buffer);
		if(l == EOF)
			break;
		month = atoi(buffer);
		
		l = fscanf(file, "%s", buffer);
		if(l == EOF)
			break;
		year = atoi(buffer);
		
		l = fscanf(file, "%s", buffer);
		if(l == EOF)
			break;
		hour = atoi(buffer);
		
		l = fscanf(file, "%s", buffer);
		if(l == EOF)
			break;
		min = atoi(buffer);
		
		//Lettura delle ruote
		l = fscanf(file, "%s", ruota);
		if(l == EOF)
			break;
			
		//Lettura del numero dei numeri
		l = fscanf(file, "%s", buffer);
		if(l == EOF)
			break;
		n = atoi(buffer);
		for(j = 0; j < n; j++){
			//Lettura dei numeri
			l = fscanf(file, "%s", buffer);
			if(l == EOF)
				break;
			numero = atoi(buffer);
			fillListaNumeri(&num, numero);
		}
		
		//Lettura del numero degli importi
		l = fscanf(file, "%s", buffer);
		if(l == EOF)
			break;
		n = atoi(buffer);
		for(j = 0; j < n; j++){
			//Lettura degli importi
			l = fscanf(file, "%s", buffer);
			if(l == EOF)
				break;				
			importo = atof(buffer);
			fillListaImporti(&impo, importo, j+1);
		}
		
		registerVincita(v0, username, day, month, year, hour, min, ruota, num, impo, 0);

		if(l == EOF)
			break;
	}
	
	fclose(file);
}

/*Funzione di apertura del file delle estrazioni
e riempimento iniziale della lista delle estrazioni*/
void openFileEstrazioni(struct estrazioni *e0, char **ex_ruote, int **m){
	
	char buffer[1024];	
	
	int ng, day, month, year, hour, min, sec, i, j;
	
	//Apertura del file delle giocate in lettura
	FILE *file = fopen("FileRegistri/Estrazioni.txt", "r");
	if(file == NULL){
		printf("Impossibile creare il file delle vincite!\n");
		exit(1);
	}

	while(1){
		int l = fscanf(file, "%s", buffer);
		if(l == EOF)
			break;
		ng = atoi(buffer);
		
		l = fscanf(file, "%s", buffer);
		if(l == EOF)
			break;
		day = atoi(buffer);
		
		l = fscanf(file, "%s", buffer);
		if(l == EOF)
			break;
		month = atoi(buffer);
		
		l = fscanf(file, "%s", buffer);
		if(l == EOF)
			break;
		year = atoi(buffer);
		
		l = fscanf(file, "%s", buffer);
		if(l == EOF)
			break;
		hour = atoi(buffer);
		
		l = fscanf(file, "%s", buffer);
		if(l == EOF)
			break;
		min = atoi(buffer);
		
		l = fscanf(file, "%s", buffer);
		if(l == EOF)
			break;
		sec = atoi(buffer);	

		for(i = 0; i < 11; i++){
			for(j = 0; j < 5; j++){
				l = fscanf(file, "%s", buffer);
				if(l == EOF)
					break;
				m[i][j] = atoi(buffer);	
			}
			if(l == EOF)
				break;
		}
	
		if(l == EOF)
			break;
	}
	
	fillListaEstrazioni(&e0, ng, day, month, year, hour, min, sec, ex_ruote, m);
	
	fclose(file);
}

//Funzione per lo sbloccaggio del client con il settaggio di appositi parametri
void UnlockClient(char *cl_sk, struct user *u0){
	struct user *p;
		
	for(p = u0; p; p = p->next){        
		if(!strcmp(p->cl_ip, cl_sk)){
			p->blocked = 0;
			p->tentativi = 3;
			p->online = 0;
		}
	}
}
/*********************************************************************/

/******************************Funzioni Client************************/

//Menu del gioco del lotto
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

//Funzione di assegnamento della lettera per ogni comando
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

//Funzione di riempimento della lista delle ruote
bool fillListaRuote_Cl(struct ruote **r0, char *ruota){
	struct ruote *q, *p, *s;
	
	//Controllo se il client ha gia' inserito quella ruota
	if(*r0 != NULL){
		for(s = *r0; s != 0; s = s->next){
			if(!strcmp(s->nome_ruota, ruota))
				return 1;
		}
	}
	
	for(s = *r0; (s != 0); s = s->next)
		p = s;
		
	q = (struct ruote*)malloc(sizeof(struct ruote));
	
	strcpy(q->nome_ruota, ruota);
	q->next = s;

	if(*r0 == 0)
		*r0 = q;
	else
		p->next = q;

	//free(q);
	
	return 0;
}

//Funzione di riempimento della lista dei numeri
bool fillListaNumeri_Cl(struct numeri **n0, int numero){
	struct numeri *q, *p, *s;
	
	//Controllo se il client ha gia' inserito quel numero
	if(*n0 != 0){
		for(s = *n0; s != 0; s = s->next){
			if(s->numero == numero)
				return 1;
		}
	}
	
	for(s = *n0; (s != 0); s = s->next)
		p = s;
		
	q = (struct numeri*)malloc(sizeof(struct numeri));
	
	q->numero = numero;
	q->next = s;

	if(*n0 == 0)
		*n0 = q;
	else
		p->next = q;

	//free(q);
	
	return 0;
}

//Funzione di controllo sull' inserimento corretto della ruota
//Le ruote da inserire devono essere soltanto 11 altrimenti la funzione restituisce false
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
	}else if(!strcmp(ruota, "Napoli")){
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

//Funzione di stampa delle istruzioni per i comandi passati al comando !help
void stampaIstruzioneHelp(char *command){
	if(!strcmp("signup", command)){
		printf("Crea un nuovo utente\n");
	}else if(!strcmp("login", command)){
		printf("Autentica un utente\n");
	}else if(!strcmp("invia_giocata", command)){
		printf("Invia una giocata g al server\n");
	}else if(!strcmp("vedi_giocate", command)){
		printf("Visualizza le giocate precedenti dove tipo = (0,1) e permette di visualizzare le giocate passate '0' oppure le giocate attive '1' (ancora non estratte)\n");
	}else if(!strcmp("vedi_estrazione", command)){
		printf("Mostra i numeri delle ultime n estrazioni sulla ruota specificata\n");
	}else if(!strcmp("vedi_vincite", command)){
		printf("Richiede al server tutte le vincite del client\n");
	}else if(!strcmp("esci", command)){
		printf("Effettua il logout e la disconnessione dal server\n");
	}else{
		printf("Hai ricercato un comando non disponibile o errato!\nI comandi disponibili sono : \n- signup\n- login\n- invia_giocata\n- vedi_giocate\n- vedi_estrazione\n- vedi_vincite\n- esci\n");
		return;
	}
}

//Funzione di invio al server di un carattere corrispondente al comando
void sendToServerChar(int sd, char c){
	char dim[2];
	int ret;
	
	memset(&dim, '\0', sizeof(dim));
	dim[0] = c;
	
	ret = send(sd, (void*)dim, 2, 0);
	
	if(ret < 0){
		perror("Send errata di un carattere da parte del client!\n");
		exit(1);
	}
}

//Funzione per ricevere dal server il flag di controllo 
//sull'username nel comando signup, e su entrambi username
//e password
bool recvFromServerFlag(int sd){
	int ret;
	char dim[2];
	
	memset(&dim, '\0', sizeof(dim));
	
	ret = recv(sd, (void*)dim, 2, 0);

	
	
	if(ret < 0){
		perror("Receive errata di un flag da parte del server!\n");
		exit(1);
	}
	
	if(dim[0] == '0')
		return 0;
	else 
		return 1;
}

//Funzione per ricevere dal server una stringa
void recvFromServer(int sd, int stato){
	int ret, len;
	char dim[2], buffer[1024];
	double i;
	
	memset(&dim, '\0', sizeof(dim));
	
	ret = recv(sd, (void*)dim, 2, 0);

	//printf("[recvFromServer] dim - %s\n", dim);
	
	len = atoi(dim);				//string -> int

	//printf("[recvFromServer] len - %d\n", len);
	
	if(ret < 0){
		perror("Receive errata della lunghezza da parte del server!\n");
		exit(1);
	}
	
	memset(&buffer, '\0', sizeof(buffer));
	
	ret = recv(sd, (void*)buffer, len, 0);

	//printf("[recvFromServer] buffer - %s\n", buffer);

	if(ret < 0){
		perror("Receive errata della stringa da parte del server!\n");
		exit(1);
	}
	
	//In base alla variabile stato passata come parametro 
	//la funzione avrà un diverso comportamento finale
	if(stato == 1){
		strcpy(sessionId, buffer);
	}else if(stato == 2){
		printf("%s ", buffer);
	}else if(stato == 3){
		i = atof(buffer);
		printf("%f ", i);
	}else if(stato == 4){
		printf("%s\t", buffer);
	}
}

//Funzione di invio al server, in base al carattere del comando, delle informazioni
//necessarie per completare l'operazione 
void SendToServerCmd(int sd, char c){
	
	/*command : stringa per il comando
	  username : stringa per contenere l'username inserito dal client
	  password : stringa per contenere la password inserita dal client
	  opzione : inerente al comando !invia_giocata
	  buffer : stringa di appoggio per l'invio e la ricezione di dati
	  ch : lettura del carattere invio durante un comando
	  ruota : stringa per contenere il nome della ruota inserita dal client
	  rts : stringa di appoggio pr la conversione double to string
	*/
	char command[16], username[1024], password[1024], opzione[2], buffer[1024], ch, ruota[11], rts[7];
	bool check, controllo;
	int n, cont, i, j, k, w, y, h, z, o, numero, contN, tipo;
	int numb, flag;
	double importo;
	
	//Puntatori alle strutture : servono per inserire gli elementi immessi dal client
	struct ruote *r;
	struct numeri *num;
	struct importi *impo;
	
	r = 0;
	num = 0;
	impo = 0;
	
	
	memset(&command, '\0', sizeof(command));
	memset(&opzione, '\0', sizeof(opzione));
	memset(&buffer, '\0', sizeof(buffer));
	memset(&username, '\0', sizeof(username));
	memset(&password, '\0', sizeof(password));
	
	switch(c){
		case 'h' :
			// !help
			ch = getchar();
			//L'utente ha solo inserito il comando !help e premuto invio
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
			
			//invio il carattere relativo al comando al server
			sendToServerChar(sd, c);
			sendTo(sd, username);
			sendTo(sd, password);
			
			check = recvFromServerFlag(sd);
			if(check)
				printf("Username gia' in uso\n");
			else
				printf("Registrazione effettuata con successo!\n");
			break;
		case 'l' :
			// !login
			scanf("%s", username);
			scanf("%s", password);
			
			sendToServerChar(sd, c);
			sendTo(sd, username);
			sendTo(sd, password);
			
			//Il client riceve la conferma se l'username e password inseriti sono giusti o sbagliati
			check = recvFromServerFlag(sd);
			if(!check){
				//Il client riceve il numero di tentativi rimasti
				tentativi = recvFromInt(sd);
				//Il client può continuare a inviare il comando login finche il parametro tentativi non è a 0
				if(tentativi > 0){
					printf("Prova a rinviare il comando !login.\nHai ancora %d tentativi!\n", tentativi);
				}else{
					//Se i tentativi arrivano al valore 0, il client viene bloccato per 30 min
					printf("I tentativi sono arrivati a 0! Sei stato bloccato!\n");
					exit(1);
				}
			}else{
				controllo = recvFromServerFlag(sd);
				//Il login e' andato a buon fine quindi 
				//il server invia l'id session al client
				if(controllo){
					recvFromServer(sd, 1);
					printf("Il tuo sid : %s, Ora sei online!\n", sessionId);
				}else{
					recvFromServer(sd, 2);
				}
			}	
			break;
		case 'i' :
			// !invia_giocata
			
			if(sessionId[0] != '\0'){
				scanf("%s", opzione);
				
				if(!strcmp("-r", opzione)){
					//Si effettua un ciclo per la lettura e l'inserimento della ruota o delle ruote che si vogliono inviare al server
					do{
						//Lettura della ruota che il client inserisce
						scanf("%s", buffer);
						//Controlli :
						//	- sul doppio inserimento di una ruota 
						//	- sulla opzione successiva 
						//	- sul nome della ruota
						if(checkRuota(buffer)){
							check = fillListaRuote_Cl(&r, buffer);
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

					do{
						//Lettura dei numeri che il client inserisce
						scanf("%s", buffer);
						
						n = atoi(buffer);
							
						//Controllo sul range dei numeri inseriti e alla quantità inserita
						if(((n >= 1) && (n <= 90)) && (cont <= 10)){
							check = fillListaNumeri_Cl(&num, n);
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
					}while((strcmp("-i", buffer)) != 0);		//Inserimento finchè l'utente non inserisce -i
					
					//Usciti dal ciclo l'ultima scanf ha preso la stringa -i 
					
					cont = 1;
					ch = ' ';
					
					do{

						if(cont <= contN){

							scanf("%s", buffer);

							importo = atof(buffer);

							//Invio degli importi
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
							
							//Estrae il carattere inviato
							ch=getchar();

						}else{
							printf("Hai inserito piu' importi che numeri.\n");
							return;
						}
					}while(ch != '\n');
									
					sendToServerChar(sd, c);

					//Puntatori alle strutture : per scorrere le liste e mandare i dati al server
					struct ruote *p = r;
					struct numeri *q = num;
					struct importi *s = impo;

					//Scorrimento della lista
					//Invio delle ruote al server					
					while(p){
						sendTo(sd, p->nome_ruota);
						p = p->next;
					}
					
					sendTo(sd, "-n");
					
					//Scorrimento della lista
					//Invio dei numeri al server 
					memset(&buffer, '\0', sizeof(buffer));
					while(q){
						sprintf(buffer, "%d", q->numero);
						sendTo(sd, buffer);
						q = q->next;
					}
					
					sendTo(sd, "-i");
					
					//Scorrimento della lista
					//Invio degli importi al server
					while(s){
						gcvt(s->importo, 5, rts);
						//invio l'importo
						sendTo(sd, rts);
						s = s->next;
					}

					//Invio la stringa fine al server per notificargli la fine del comando inviato
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
			
			if(sessionId[0] != '\0'){
				scanf("%s", buffer);
			
				flag = atoi(buffer);
			
				if(flag < 0 || flag > 1){
					printf("Devi digitare 0 o 1. Riprova a inviare il comando.\n");
					return;
				}
			
				//Invio il char del comando
				sendToServerChar(sd, c);
			
				//Invio il flag
				sendToInt(sd, flag);
			
				//Ricezione dal server delle giocate ancora in corso o delle giocate passate 
				//in base al flag inviato dal client stesso
				//Ricezione dal server il numero delle giocate effettuate
				k = recvFromInt(sd);
				
				if(k == 0){
					if(flag){
						printf("Non hai giocate ancora in corso\n");
						break;
					}else{
						printf("Non hai giocate concluse\n");
						break;
					}
				}
				
				for(i = 0; i < k; i++){
					printf("%d) ", y);
		
					//Ricezione delle ruote
					j = recvFromInt(sd);
					for(w = 0; w < j; w++){
						recvFromServer(sd, 2);
					}
				
					//Ricezione dei numeri
					j = recvFromInt(sd);
					for(w = 0; w < j; w++){
						n = recvFromInt(sd);
						printf("%d ", n);
					}
			
					//Ricezione degli importi
					j = recvFromInt(sd);
					for(w = 0; w < j; w++){
						printf("* ");
						recvFromServer(sd, 3);
						recvFromServer(sd, 2);
					}
					printf("\n");
					y++;
				}
			}else{
				printf("Devi effettuare prima il login per inviare la giocata.\n");
				printf("Se non sei ancora registrato basta digitare il comando !signup e inserire username e password.\n");
				return;
			}	
			
			break;
		case 'e' :
			// !vedi_estrazione
			
			memset(&buffer, '\0', sizeof(buffer));
			cont = 0;
			
			if(sessionId[0] != '\0'){
				scanf("%s", buffer);
				
				numero = atoi(buffer);
				
				if(numero < 0){
					printf("Hai inserito un numero negativo. Riprova a inviare il comando\n");
					return;
				}
				
				ch = getchar();
				
				//Il client non ha premuto invio quindi l'applicazione aspetta la ruota
				if(ch != '\n'){
					scanf("%s", ruota);
					if(!checkRuota(ruota)){
						printf("La ruota che hai inserito non e' quella corretta. Riprova a inviare il comando\n");
						return;
					}
				}
				
				//Invio del carattere del comando al server
				sendToServerChar(sd, c);
				
				//Invio del carattere dopo la scanf per comunicare al server
				//che il client ha inserito anche la ruota
				sendToServerChar(sd, ch);

				//Invio del numero
				sendTo(sd, buffer);
			
				//Ricezione di del numero di Estrazioni totali dal server
				cont = recvFromInt(sd);
				
				k = 0;
				
				//Invio la ruota
				if(ch != '\n'){
					sendTo(sd, ruota);
					
					//Il client aspetta di ricevere tutte le estrazioni di quella ruota
					if(numero >= cont){
						while(k < cont){
							printf("%s\t\t", ruota);
							for(j = 0; j < 5; j++){
								numb = recvFromInt(sd);
								printf("%d ", numb);
							}
							printf("\n");
							k++; 
						}
					}else{
						while(k < numero){
							printf("%s\t\t", ruota);
							for(j = 0; j < 5; j++){
								numb = recvFromInt(sd);
								printf("%d ", numb);
							}
							printf("\n");
							k++;
						}
					}
				}else{
					
					//Il client aspetta di ricevere tutte le estrazioni
					if(numero >= cont){
						while(k < cont){
							for(i = 0; i < 11; i++){
								recvFromServer(sd, 4);
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
								recvFromServer(sd, 4);
								for(j = 0; j < 5; j++){
									numb = recvFromInt(sd);
									printf("%d ", numb);
								}
								printf("\n");
							}
							k++;
						}
					}
				}
				printf("****************************************\n");
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
				
				//Ricezione del numero delle vincite
				i = recvFromInt(sd);
				
				if(i == 0){
					printf("Non hai ancora registrato delle vincite\n");
					return;
				}else{
				
					for(j = 0; j < i; j++){
						
						//Ricezione del timestamp
						printf("Estrazione del ");
						o = recvFromInt(sd);
						if(o < 10)
							printf("0%d-", o);
						else
							printf("%d-", o);
						o = recvFromInt(sd);
						if(o < 10)
							printf("0%d-", o);
						else
							printf("%d-", o);
						o = recvFromInt(sd);
						printf("%d ", o);
						o = recvFromInt(sd);
						if(o < 10)
							printf("alle ore 0%d:", o);
						else
							printf("alle ore %d:", o);
						o = recvFromInt(sd);
						if(o < 10)
							printf("0%d\n", o);
						else
							printf("%d\n", o);
						
						//Ricezione della ruota
						recvFromServer(sd, 2);
						
						//Ricezione quantità numeri
						y = recvFromInt(sd);
						
						for(h = 0; h < y; h++){
							numb = recvFromInt(sd);
							printf("%d ", numb);
						}
						
						//Ricezione numero importi
						k = recvFromInt(sd);
						
						printf("\t>>\t");
						
						for(z = 0; z < k; z++){
							tipo = recvFromInt(sd);
							if(tipo == 1)
								printf("Estratto : ");
							else if(tipo == 2)
								printf("Ambo : ");
							else if(tipo == 3)
								printf("Terno : ");
							else if(tipo == 4)
								printf("Quaterna : ");
							else if(tipo == 5)
								printf("Cinquina : ");
							recvFromServer(sd, 3);
						}
						printf("\n****************************************\n");
					}
					
					j = recvFromInt(sd);
					
					/*

					if(j > 0){
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

					*/
				}
			}else{
				printf("Devi effettuare prima il login.\n");
				printf("Se non sei ancora registrato basta digitare il comando !signup e inserire username e password.\n");
				return;
			}	
			break;
		case 'q' :
			// !esci
			sendToServerChar(sd, c);
			//Ricezione del check di avvenuto o non avvenuto logout
			check = recvFromServerFlag(sd);
			if(check){
				if(sessionId[0] != '\0'){
					printf("Log out effettuato con successo!\n");
					//Reset del session Id
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
			break;
	}
}

/*********************************************************************/
