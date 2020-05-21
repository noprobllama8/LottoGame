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
#include "library.h"

#define TRUE 1
#define FALSE 0
#define USERLOG 100

struct user *users;					//struttura registro degli utenti registrati
struct giocata *giocate;			//struttura registro delle giocate degli utenti
struct vincita *vincite;

struct user{
	//struct sockaddr_in cl_ip; 		
	char cl_ip[16];					//ip del client
	char sessionId[11];				//id di sessione assegnato all'username al login
	char username[1024];			//username che fornisce il client al server
	char password[1024];			//password che fornisce il client al server
	int tentativi;					//tentativi di login da parte del client
	bool online;					//flag che rappresenta se l'username è online o meno
	bool blocked;                   //flag che rappresenta se il client è bloccato
	char timestamp[1024];			//timestamp di blocco (si resetta allo sblocco)
	int minuti;						
	struct user *next;
};

//struttura della giocata per ogni username
struct giocata{
	char username[1024];
	
	//timestamp
	int nome_giorno;
	int giorno;
	int mese;
	int anno;
	int ora;
	int min;
	int sec;

	struct ruote *r;				
	struct numeri *num;				//1-90
	struct importi *impo;  			
	struct giocata *next;
};

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

struct estrazioni{
	
	//timestamp
	int nome_giorno;
	int giorno;
	int mese;
	int anno;
	int ora;
	int min;
	int sec;
	
	char* nome_ruote[11];
	int g_vincenti[11][5];
	struct estrazioni *next;
};

struct vincita{
	//timestamp estrazione
	int nome_giorno;
	int giorno;
	int mese;
	int anno;
	int ora;
	int min;
	int sec;
	
	char username[1024];
	struct ruote *r;				
	struct numeri *num;	
	struct importi *impo;  			
	struct vincita *next;
};

//funzione di invio al client del flag di controllo 
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
		perror("Send errata!\n");
		exit(1);
	}/*else{
		printf("Stringa inviata corretemente : %s\n", dim);
	}*/
}

void sendToClientGiocate(int sd, struct giocata *g){
	//controllo il timestamp
	//printf("%d) ", j);
	int j = 0;
	char buffer[1024];
	
	//struct giocate *p = g;
	
	struct ruote *r;
	struct numeri *num;
	struct importi *impo;
	
	r = g->r;
	num = g->num;
	impo = g->impo;
	
	memset(&buffer, '\0', sizeof(buffer));
	
	while(r){
		j++;
		r = r->next;
	}
											
	//invia il numero delle ruote
	sendToInt(sd, j);
	
	//printf("j (ruote) : %d ", j);
	
	r = g->r;
											
	//invia le ruote
	while(r){
		//printf("%s ", r->nome_ruota);
		sendTo(sd, r->nome_ruota);
		//printf("%s ", r->nome_ruota);
		r = r->next;
	}
	//printf("\n");
											
	j = 0;
											
	while(num){
		j++;
		num = num->next;
	}
											
	//invia il numero dei numeri
	sendToInt(sd, j);
	
	//printf("j (numeri) : %d ", j);
								
	j = 0;
	num = g->num;
											
	//invia i numeri
	while(num){
		//printf("%d ", num->numero);
		sendToInt(sd, num->numero);
		//printf("%d ", num->numero);
		num = num->next;
	}
	//printf("\n");
											
	while(impo){
		j++;
		impo = impo->next;
	}
										
	//invia il numero di importi
	sendToInt(sd, j);
	
	//printf("j (importi) : %d ", j);
								
	//j = 0;
	impo = g->impo;
											
	//invia gli importi
	while(impo){
		//printf("* %f ", impo->importo);
		gcvt(impo->importo, 5, buffer);
		//invio l'importo
		sendTo(sd, buffer);
		
		//printf("%s ", buffer);
											
		if(impo->tipo == 1){
			//printf("estratto ");
			sendTo(sd, "estratto");
		}else if(impo->tipo == 2){
			//printf("ambo ");
			sendTo(sd, "ambo");
		}else if(impo->tipo == 3){
			//printf("terno ");
			sendTo(sd, "terno");
		}else if(impo->tipo == 4){
			//printf("quaterna ");
			sendTo(sd, "quaterna");
		}else if(impo->tipo == 5){
			//printf("cinquina ");
			sendTo(sd, "cinquina");
		}										
		impo = impo->next;
	}
	//printf("\n");
}

//funzione di ricezione del server dal client di un carattere corrispondente al comando
char recvFromClientChar(int sd){
	char dim[2], c;
	int ret;
	
	memset(&dim, '\0', sizeof(dim));
	
	ret = recv(sd, (void*)dim, 2, 0);
	
	//printf("dim : %s\n", dim);
	
	if(ret < 0){
		perror("Send errata!\n");
		exit(1);
	}else{
		c = dim[0];
		//printf("Carattere del comando ricevuto!\n");
	}

	return c;
}

//funzione di ricezione del server dal client
void recvFromClient(int sd, char *buffer){
	int ret, len;
	char dim[2];
	
	memset(&dim, '\0', sizeof(dim));
	
	ret = recv(sd, (void*)dim, 2, 0);
	len = atoi(dim);
	//printf("%d\n", len);
	if(ret < 0){
		perror("Receive errata!\n");
		exit(1);
	}/*else{
		printf("Lunghezza ricevuta : %d\n", len);
	}*/
	
	ret = recv(sd, (void*)buffer, len, 0);
	if(ret < 0){
		perror("Receive errata!\n");
		exit(1);
	}/*else{
		printf("Ho ricevuto la stringa (dentro funzione): %s\n", buffer);
	}*/
	
	//printf("%s\n", buffer);
}

