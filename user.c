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

void* receive_handler(void* socket_desc) {
    int sock = *(int*)socket_desc;
    char buf[BUFSZ];
    
    // Receive server response
    while (recv(sock, buf, sizeof(buf), 0) > 0) {
        printf("%s\n", buf);
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
        char comando[BUFSZ];
        fgets(comando, BUFSZ - 1, stdin);

        send(s, comando, strlen(comando), 0);
        
        if(strncmp(comando, "close connection", 16) == 0){
            break;
        }
        
    }
    
    exit(EXIT_SUCCESS);

}