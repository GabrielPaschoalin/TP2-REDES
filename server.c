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
            fflush(stdout);
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

void listUsers(int client_socket, int client_id) {
    char user_list[BUFSZ];
    memset(user_list, 0, BUFSZ);

    for (int i = 1; i < MAX_CLIENTS; i++) {
        if (listaId[i] == 1 && i != client_id) {
            char user_id[4];
            sprintf(user_id, "0%i ", i);
            strcat(user_list, user_id);
        }
    }

    char response[BUFSZ];
    snprintf(response, BUFSZ + 10, "RES_LIST(%s)", user_list);
    send(client_socket, response, strlen(response), 0);
}

void send_to_all_clients_except_user_left(int user_id) {
    char message[BUFSZ];
    snprintf(message, BUFSZ, "REQ_REM: User %i left the group!\n", user_id);
    send_to_all_clients_except(message, user_id);
}

void closeConnection (struct client_data *client){

    int removedId = client->userId;

    printf("User 0%i removed\n", removedId);
    
    listaId[removedId] = 0;
    send_to_all_clients_except_user_left(removedId);

    sendMessageToClient(client->csock, "OK(01)");

    close(client->csock);
    free(client);
    pthread_exit(EXIT_SUCCESS);
}

void sendAll (const char *message,  int sender){
    
    char message_sender[BUFSZ];
    formatTextToSender(0, message, message_sender,"All");
    
    char message_network[BUFSZ];
    formatarMSG(message_sender,sender,-1,message_network);
    sendMessageToClient(clientList[sender]->csock, message_network);

    char message_receiver[BUFSZ];
    formatTextToOthers(clientList[sender]->userId, message, message_receiver, "All");
    formatarMSG(message_receiver,sender,-1,message_network);
    send_to_all_clients_except(message_network, clientList[sender]->userId);   

}

void sendPrivate (const char *message,  int sender, int receiver){

    char message_sender[BUFSZ];
    formatTextToSender(receiver, message, message_sender, "Private");
    
    char message_network[BUFSZ];
    formatarMSG(message_sender, sender, receiver, message_network);

    sendMessageToClient(clientList[sender]->csock, message_network);

    char message_receiver[BUFSZ];
    formatTextToOthers(clientList[sender]->userId, message, message_receiver, "Private");
    
    formatarMSG(message_receiver, sender, receiver, message_network);

    sendMessageToClient(clientList[receiver]->csock, message_network);

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

    char mensagem[BUFSZ-50];
    char envio[BUFSZ];
    memset(mensagem, 0, BUFSZ);
    memset(envio, 0, BUFSZ);

    formatJoinMessage(cdata->userId, mensagem);
    
    //sprintf(envio, "MSG(%i, NULL, %s)", cdata->userId, mensagem);
    formatarMSG(mensagem, cdata->userId, -1, envio);

    send_to_all_clients(envio);


    char buf[BUFSZ];
    memset(buf, 0, BUFSZ);
    
    while (1){
        size_t count = recv(cdata->csock, buf, BUFSZ-1, 0);
        if (count == -1){
            listaId[cdata->userId] = 0;
            break;
        }

        if (strncmp(buf, "REQ_REM", 7) == 0){
            closeConnection(cdata); // cdata is the client_data pointer for the user being removed
            connectedUsers--;
        }
        else if(strncmp(buf, "list users", 10) == 0){
            listUsers(cdata->csock, cdata->userId);
        }
        else if(strncmp(buf, "MSG", 3) == 0){
            

            int* id1 = (int*)malloc(sizeof(int));;
            int* id2 = (int*)malloc(sizeof(int));;
            char msg [BUFSZ];

            extractIDsAndMessage(buf, id1, id2, msg);
            
            if(containsNULL(buf) && listaId[*id2] == 1){ //Privada
                sendPrivate(msg, *id1, *id2);
            }else if (!containsNULL(buf)){
                sendAll(msg, *id1);
            }else{
                char erro[15] = "ERROR(03)";
                sendMessageToClient(clientList[*id1]->csock, erro);
            }
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

        if (connectedUsers >= MAX_CLIENTS - 1) {

            // Close the socket immediately to prevent the server from accepting more connections.
            sendMessageToClient(csock, "ERROR(01)");
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