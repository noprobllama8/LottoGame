/***************************Strutture Dati Server****************************/

//puntatore della struttura di memorizzazione degli utenti registrati
struct user *users;					
struct vincita *vincite;
pid_t pid;
int piped[2], sock;

//struttura di memorizzazione degli utenti registrati
struct user{
	//struct sockaddr_in cl_ip;
	
	//ip del client 
	//dichiarato di tipo char perchè viene stampato solo su file e per controllato al momento dell'accesso
	//per avvisare di un eventuale blocco  		
	char cl_ip[16];					
	char sessionId[11];				//id di sessione assegnato all'username al login
	char username[1024];			//username che fornisce il client al server
	char password[1024];			//password che fornisce il client al server
	
	//tentativi di login da parte del client
	//max tentativi = 3
	//viene decrementato ad ogni accesso errato, se (tentativi == 0) -> blocco del client
	int tentativi;					
	
	//flag che rappresenta se l'username è online
	//online = 0 -> l'user è offline, lo sarà ad ogni primo accesso e dopo uno sblocco
	//online = 1 -> l'user è online, lo sarà all'invio del comando !login
	bool online;					
	 
	//flag che rappresenta se il client è bloccato
	//blocked = 0 -> il client non è bloccato, lo sarà ad ogni prima connessione e dopo uno sblocco
	//blocked = 1 -> il clien è bloccato, lo sarà al primo blocco e gli sarà vietato di collegarsi al server
	bool blocked;                  
	
	//indica i secondi Unix del blocco
	time_t secBlocco;				
	
	struct user *next;
};

//struttura di memorizzazione delle giocate effettuate dai client con i loro username
struct giocata{
	char username[1024];
	
	//timestamp :
	//	1) della giocata quando il client la invia
	//  2) dell'estrazione quando deve essere processata per una presunta vincita
	int nome_giorno;
	int giorno;
	int mese;
	int anno;
	int ora;
	int min;
	int sec;

	//puntatore di struttura delle ruote inserite nella giocata
	struct ruote *r;				
	
	//puntatore di struttura dei numeri inseriti nella giocata
	struct numeri *num;				
	
	//puntatori di struttura degli importi inseriti nella giocata
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
	double importo;
	int tipo;						//1 = estratto - 2 = ambo - 3 = terno - 4 = quaterna - 5 = cinquina
	struct importi *next;
};

struct estrazioni{
	
	//timestamp dell'estrazione
	int nome_giorno;
	int giorno;
	int mese;
	int anno;
	int ora;
	int min;
	int sec;
	
	char* nome_ruote[11];
	int g_vincenti[11][5];			//giocate vincenti di tutte le ruote
	struct estrazioni *next;
};

struct vincita{
	//timestamp dell'estrazione dove si è manifestata la vincita
	int giorno;
	int mese;
	int anno;
	int ora;
	int min;
	
	char username[1024];
	char nome_ruota[11];				
	struct numeri *num;	
	struct importi *impo;  			
	struct vincita *next;
};

/****************************************************************************/

/****************Funzioni condivise tra Server e Client**********************/

void sendTo(int sd, char *buffer);
void sendToInt(int sd, int i);
int recvFromInt(int sd);

/****************************************************************************/

/*****************************Funzioni Server********************************/
void intHandler(int sign);
void sendToClientFlag(int sd, bool flag);
void sendToClientGiocate(int sd, struct giocata *g);
char recvFromClientChar(int sd);
void recvFromClient(int sd, char *buffer);
bool checkUsername(struct user **p0, char *username);
bool checkUsPsw(struct user **p0, char *username, char *password);
struct user* buildListUsers(struct user **p0);
void generaIDsession(char *sessionId);
void loginUsers(int sd, struct user **p0, char *username, char *password, char *id);
bool logoutUsers(struct user **p0, char *username);
void createFileRegister(struct user *p0);
void createFileBlocked(char *cl_addr, time_t seconds);
void createFileGiocate(struct giocata *g0);
void createFileVincite(struct vincita *v0);
void createFileEstrazioni(int n_giorno, int giorno, int mese, int anno, int hour, int min, int sec, int **m);
void registerGiocata(struct giocata **g0, char *username, int n_giorno, int giorno, int mese, int anno, int hour, int min, int sec, struct ruote *r0, struct numeri *n0, struct importi *i0, bool mode);
void registerUsers(char *cl_addr, struct user **p0, char *username, char *password, bool stato);
void registerVincita(struct vincita **v0, char *username, int giorno, int mese, int anno, int hour, int min, char *ruota, struct numeri *n0, struct importi *i0, bool mode);
void fillListaRuote(struct ruote **r0, char *ruota);
void fillListaNumeri(struct numeri **n0, int numero);
void fillListaImporti(struct importi **i0, double importo, int tipo);
void fillListaEstrazioni(struct estrazioni **e0, int n_giorno, int giorno, int mese, int anno, int hour, int min, int sec, char **ruota, int **m);
void RichiestaFromClientCmd(char c);
int fatt(int n);
double combinazioniNoRep(int n, int k);
void pipeToFather(int piped, int n_giorno, int giorno, int mese, int anno, int hour, int min, int sec, int **m);
void AssegnaVincita(struct giocata *vecchie, struct vincita **vincite, char **ruota, int **m);
void AssegnaGiocata(struct giocata *vecchie, struct vincita **v, struct giocata *nuove, char **ex_ruote, int** m, int n_giorno, int giorno, int mese, int anno, int hour, int min, int sec);
void blockUser(int i, struct user *u0, char *cl_sk);
void openFileGiocate(struct giocata **g0);
void openFileRegister(struct user **u0);
bool openFileBlocked(char *cl_sk);
void openFileVincite(struct vincita **v0);
void openFileEstrazioni(struct estrazioni *e0, char **ex_ruote, int **m);
void UnlockClient(char *cl_sk, struct user *u0);

/****************************************************************************/

/*******************************Strutture Dati Client************************/

bool bloccato;
int tentativi, mode;
char sessionId[11];

/****************************************************************************/

/******************************Funzioni Client*******************************/

void welcome();
char AssegnaLettera(char *cmd);
bool fillListaRuote_Cl(struct ruote **r0, char *ruota);
bool fillListaNumeri_Cl(struct numeri **n0, int numero);
bool checkRuota(char *ruota);
void stampaIstruzioneHelp(char *command);
void sendToServerChar(int sd, char c);
bool recvFromServerFlag(int sd);
void recvFromServer(int sd, int stato);
void SendToServerCmd(int sd, char c);

/****************************************************************************/