//funzione di controllo dell'username al momento della registrazione
bool checkUsername(struct user **p0, char *username){
	struct user *p;
	
	for(p = *p0; p->next; p = p->next){
		if(!strcmp(p->username, username))
			return true;
	}
	
	return false;
}

//funzione di controllo dell'username e della password al momento del login
bool checkUsPsw(struct user **p0, char *username, char *password){
	struct user *p;
	
	/*printf("username : %s\n", username);
	printf("password : %s\n", password);*/

	for(p = *p0; p->next; p = p->next){
		/*printf("ip : %s", p->cl_ip);
		printf("p->username : %s\n", p->username);
		printf("p->password : %s\n", p->password);*/
		if(!strcmp(p->username, username)){
			if(!strcmp(p->password, password))
				return true;
		}
	}
	
	return false;
}

//funzione per la costruzione della lista degli user
struct user* buildListUsers(struct user **p0){
	struct user *user = (struct user*)malloc(sizeof(struct user));
	struct user *p;
		
	if(!user)
        return NULL;

    user->next = NULL;

    if(!*p0){
        *p0 = user;
		//printf("Entrato if\n");
	}
    else {
		//printf("Entrato else\n");
        p = *p0;
        while(p->next != NULL) {
            p = p->next;
        }
        p->next = user;
    }
	return user;
}

//funzione per la generazione di una stringa alfanumerica
//di 10 caratteri per l'id session del client che effettua
//il login
void generaIDsession(char *sessionId){
	int i;
	
	//0-9 = 48-57 , a-z = 97-122
	char str[36] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 
				  'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 
				  'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 
				  'u', 'v', 'w', 'x', 'y', 'z'}; 
	
	for(i = 0; i < 10; i++){
		sessionId[i] = str[rand() % 35];
	}
}

//funzione per il login del client
void loginUsers(struct user **p0, char *username, char *password, char *id){
	struct user *p;
		
	for(p = *p0; p->next; p = p->next){											//scorro la lista
		if(!strcmp(p->username, username)){										//se il campo username all'interno dell'elemento della lista è uguale all'username passato come parametro
			if(!strcmp(p->password, password)){									//e se il campo password all'interno dell'elemento della lista è uguale alla password passata come parametro
				if(p->online == false){											//e se il campo online all'interno dell'elemento della lista è ancora a false
					if(p->blocked == false){									//e se il campo bloccato all'interno dell'elemento della lista è ancora a false
						generaIDsession(id);									//genero l'id session
						strcpy(p->sessionId, id);								//lo copio all'interno del campo id dell'elemento della lista
						p->online = true;										//pongo il campo online dell'elemento corrispondente a true
						printf("Utente %s ora e' online\n", p->username);
					}
				}
			}
		}
	}
}

bool logoutUsers(struct user **p0, char *username){
	struct user *p;
		
	for(p = *p0; p->next; p = p->next){										//scorro la lista
		if(!strcmp(p->username, username)){									//se il campo username all'interno dell'elemento della lista è uguale all'username passato come parametro
			//printf("Trovato username!\n");
			if(p->online == true){										    //e se il campo online all'interno dell'elemento della lista è ancora a false
				if(p->sessionId[0] != '\0'){
				//printf("L'utente e' online!\n");
					if(p->blocked == false){									//e se il campo bloccato all'interno dell'elemento della lista è ancora a false
						//printf("Proseguo con il logout\n");
						memset(&p->sessionId, '\0', sizeof(p->sessionId));
						p->online = false;										//pongo il campo online dell'elemento corrispondente a true
						printf("Utente %s ora e' offline\n", p->username);
						return true;
					}
				}
			}
		}
	}
	
	return false;
}

//funzione di creazione del file registro
void createFileRegister(struct user *p0){
	/*char cl_ip[100];
	
	memset(&(cl_ip), '\0', sizeof(cl_ip));*/
		
	FILE *file = fopen("UsersRegister.txt", "a");
	if(file == NULL){
		printf("Impossibile creare il file!\n");
	}
	
	//inet_ntop(AF_INET, &(p0->cl_ip.sin_addr), cl_ip, INET_ADDRSTRLEN);
	
	fprintf(file, "%s\n%s\n", p0->username, p0->password);
	
	printf("Creazione e salvataggio del file avvenuti con successo!\n");
	
	fclose(file);
}

//funzione di creazione del file registro degli utenti bloccati
void createFileBlocked(char *cl_addr, char *time_block){
	FILE *file = fopen("ClientBlocked.txt", "a");
	if(file == NULL){
		printf("Impossibile creare il file!\n");
	}
	
	fprintf(file, "%s\n%s\n", cl_addr, time_block);	
	
	printf("Creazione e salvataggio del file avvenuti con successo!\n");
	
	fclose(file);
}

