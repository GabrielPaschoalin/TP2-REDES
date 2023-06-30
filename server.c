#include "common.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h> 
#include <sys/socket.h>
#include <pthread.h>
#include <time.h>

void usage(int argc, char **argv){

    printf("usage: %s <v4|v6> <server port>\n", argv[0]);
    printf("example: %s v4 51511\n", argv[0]);
    exit(EXIT_FAILURE);
}

#define BUFSZ 1024
#define MAX_CLIENTS 16
int listaId[MAX_CLIENTS];
int connectedUsers;

struct client_data{
    int csock;
    struct sockaddr_storage storage;
    int userId;
};

struct client_data *clientList[MAX_CLIENTS];

void sendMessageToClient(int clientSocket, const char* message) {
    ssize_t bytesSent = send(clientSocket, message, strlen(message), 0);
    if (bytesSent == -1) {
        perror("Error sending message to client");
    }
}

void send_to_all_clients(const char *message) {
    for (int i = 1; i < MAX_CLIENTS; i++) {
        if (listaId[i] == 1) {
            send(clientList[i]->csock, message, strlen(message), 0);
        }
    }
}

void send_to_all_clients_except(const char *message, int excluded) {
    for (int i = 1; i < MAX_CLIENTS; i++) {
        if (listaId[i] == 1 && clientList[i]->userId != excluded) {
            send(clientList[i]->csock, message, strlen(message), 0);
        }
    }
}

void listUsers (struct client_data *clientSocket){

    char saida[30] = "";
    char new [4];

    for (int i = 1; i <= 15; i++){
        if (listaId[i] == 1 && clientSocket->userId != i){
            if(i < 10){
                sprintf(new, "0%i ", i);
            }else{
                sprintf(new, "%i ", i);
            }

            strcat(saida, new);
        }
    }

    sendMessageToClient(clientSocket->csock, saida);

}

void closeConnection (struct client_data *clientSocket){
    
    send_to_all_clients_except("Client disconnected", clientSocket->userId);

    printf("User 0%i removed", clientSocket->userId);

    listaId[clientSocket->userId] = 0;

    sendMessageToClient(clientSocket->csock, "Removed Successfully");

}

void sendAll (const char * buf, struct client_data *sender){
    
    char message[BUFSZ];
    extractTextInQuotes(buf, message);

    char message_sender[BUFSZ];
    formatTextToSender(0, message, message_sender,"All");
    sendMessageToClient(sender->csock, message_sender);

    char message_receiver[BUFSZ];
    formatTextToOthers(sender->userId, message, message_receiver, "All");
    send_to_all_clients_except(message_receiver, sender->userId);   

}

void sendPrivate (const char * buf, struct client_data *sender){

    char message[BUFSZ];
    extractTextInQuotes(buf, message);

    int receiver =  extractReceiver(buf);

    char message_sender[BUFSZ];
    formatTextToSender(receiver, message, message_sender, "Private");

    sendMessageToClient(sender->csock, message_sender);

    char message_receiver[BUFSZ];
    formatTextToOthers(sender->userId, message, message_receiver, "Private");
    
    sendMessageToClient(clientList[receiver]->csock, message_receiver);

}

void * client_thread(void *data){

    struct client_data *cdata = (struct client_data *)data;
    struct sockaddr *caddr = (struct sockaddr *)(&cdata->storage);   

    char caddrstr[BUFSZ];
    addrtostr(caddr, caddrstr, BUFSZ);    

    if(cdata->userId < 10){
        printf("User 0%i added\n", cdata->userId);
    }else{
        printf("User %i added\n", cdata->userId);
    }
    
    clientList[cdata->userId] = cdata;

    char mensagem[BUFSZ];
    memset(mensagem, 0, BUFSZ);
    if(cdata->userId < 10){
        sprintf(mensagem, "User 0%i joined the group!", cdata->userId);
    }else{
        sprintf(mensagem, "User %i joined the group!", cdata->userId);
    }

    send_to_all_clients(mensagem);

    char buf[BUFSZ];
    memset(buf, 0, BUFSZ);
    
    while (1){
        

        size_t count = recv(cdata->csock, buf, BUFSZ-1, 0);
        if (count == -1){
            listaId[cdata->userId] = 0;
            break;
        }
        printf("[msg] %s, %d bytes: %s\n", caddrstr, (int)count, buf);

        if (strncmp(buf, "close connection", 16) == 0){

            closeConnection (cdata);
            connectedUsers --;
            break;
        }
        else if(strncmp(buf, "list users", 10) == 0){
            listUsers(cdata);
        }
        else if(strncmp(buf, "send all", 8) == 0){
            sendAll(buf, cdata);    
        }
        else if(strncmp(buf, "send to", 7) == 0){
            sendPrivate(buf, cdata);
        }

    }

    close(cdata->csock);
    pthread_exit(EXIT_SUCCESS);
}

int defineID(){
    for(int i = 1; i < MAX_CLIENTS ; i++){
        if(listaId[i] == 0){
            listaId[i] = 1;
            return i;
        }
    }
    return 0;
}

int main (int argc, char **argv){

    if(argc < 3){
        usage(argc,argv);
    }

    //Cria o socket
    struct sockaddr_storage storage;
    if (0 != server_sockaddr_init(argv[1], argv[2], &storage)){
        usage(argc, argv);
    }

    int s;
    s = socket(storage.ss_family, SOCK_STREAM, 0);
    if (s == -1){
        logexit("socket");
    }

    int enable = 1;
    if (0 != setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int))){
        logexit("setsocket");
    }

    struct sockaddr *addr = (struct sockaddr *)(&storage);
    //bind
    if (0 != bind(s, addr, sizeof(storage))){
        logexit("bind");
    }

    //listen
    if (0 != listen(s, 10)){
        logexit("listen");
    }

    for (int i = 0; i < MAX_CLIENTS; i++) {
    clientList[i] = NULL;
    }

    char addrstr[BUFSZ];
    addrtostr(addr, addrstr, BUFSZ);    
    printf("bound to %s, waiting conections \n", addrstr);

    while (1){
        
        struct sockaddr_storage cstorage;
        struct sockaddr *caddr = (struct sockaddr *)(&storage);
        socklen_t caddrlen = sizeof(cstorage);

        int csock = accept(s, caddr, &caddrlen);
        if (csock == -1){
            logexit("accept");
        }

        if (connectedUsers >= 15) {

            // Close the socket immediately to prevent the server from accepting more connections.
            sendMessageToClient(csock, "User limit exceeded");
            close(csock); // Close the client socket
        }
        else{
            //Cliente Chegou
            connectedUsers ++;
            struct client_data *cdata = malloc(sizeof(*cdata));

            if (!cdata){
                logexit("malloc");
            }
            cdata->csock = csock;
            memcpy(&(cdata->storage), &storage, sizeof(storage));

            cdata->userId = defineID();    
            
            pthread_t tid;
            pthread_create(&tid, NULL, client_thread, cdata);
        }
        

    }

    exit(EXIT_SUCCESS);

}