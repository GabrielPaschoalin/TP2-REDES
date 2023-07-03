#include "common.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h> 
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>

void usage(int argc, char **argv){

    printf("usage: %s <server IP> <server port>\n", argv[0]);
    printf("example: %s 127.0.0.1 51511\n", argv[0]);
    exit(EXIT_FAILURE);
}

#define BUFSZ 1024
#define MAX_USERS 15

int userIdGlobal = -1;

void* receive_handler(void* socket_desc) {
    int sock = *(int*)socket_desc;
    
    char buf[BUFSZ];
    
    // Receive server response
    while (recv(sock, buf, sizeof(buf), 0) > 0) {
    
        if (strcmp(buf, "ERROR(01)") == 0) {
            printf("User limit exceeded\n");
            close(sock);
            exit(EXIT_SUCCESS);
        }else if(strncmp(buf, "REQ_REM", 7) == 0){
            int msg_id;
            sscanf(buf, "REQ_REM(%d)", &msg_id);
            printf("User %i left the group!", msg_id);
        }else if (strncmp(buf, "MSG", 3) == 0) {
            
            char message[BUFSZ];
            int* id1 = (int*)malloc(sizeof(int));;
            int* id2 = (int*)malloc(sizeof(int));;

            extractIDsAndMessage(buf,id1,id2,message); 

            if(containsNULL(buf)){ //Privada
                printf("%s\n", message);
                
            }else{
                if(userIdGlobal == -1){
                    userIdGlobal = *id1;
                }
                printf("%s\n", message);
            }

        }else if (strncmp(buf, "RES_LIST", sizeof("RES_LIST")-1) == 0){
            //listUsers
            char user_list[BUFSZ];
            print_ids(buf, user_list);
            printf("%s\n", user_list);
        }else if(strcmp(buf, "OK(01)") == 0){
            printf("Removed Successfully");
        }else if(strcmp(buf, "ERROR(02)") == 0){
            printf("User not found");
        }else if(strcmp(buf, "ERROR(03)") == 0){
            printf("Receiver not found");
        }

        memset(buf, 0, sizeof(buf));

    }
    // Close the socket when done receiving
    close(sock);
    
    return NULL;
}


int main(int argc, char **argv){

    if (argc < 3){
        usage (argc,argv);
    }

    struct sockaddr_storage storage;
    if (0 != addrparse(argv[1], argv[2], &storage)){
        usage(argc, argv);
    }

    //Declaração do Socket
    int s;
    s = socket(storage.ss_family, SOCK_STREAM, 0);

    if (s == -1){
        logexit("socket");
    }

    struct sockaddr *addr = (struct sockaddr *)(&storage);

    //Conectar ao servidor
     if (0 != connect(s, addr, sizeof(storage))){
        logexit("connect");
    }

    char addrstr[BUFSZ];
    addrtostr(addr, addrstr, BUFSZ);
    printf("connected to %s \n", addrstr);

    char buf[BUFSZ];
    memset(buf,0,BUFSZ);

    // Create a thread to receive server responses

    pthread_t receive_thread;

    if (pthread_create(&receive_thread, NULL, receive_handler, (void*)&s) < 0) {
        perror("Thread creation failed");
        exit(1);
    }


    while (1)
    {
        // Comando
        char comando[BUFSZ-50];
        fgets(comando, BUFSZ - 1, stdin);

        char message_send [BUFSZ+30];
        
        if(strncmp(comando, "close connection", 16) == 0){
            sprintf(message_send, "REQ_REM(%i)", userIdGlobal );
            send(s, message_send, strlen(message_send), 0);
            break;
        }  
        else if(strncmp(comando, "send to", 7)==0){

            char text [BUFSZ];
            extractTextInQuotes(comando, text);
            
            int receiver_id;
            sscanf(comando, "send to %d", &receiver_id);

            formatarMSG(text,userIdGlobal,receiver_id,message_send);
                        
            send(s, message_send, strlen(comando), 0);
            
        }else if(strncmp(comando, "send all", 8) == 0){

            char text [BUFSZ];
            extractTextInQuotes(comando, text);
        
            formatarMSG(text,userIdGlobal,-1,message_send);
        
            send(s, message_send, strlen(comando), 0);

        }else{
            printf("invalid command\n");
            continue;
        }


    }
    
    exit(EXIT_SUCCESS);

}