//funcione di creazione del file giocate
void createFileGiocate(struct giocata *g0){
	
	int cont;
	
	FILE *file = fopen("UsersGiocate.txt", "a");
	if(file == NULL){
		printf("Impossibile creare il file!\n");
	}
	
	struct giocata *p;
	struct ruote *q;
	struct numeri *s;
	struct importi *t;

	for(p = g0; p; p = p->next){
		fprintf(file, "%s\n%d %d %d %d %d %d %d\n", p->username, p->nome_giorno, p->giorno, p->mese, p->anno, p->ora, p->min, p->sec);
		
		//conteggio preliminarmente il numero delle ruote
		cont = 0;
		for(q = g0->r; q; q = q->next){
			cont++;
		}
		//lo stampo sul file
		fprintf(file, "%d\n", cont);
		for(q = g0->r; q; q = q->next){
			fprintf(file, "%s\n", q->nome_ruota);
		}
		//conteggio preliminarmente il numero dei numeri
		cont = 0;
		for(s = g0->num; s; s = s->next){
			cont++;
		}
		//lo stampo sul file
		fprintf(file, "%d\n", cont);
		for(s = g0->num; s; s = s->next){
			fprintf(file, "%d\n", s->numero);
		}
		//conteggio preliminarmente il numero degli importi
		cont = 0;
		for(t = g0->impo; t; t = t->next){
			cont++;
		}
		//lo stampo sul file
		fprintf(file, "%d\n", cont);
		for(t = g0->impo; t; t = t->next){
			fprintf(file, "%f\n", t->importo);
		}
	}
		
	printf("Creazione e salvataggio del file avvenuti con successo!\n");
	
	fclose(file);
}

//funzione della registrazione della giocata e riempimento della lista giocate
void registerGiocata(struct giocata **g0, char *username, int n_giorno, int giorno, int mese, int anno, int hour, int min, int sec, struct ruote *r0, struct numeri *n0, struct importi *i0, bool mode){
	struct giocata *q, *p;
	
	for(q = *g0; q != 0; q = q->next)
		p = q;
	
	q = (struct giocata*)malloc(sizeof(struct giocata));
	
	strcpy(q->username, username);
	
	q->nome_giorno = n_giorno;
	q->giorno = giorno;
	q->mese = mese;
	q->anno = anno;
	q->ora = hour;
	q->min = min;
	q->sec = sec;
	
	q->r = (struct ruote*)malloc(sizeof(struct ruote));
	q->r = r0;
	q->num = (struct numeri*)malloc(sizeof(struct numeri));
	q->num = n0;
	q->impo = (struct importi*)malloc(sizeof(struct importi));
	q->impo = i0;
	q->next = NULL;

	if(*g0 == 0)
		*g0 = q;
	else
		p->next = q;
	
	if(mode)
		createFileGiocate(q);
}

//funzione per la registrazione del client
void registerUsers(char *cl_addr, struct user **p0, char *username, char *password, bool stato){
	struct user *q, *p;
	
	for(q = *p0; q; q = q->next)
		p = q;
	
	//al momento della registrazione memorizzo l'ip che servira'
	//al momento del login per un eventuale blocco
		
	q = (struct user*)malloc(sizeof(struct user));
	
	//inet_pton(AF_INET, cl_addr, &(q->cl_ip));
	memset(&((q)->sessionId), '\0', sizeof((q)->sessionId));
	//strcpy((q)->cl_ip, cl_addr);
	//printf("ip : %s\n", q->cl_ip);
	strcpy((q)->username, username);
	//printf("username : %s\n", q->username);
	strcpy((q)->password, password);
	//printf("password : %s\n", q->password);
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
		printf("Utente registrato\n");
	}
}

//funzione di riempimento della lista delle ruote
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
}

//funzione di riempimento della lista dei numeri
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
}

//funzione di riempimento della lista dei numeri
void fillListaImporti(struct importi **i0, float importo, int tipo){
	struct importi *q, *p;
	
	for(q = *i0; q != 0; q = q->next)
		p = q;
		
	q = (struct importi*)malloc(sizeof(struct importi));
	
	q->importo = importo;
	//printf("importo : %f\n", q->importo);
	q->tipo = tipo;
	//printf("tipo : %d\n", q->tipo);
	q->next = NULL;

	if(*i0 == 0)
		*i0 = q;
	else
		p->next = q;
}

void fillListaEstrazioni(struct estrazioni **e0, int n_giorno, int giorno, int mese, int anno, int hour, int min, int sec, char **ruota, int **m, bool mode){
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
		//printf("entrato1\n");
		q->nome_ruote[i] = ruota[i];
		//printf("ha copiato la stringa\n");
		for(j = 0; j < 5; j++){
			//printf("entrato2\n");
			q->g_vincenti[i][j] = m[i][j];
			//printf("%d ", m[i][j]);
		}
		//printf("\n");
	}
	q->next = NULL;

	if(*e0 == 0)
		*e0 = q;
	else
		p->next = q;
	
	if(mode);
		//createFileEstrazioni(q);
}

void registerVincita(struct vincita **v0, char *username, struct ruote *r0, struct numeri *n0, struct importi *i0/*, bool mode*/){
	struct vincita *q, *p;
	
	for(q = *v0; q != 0; q = q->next)
		p = q;
	
	q = (struct vincita*)malloc(sizeof(struct vincita));
	
	strcpy(q->username, username);
	
	//timestamp di estrazione
	/*q->nome_giorno = n_giorno;
	q->giorno = giorno;
	q->mese = mese;
	q->anno = anno;
	q->ora = hour;
	q->min = min;
	q->sec = sec;*/
	
	q->r = (struct ruote*)malloc(sizeof(struct ruote));
	q->r = r0;
	q->num = (struct numeri*)malloc(sizeof(struct numeri));
	q->num = n0;
	q->impo = (struct importi*)malloc(sizeof(struct importi));
	q->impo = i0;
	q->next = NULL;

	if(*v0 == 0)
		*v0 = q;
	else
		p->next = q;
}

