#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define TRUE 1

void sendTo(int sd, char *buffer){
	
	int len, ret;
	char dim[2];
	
	len = strlen(buffer)+1;					//valore len
	
	memset(&dim, '\0', sizeof(dim));
	sprintf(dim, "%d", len);            	//int to string
	
	ret = send(sd, (void*)dim, 2, 0);
	if(ret < 0){
		perror("Send errata!\n");
		exit(1);
	}
	
	ret = send(sd, (void*)buffer, len, 0);
	if(ret < 0){
		perror("Send errata!\n");
		exit(1);
	}
}

void sendToInt(int sd, int i){
	int ret;
	char dim[3];
	
	memset(&dim, '\0', sizeof(dim));
	
	sprintf(dim, "%d", i);

	ret = send(sd, (void*)dim, 3, 0);
	if(ret < 0){
		perror("Send errata!\n");
		exit(1);
	}
}

int recvFromInt(int sd){
	int ret, i;
	char dim[3];
	
	memset(&dim, '\0', sizeof(dim));
	
	ret = recv(sd, (void*)dim, 3, 0);
	if(ret < 0){
		perror("Receive errata!\n");
		exit(1);
	}else{
		i = atoi(dim);
	}
	return i;
}