void RichiestaFromClientCmd(char c){	
	switch(c){
		case 'h' :
			break;
		case 's' :
			printf("Richiesta di registrazione\n");
			break;
		case 'l' :
			printf("Richiesta di login\n");
			break;
		case 'i' :
			printf("Richiesta di invia la giocata\n");
			break;
		case 'g' :
			printf("Richiesta di visualizzazione delle giocate effettuate\n");
			break;
		case 'e' :
			printf("Richiesta di visualizzazione dell'estrazione\n");
			break;
		case 'v' :
			printf("Richiesta di visualizzazione delle vincite effettuate\n");
			break;
		case 'q' :
			printf("Richiesta di logout\n");
			break;
		default :
			printf("Richiesta errata!\n");
			break;
	}
}

int fatt(int n){
	if(n == 0)
		return 1;
	return n * fatt(n);
}

int combinazioniNoRep(int n, int k){
	if(k > n)
		return -1;
	if(k < 0 || n < 0)
		return -1;
	if(k > 5)
		return -1;
	
	int combNoRip = 0;
	int fattN = fatt(n);
	int fattK = fatt(k);
	int fattNmenoK = fatt(n-k);
	
	combNoRip = (fattN) / (fattK * fattNmenoK);
	
	return combNoRip;
}

int main(int argc, char const *argv[]){
	
	struct sockaddr_in my_addr, cl_addr;
	socklen_t addrlen;
	
	struct user *busers;
	struct ruote *r;
	struct numeri *num;
	struct importi *impo;
	
	struct ruote *r0;
	struct numeri *num0;
	struct importi *impo0;
	
	struct estrazioni *ex;
	struct estrazioni *p;
	struct giocata *g;
	struct vincita *v;
	
	struct giocata *vecchie, *nuove;
	
	int sock , new_fd , ret, fdmax, i, periodo, numero, cont, piped[2], w, j, k, n, q, s, flag;
	float importo, estratto, ambo, terno, quaterna, cinquina;
	bool check, ipbloccato;
    char buffer[1024], c, username[1024], password[1024], id[11], cl_ip[16], ch, ruota[11];
	char cl_sk[16]/*, t_block[100]*/, nb[3];
	
	int n_giorno, giorno, mese, anno, hour, min, sec;
	int ng, day, month, year, ora, minuti, secondi;
	
	int** m;
	char* ex_ruote[11] = {"Bari", "Cagliari", "Firenze", "Genova", "Milano",
						  "Napoli", "Palermo", "Roma", "Torino", "Venezia", "Nazionale"};
			  
    fd_set readfds, masterfds;
	FILE *file;
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

	//printf("periodo : %d\n", periodo);
	FD_ZERO(&masterfds);
	
	ex = (struct estrazioni*)malloc(sizeof(struct estrazioni));
	ex = NULL;

	m = (int**)malloc(11 * sizeof(int*));
	m[0] = (int*)malloc(11 * 5 * sizeof(int));

	for(j = 0; j < 11; j++)
		m[j] = m[0] + j * 5;
	
	if(pipe(piped) < 0) {
		perror("piped");
		exit(1);
	}

	pid_t pid = fork();

	if(pid == -1){
		printf("Impossibile creare processo figlio!\n");
		exit(1);
	}else if(pid == 0){
		while(TRUE){
				//gestisco le estrazioni
			
				//current timestamp 
				time(&t);
				l_time = localtime(&t);
				strftime(buffer, 1024, "%u", l_time);
				n_giorno = atoi(buffer);
			
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
			
			if(n_giorno == 2 || n_giorno == 4 || n_giorno == 6){
				for(j = 0; j < 11; j++){
					for(k = 0; k < 5; k++){
						n = (rand()%89)+1;        //1-90
						m[j][k] = n;
									
						//con questo controllo l'estrazione e' senza ripetizione
						//per avere in un estrazione tutti i numeri diversi
						for(w = 0; w < k; w++){
							if(m[j][k] == m[j][w]){
								k--;
								break;
							}
						}
					}
				}
			
				fillListaEstrazioni(&ex, n_giorno, giorno, mese, anno, hour, min, sec, ex_ruote, m, 1);
				sleep(periodo);
			}
		}
	}else{
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
	
		FD_SET(sock, &masterfds);				//Aggiungo il sock al set 
		fdmax = sock;							//Tengo traccia del maggiore

		if(ret < 0){
			perror("Bind errata!\n");
			exit(1);
		}else{
			printf("Bind effettuata con successo!\n");
		}
	
		ret = listen(sock, USERLOG);

		if(ret < 0){
			perror("Listen errata!\n");
			exit(1);
		}else{
			printf("Server in ascolto...\n");
		}
	
		c = '\0';
		check = 0;
		
		ipbloccato = 0;
	
		r = (struct ruote*)malloc(sizeof(struct ruote));
		num = (struct numeri*)malloc(sizeof(struct numeri));
		impo = (struct importi*)malloc(sizeof(struct importi));
		r0 = (struct ruote*)malloc(sizeof(struct ruote));
		num0 = (struct numeri*)malloc(sizeof(struct numeri));
		impo0 = (struct importi*)malloc(sizeof(struct importi));
		
		vecchie = (struct giocata*)malloc(sizeof(struct giocata));
		nuove = (struct giocata*)malloc(sizeof(struct giocata));
		
		giocate = NULL;
		vincite = NULL;
		users = 0;
		
		while(TRUE){
			memset(&buffer, '\0', sizeof(buffer));
			readfds = masterfds;
			select(fdmax+1, &readfds, NULL, NULL, NULL);
		
			for(i = 0; i <= fdmax; i++){
				if(FD_ISSET(i, &readfds)){					//Trovato un descrittore pronto
					if(i == sock){
				
						busers = buildListUsers(&users);
					
						//far partire la accept
						addrlen = sizeof(cl_addr);
						new_fd = accept(i, (struct sockaddr *)&cl_addr, &addrlen);
					
						memset(&cl_sk, '\0', sizeof(cl_sk));
						memset(&cl_ip, '\0', sizeof(cl_ip));
					
						inet_ntop(AF_INET, &(cl_addr.sin_addr), cl_sk, INET_ADDRSTRLEN);
					
						/******************************************************************/
			            //riempimento dello storico degli user registrati
						//aprire il file degli user registrati
						file = fopen("UsersRegister.txt", "r");
						if(file == NULL){
							printf("Impossibile creare il file degli utenti registrati!\n");
						}
					
						//se ci sono dati memorizzati
						while(1){
							int l = fscanf(file, "%s%s", username, password);
							if(l == EOF)
								break;
							//printf("cl_ip : %s\n", cl_ip);
							//riempire la lista con i campi corrispondenti trovati nel file
							registerUsers(cl_ip, &busers, username, password, 0);
						}
					
						fclose(file);
						/******************************************************************/
						//aprire il file delle estrazioni in lettura
						
						file = fopen("Estrazioni.txt", "r");
						if(file == NULL){
							printf("Impossibile aprire il file!\n");
						}
	
						while(TRUE){
							int l = fscanf(file, "%s", buffer);
							if(l == EOF)
								break;
							n_giorno = atoi(buffer);
							
							l = fscanf(file, "%s", buffer);
							if(l == EOF)
								break;
							giorno = atoi(buffer);
							
							l = fscanf(file, "%s", buffer);
							if(l == EOF)
								break;
							mese = atoi(buffer);
							
							l = fscanf(file, "%s", buffer);
							if(l == EOF)
								break;
							anno = atoi(buffer);
							
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
							}
							
							if(l == EOF)
								break;
							
							fillListaEstrazioni(&ex, n_giorno, giorno, mese, anno, hour, min, sec, ex_ruote, m, 0);
						}
	
						fclose(file);
						
						/******************************************************************/
						//aprire il file delle giocate in lettura
						file = fopen("UsersGiocate.txt", "r");
						if(file == NULL){
							printf("Impossibile creare il file degli utenti registrati!\n");
						}
					
						//se ci sono dati memorizzati
						while(1){
							r = NULL;
							num = NULL;
							impo = NULL;
							
							//leggo dal file username e timestamp
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
							
							//leggo il numero delle ruote
							l = fscanf(file, "%s", buffer);
							if(l == EOF)
								break;						
							n = atoi(buffer);
							for(j = 0; j < n; j++){
								//leggo le ruote
								l = fscanf(file, "%s", ruota);
								if(l == EOF)
									break;
								fillListaRuote(&r, ruota);
							}
							
							//leggo il numero dei numeri
							l = fscanf(file, "%s", buffer);
							if(l == EOF)
								break;
							n = atoi(buffer);
							for(j = 0; j < n; j++){
								//leggo i numeri
								l = fscanf(file, "%s", buffer);
								if(l == EOF)
									break;
								numero = atoi(buffer);
								fillListaNumeri(&num, numero);
							}
							
							//leggo il numero degli importi
							l = fscanf(file, "%s", buffer);
							if(l == EOF)
								break;
							n = atoi(buffer);
							for(j = 0; j < n; j++){
								//leggo gli importi
								l = fscanf(file, "%s", buffer);
								if(l == EOF)
									break;				
								importo = atof(buffer);
								fillListaImporti(&impo, importo, j+1);
							}
							
							registerGiocata(&giocate, username, ng, day, month, year, ora, minuti, secondi, r, num, impo, 0);

							if(l == EOF)
								break;
						}
						
						fclose(file);

						/******************************************************************/
									
						memset(&cl_ip, '\0', sizeof(cl_ip));
					
						/******************************************************************/
				        //apertura file bloccati
						//aprire il file degli ip bloccati
						/*file = fopen("ClientBlocked.txt", "r");
						if(file == NULL){
							printf("Impossibile creare il file degli utenti bloccati!\n");
						}
					
						//current timestamp 
						time(&t);
						time_block = localtime(&t);
						
					
						//se ci sono dati memorizzati
						while(1){
							int l = fscanf(file, "%s%s", cl_ip, t_block);
							if(l == EOF)
								break;
						
							//sono passati 30 min? 
						
								//inviare flag di sblocco al client per sbloccarlo
								//cambiare bloccati = false di cl_addr nella struttura user
						
							//controllare se cl_addr si trova all'interno del file
							if(!strcmp(cl_sk, cl_ip)){
								printf("Trovato l'ip all'interno del file degli ip bloccati\n");
								ipbloccato = 1;
							}
						}
					
						fclose(file);*/
					
						/******************************************************************/
					
						sendToClientFlag(new_fd, ipbloccato);
					
						//inviare messaggio di errore al client (mettere un flag se trovo l'ip)
						if(ipbloccato){
							//invia messaggio errore
							close(new_fd);
						}else{
					
							//ritornare al ciclo
							FD_SET(new_fd, &masterfds);			//Aggiungo il nuovo socket	
							if(new_fd == -1){
								printf("Impossibile creare una nuova socket\n");
								continue;
							}
					
							printf("Arrivato un nuovo client\n");
					
							if(new_fd > fdmax){
								fdmax = new_fd;					//Aggiorno il massimo
							}
						}
					}else{						//E' un altro socket
					
						c = recvFromClientChar(i);
					
						RichiestaFromClientCmd(c);			//in base alla lettera inviata dal client il server stampa a video la richiesta effettuata				
					
						g = giocate;
						
						/*while(g){
							if(!strcmp(g->username, username)){
								if((g->anno < anno) || 
								   ((g->anno == anno) && (g->mese < mese)) ||
							       ((g->anno == anno) && (g->mese == mese) && (g->giorno < giorno)) ||
							       (((g->anno == anno) && (g->mese == mese) && (g->giorno == giorno)) && 
							       ((periodo >= 3600) && (g->ora < hour) && (g->min < minuti + ((periodo / 60) - 60))) || 
								   ((periodo < 3600) && (g->ora < hour) && (g->min > minuti + (60 - (periodo / 60)))) || 
								   ((g->ora == hour) && (g->min < minuti + ((periodo / 60) - 60))))
								  ){
									registerGiocata(&vecchie, g->username, n_giorno, giorno, mese, anno, ora, min, sec, g->r, g->num, g->impo, 0);
								}else{
								    registerGiocata(&nuove, g->username, g->nome_giorno, g->giorno, g->mese, g->anno, g->ora, g->min, g->sec, g->r, g->num, g->impo, 0);
								}
							}
							g = g->next;
						}*/
						
						/*file = fopen("Estrazioni.txt", "r");
						if(file == NULL){
							printf("Impossibile aprire il file!\n");
						}
	
						while(TRUE){
							int l = fscanf(file, "%s", buffer);
							if(l == EOF)
								break;
							n_giorno = atoi(buffer);
							
							l = fscanf(file, "%s", buffer);
							if(l == EOF)
								break;
							giorno = atoi(buffer);
							
							l = fscanf(file, "%s", buffer);
							if(l == EOF)
								break;
							mese = atoi(buffer);
							
							l = fscanf(file, "%s", buffer);
							if(l == EOF)
								break;
							anno = atoi(buffer);
							
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
							}
							
							if(l == EOF)
								break;
							
							fillListaEstrazioni(&ex, n_giorno, giorno, mese, anno, hour, min, sec, ex_ruote, m, 0);
						}
	
						fclose(file);*/
					
						switch(c){
							case 's' : 
								recvFromClient(i, buffer);          //buffer = username
								strcpy(username, buffer);
						
								check = checkUsername(&users, username);
					
								recvFromClient(i, buffer);			//buffer = password
								strcpy(password, buffer);
					
								if(!check){
								
									sendToClientFlag(i, check);
								
									registerUsers(cl_sk, &busers, username, password, 1);
								
								}else{
									sendToClientFlag(i, check);
									printf("Username gia' in uso\n");
								}
								break;
							case 'l' :
								// !login
								recvFromClient(i, buffer);          //username = buffer
								strcpy(username, buffer);
								
								recvFromClient(i, buffer);			//password = buffer
								strcpy(password, buffer);
								
								check = checkUsPsw(&users, username, password);
								
								sendToClientFlag(i, check);
							
								if(check){
									memset(&id, '\0', sizeof(id));
								
									loginUsers(&users, username, password, id);
								
									//invio del session id al client
									sendTo(i, id);
								}else{
									printf("Il client non e' registrato o ha digitato username o password errate!\n");
								
									/********************************************************************************/
								
									/*memset(&t_block, '\0', sizeof(t_block));
								
									struct user *p = users;
									while(p){
										inet_ntop(AF_INET, &(p->cl_ip.sin_addr), cl_ip, INET_ADDRSTRLEN);
										//trovo l'ip nella lista users, se e' uguale a quello del socket
										if(!strcmp(cl_sk, cl_ip)){
											//decrementare tentativi del client che è collegato
											p->tentativi--;
										
											//invio tentativi al client così che possa tenerne traccia
											sendToInt(i, p->tentativi);
										
											//quando arriva a 0
											if(p->tentativi == 0){
											
												//setto il flag di blocco
												p->blocked = true;
											
												//inserimento ip nella struttura bloccati
												time(&t);
												time_block = localtime(&t);
												

												//scrivere ip nel file bloccati.txt
												createFileBlocked(cl_sk, t_block);
											
												//invio messaggio al client del timestamp
												printf("I tentativi sono arrivati a 0!\n");
												close(i);
											}
										}
										p = p->next;
									}*/
									/********************************************************************************/
								}
								break;
							case 'i' :
								// !invia_giocata 	
								r = NULL;
								num = NULL;
								impo = NULL;								

								memset(&buffer, '\0', sizeof(buffer));
							
								do{
									recvFromClient(i, buffer);				//gli arriva una ruota
								
									if((strcmp(buffer, "-n")) != 0){
										//inserimento nella struttura
										strcpy(ruota, buffer);
										fillListaRuote(&r, ruota);
									}
								
								}while((strcmp(buffer, "-n")) != 0);
								
								do{
									recvFromClient(i, buffer);				//gli arriva un numero
								
									if((strcmp(buffer, "-i")) != 0){
										//inserimento nella struttura
										numero = atoi(buffer);
										fillListaNumeri(&num, numero);
									}
								}while((strcmp(buffer, "-i")) != 0);
							
								cont = 1;
							
								do{	
									recvFromClient(i, buffer);				//gli arriva un importo

									if((strcmp(buffer, "fine")) != 0){
										//inserimento nella struttura
										importo = atof(buffer);
										fillListaImporti(&impo, importo, cont);
										cont++;
									}
								}while((strcmp(buffer, "fine")) != 0);
								
								memset(&buffer, '\0', sizeof(buffer));
							
								//current timestamp 
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
								//printf("anno : %d", anno);
								strftime(buffer, 1024, "%H", l_time);
								ora = atoi(buffer);
								strftime(buffer, 1024, "%M", l_time);
								minuti = atoi(buffer);
								strftime(buffer, 1024, "%S", l_time);
								secondi = atoi(buffer);
						
								registerGiocata(&giocate, username, ng, day, month, year, ora, minuti, secondi, r, num, impo, 1);
							
								sendToClientFlag(i, 1);
							
								break;
							case 'g' :
								// !vedi_giocate
								memset(&buffer, '\0', sizeof(buffer));
								vecchie = NULL;
								nuove = NULL;
								
								flag = recvFromInt(i);
								j = 0;
								g = giocate;
							
								/*while(g){
									if(!strcmp(g->username, username)){
										if((g->anno < anno) || 
										   ((g->anno == anno) && (g->mese < mese)) ||
										   ((g->anno == anno) && (g->mese == mese) && (g->giorno < giorno)) ||
										   (((g->anno == anno) && (g->mese == mese) && (g->giorno == giorno)) && 
										   ((periodo >= 3600) && (g->ora < hour) && (g->min < minuti + ((periodo / 60) - 60))) || 
										   ((periodo < 3600) && (g->ora < hour) && (g->min > minuti + (60 - (periodo / 60)))) || 
										   ((g->ora == hour) && (g->min < minuti + ((periodo / 60) - 60))))
										){
											registerGiocata(&vecchie, g->username, n_giorno, giorno, mese, anno, ora, min, sec, g->r, g->num, g->impo, 0);
										}else{
											registerGiocata(&nuove, g->username, g->nome_giorno, g->giorno, g->mese, g->anno, g->ora, g->min, g->sec, g->r, g->num, g->impo, 0);
										}
									}
									g = g->next;
								}*/
								
								if(!flag){
									g = vecchie;
									while(g){
										j++;
										g = g->next;
									}
								}else{
									g = nuove;
									while(g){
										j++;
										g = g->next;
									}
								}
								
								sendToInt(i, j);
								
								//giocate passate
								if(!flag){
									g = vecchie;
									while(g){
										//scorro la lista delle giocate
										//in base all'username 
										sendToClientGiocate(i, g);
										g = g->next;
									}
								}else{
									g = nuove;
									while(g){
										sendToClientGiocate(i, g);
										g = g->next;
									}
								}						
							
								break;
							case 'e' :
								// !vedi_estrazione
								
								memset(&buffer, '\0', sizeof(buffer));
								memset(&nb, '\0', sizeof(nb));
								memset(&ruota, '\0', sizeof(ruota));
								cont = 0;
								
								ch = recvFromClientChar(i);
								
								recvFromClient(i, buffer);			//gli arriva n
								numero = atoi(buffer);				//conversione strint to int
								
								p = ex;
								
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
								
								//se ch è \n mi aspetto di ricevere la ruota dal client
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
								// !vedi_vincite
								g = vecchie;
								cont = 0;
								q = 0;
								s = 0;
								
								//numero delle giocate di username
								while(g){
									if(!strcmp(g->username, username))
										cont++;
									g = g->next;
								}
								
								sendToInt(i, cont);
								
								g = vecchie;
								cont = 0;
								
								while(g){
									if(!strcmp(g->username, username)){
										r0 = NULL;
										num0 = NULL;
										impo0 = NULL;
										
										/*sendToInt(i, g->giorno);
										sendToInt(i, g->mese);
										sendToInt(i, g->anno);
										sendToInt(i, g->ora);
										sendToInt(i, g->min);*/
										
										//conteggio delle ruote
										for(r = g->r; r; r = r->next){
											q++;
										}
										
										//numero delle ruote
										sendToInt(i, q);
										
										q = 0;

										for(r = g->r; r; r = r->next){
											p = ex;
											
											for(j = 0; j < 11; j++){
												if(!strcmp(r->nome_ruota, p->nome_ruote[j])){
													break;
												}
											}
											
											cont = 0;
											//conteggio dei numeri giocati che sono vincenti
											for(num = g->num; num; num = num->next){
												for(k = 0; k < 5; k++){
													if(p->g_vincenti[j][k] == num->numero){
														cont++;
													}
												}												
											}
											
											//numero dei numeri beccati
											sendToInt(i, cont);
											
											if(cont > 0){
												for(j = 0; j < 11; j++){
													if(!strcmp(r->nome_ruota, p->nome_ruote[j])){
														//nome della ruota
														fillListaRuote(&r0, r->nome_ruota);
														sendTo(i, r->nome_ruota);
														break;
													}
												}
											
												for(num = g->num; num; num = num->next){
													for(k = 0; k < 5; k++){
														if(p->g_vincenti[j][k] == num->numero){
															fillListaNumeri(&num0, num->numero);
															sendToInt(i, num->numero);
														}
													}												
												}
												
												for(impo = g->impo; impo; impo = impo->next){
													if((impo->tipo == 1 && impo->importo > 0) || 
													   (impo->tipo == 2 && impo->importo > 0) ||
													   (impo->tipo == 3 && impo->importo > 0) ||
													   (impo->tipo == 4 && impo->importo > 0) ||
													   (impo->tipo == 5 && impo->importo > 0)
													){
														s++;
													}
												}
												
												sendToInt(i, s);
												
												s = 0;
												
												importo = 0.0;
												
												for(impo = g->impo; impo; impo = impo->next){
													sendToInt(i, impo->tipo);
													//ha preso un numero su 5
													if(cont == 1){
														if(impo->tipo == 1 && impo->importo > 0){
															importo = impo->importo * 11.23;
															//printf("importo : %f\n", importo);
															check = 1;
														}else{
															importo = 0.0;
															check = 0;
														}
													//ha preso due numeri su 5
													}else if(cont == 2){
														if(impo->tipo == 1 && impo->importo > 0){
															importo = impo->importo * 11.23;
															//printf("importo : %f\n", importo);
															check = 1;
														}else if(impo->tipo == 2 && impo->importo > 0){
															importo = impo->importo * 250.00;
															//printf("importo : %f\n", importo);
															check = 1;
														}else{
															importo = 0.0;
															check = 0;
														}
													//ha preso tre numeri su 5
													}else if(cont == 3){
														if(impo->tipo == 1 && impo->importo > 0){
															importo = impo->importo * 11.23;
															check = 1;
														}else if(impo->tipo == 2 && impo->importo > 0){
															importo = (impo->importo * 250.00) / (combinazioniNoRep(3, 2));
															check = 1;
														}else if(impo->tipo == 3 && impo->importo > 0){
															importo = impo->importo * 4500.00;
															check = 1;
														}else{
															importo = 0.0;
															check = 0;
														}
													//ha preso quattro numeri su 5
													}else if(cont == 4){
														if(impo->tipo == 1 && impo->importo > 0){
															importo = impo->importo * 11.23;
															check = 1;
														}else if(impo->tipo == 2 && impo->importo > 0){
															importo = (impo->importo * 250.00) / (combinazioniNoRep(4, 2));
															check = 1;
														}else if(impo->tipo == 3 && impo->importo > 0){
															importo = (impo->importo * 4500.00) / (combinazioniNoRep(4, 3));
															check = 1;
														}else if(impo->tipo == 4 && impo->importo > 0){
															importo = impo->importo * 120000.00;
															check = 1;
														}else{
															importo = 0.0;
															check = 0;
														}
													//ha preso cinque numeri su 5
													}else if(cont == 5){
														if(impo->tipo == 1 && impo->importo > 0){
															importo = impo->importo * 11.23;
															check = 1;
														}else if(impo->tipo == 2 && impo->importo > 0){
															importo = (impo->importo * 250.00) / (combinazioniNoRep(5, 2));
															check = 1;
														}else if(impo->tipo == 3 && impo->importo > 0){
															importo = (impo->importo * 4500.00) / (combinazioniNoRep(5, 3));
															check = 1;
														}else if(impo->tipo == 4 && impo->importo > 0){
															importo = (impo->importo * 120000.00) / (combinazioniNoRep(5, 4));
															check = 1;
														}else if(impo->tipo == 5 && impo->importo > 0){
															importo = impo->importo * 6000000.00;
															check = 1;
														}else{
															importo = 0.0;
															check = 0;
														}
													}
													
													sendToClientFlag(i, check);
													fillListaImporti(&impo0, importo, impo->tipo);
						
													if(check){
														gcvt(importo, 11, buffer);
														sendTo(i, buffer);
													}
												}
											}
											
										}
										registerVincita(&vincite, g->username, r0, num0, impo0);
										
										
									}
									g = g->next;
								}
								
								v = vincite;
								estratto = 0.0;
								ambo = 0.0;
								terno = 0.0;
								quaterna = 0.0;
								cinquina = 0.0;
									
								//inviare le somme vinte per ogni tipo
								while(v){
									if(!strcmp(v->username, username)){
										for(impo0 = v->impo; impo0; impo0 = impo0->next){
											if(impo0->tipo == 1)
												estratto += impo0->importo;
											else if(impo0->tipo == 2)
												ambo += impo0->importo;
											else if(impo0->tipo == 3)
												terno += impo0->importo;
											else if(impo0->tipo == 4)
												quaterna += impo0->importo;
											else if(impo0->tipo == 5)
												cinquina += impo0->importo;
										}
									}
									v = v->next;
								}
										
								gcvt(estratto, 11, buffer);
								sendTo(i, buffer);
								gcvt(ambo, 11, buffer);
								sendTo(i, buffer);
								gcvt(terno, 11, buffer);
								sendTo(i, buffer);
								gcvt(quaterna, 11, buffer);
								sendTo(i, buffer);
								gcvt(cinquina, 11, buffer);
								sendTo(i, buffer);
								
								break;
							case 'q' :
								// !esci
								check = logoutUsers(&users, username);
								sendToClientFlag(i, check);
								if(check)
									close(i);
								break;
							default :
								printf("Ricevuto comando errato!\n");
								break;
						}
					}
				}
			}
		}
		close(sock);
	}
	return 0;
}
