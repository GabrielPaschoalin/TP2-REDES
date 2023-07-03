#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <time.h>

void logexit(const char *msg){
    perror(msg);
    exit(EXIT_FAILURE);
}


int addrparse(const char *addrstr, const char *portstr, struct sockaddr_storage *storage)
{

    if (addrstr == NULL || portstr == NULL)
    {
        return -1;
    }

    // porta de acesso, converter portstr de string para int e deposi para uint16_t
    uint16_t port = (uint16_t)atoi(portstr); // unsigned short

    if (port == 0)
    {
        return -1;
    }

    port = htons(port); // número da porta precisa estar como Big Endian

    // Verificar se é IPv4

    struct in_addr inaddr4; // 32-bit IP address
    if (inet_pton(AF_INET, addrstr, &inaddr4))
    {
        struct sockaddr_in *addr4 = (struct sockaddr_in *)storage;
        addr4->sin_family = AF_INET;
        addr4->sin_port = port;
        addr4->sin_addr = inaddr4;
        return 0;
    }

    // Verificar se é IPv6

    struct in6_addr inaddr6; // 128-bit IPv6 address
    if (inet_pton(AF_INET6, addrstr, &inaddr6))
    {
        struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)storage;
        addr6->sin6_family = AF_INET6;
        addr6->sin6_port = port;
        // addr6->sin6_addr = inaddr6;
        memcpy(&(addr6->sin6_addr), &inaddr6, sizeof(inaddr6));
        return 0;
    }

    return -1;
}

void addrtostr(const struct sockaddr *addr, char *str, size_t strsize)
{
    int version;
    char addrstr[INET6_ADDRSTRLEN + 1] = "";
    uint16_t port;

    if (addr->sa_family == AF_INET)
    { // IPv4

        version = 4;
        struct sockaddr_in *addr4 = (struct sockaddr_in *)addr; // Conversão de tipo
        if (!inet_ntop(AF_INET, &(addr4->sin_addr), addrstr, INET6_ADDRSTRLEN + 1))
        {
            logexit("ntop");
        }

        port = ntohs(addr4->sin_port); // network to host short
    }

    else if (addr->sa_family == AF_INET6)
    { // IPv6

        version = 6;
        struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)addr; // Conversão de tipo
        if (!inet_ntop(AF_INET6, &(addr6->sin6_addr), addrstr, INET6_ADDRSTRLEN + 1))
        {
            logexit("ntop");
        }

        port = ntohs(addr6->sin6_port); // network to host short
    }

    else
    {
        logexit("unknown protocol family.");
    }

    if (str)
    {
        snprintf(str, strsize, "IPv%d %s %hu", version, addrstr, port);
    }
}

int server_sockaddr_init(const char *proto, const char* portstr, 
                        struct sockaddr_storage *storage){

    
    uint16_t port = (uint16_t)atoi(portstr); // unsigned short
    if(port == 0){
        return -1;
    }

    port = htons(port); // número da porta precisa estar como Big Endian
 
    memset(storage, 0, sizeof(*storage));

    if (0 == strcmp(proto, "v4")){ //IPv4

        struct sockaddr_in *addr4 = (struct sockaddr_in *)storage; // Conversão de tipo
        addr4->sin_family = AF_INET;
        addr4->sin_addr.s_addr = INADDR_ANY; //rota em todos os endereços que o computador possua
        addr4->sin_port = port;
        return 0;

    }else if (0 == strcmp(proto, "v6")){

        struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)storage; // Conversão de tipo
        addr6->sin6_family = AF_INET6;
        addr6->sin6_addr = in6addr_any; //rota em todos os endereços que o computador possua
        addr6->sin6_port = port;        
        return 0;

    }else{
        return -1;
    }
}

//---------------------------------------------------------------------------------

#define BUFSZ 1024
#define MAX_CLIENTS 16

void extractTextInQuotes(const char* input, char* extracted_text) {
    const char* start = strchr(input, '"');
    const char* end = strrchr(input, '"');
    if (start != NULL && end != NULL && start < end) {
        strncpy(extracted_text, start + 1, end - start - 1);
        extracted_text[end - start - 1] = '\0';
    } else {
        extracted_text[0] = '\0';
    }

}

void formatTextToOthers(int clientId, const char* message, char* final_message,char* allOrPrivate) {
    time_t currentTime;
    struct tm *localTime;

    // Get the current time in seconds since the epoch
    currentTime = time(NULL);

    // Convert the current time to local time (broken-down time components)
    localTime = localtime(&currentTime);

    // Format the time as HH:MM into the formattedTime array
    char formattedTime[6];
    strftime(formattedTime, sizeof(formattedTime), "%H:%M", localTime);

    // Format the final text using the formatted time, client id, and message
    if(strncmp(allOrPrivate,"All", 3) == 0){
        snprintf(final_message, BUFSZ, "[%s] %02d: %s", formattedTime, clientId, message);
    }else if(strncmp(allOrPrivate,"Private", 7) == 0){
        snprintf(final_message, BUFSZ, "P[%s] %02d: %s", formattedTime, clientId, message);
    }


}

void formatTextToSender(int ReceiverId, const char* message, char* final_message, char* allOrPrivate) {
    time_t currentTime;
    struct tm *localTime;

    // Get the current time in seconds since the epoch
    currentTime = time(NULL);

    // Convert the current time to local time (broken-down time components)
    localTime = localtime(&currentTime);

    // Format the time as HH:MM into the formattedTime array
    char formattedTime[6];
    strftime(formattedTime, sizeof(formattedTime), "%H:%M", localTime);

    // Format the final text using the formatted time, client id, and message
    if(strncmp(allOrPrivate,"All", 3) == 0){
        snprintf(final_message, BUFSZ, "[%s]->all: %s", formattedTime, message);
    }
    else if (strncmp(allOrPrivate,"Private", 7) == 0){
        snprintf(final_message, BUFSZ, "P[%s]->0%i: %s", formattedTime, ReceiverId, message);
    }
}

void formatJoinMessage(int userId, char* message) {
    if (userId < 10) {
        sprintf(message, "User 0%i joined the group!", userId);
    } else {
        sprintf(message, "User %i joined the group!", userId);
    }
}

void print_ids(const char* response, char* user_list) {
    int length = strlen(response);
    if (response[length - 1] == '\n')
        length--;

    int count = 0;
    for (int i = 0; i < length; i++) {
        if (response[i] >= '0' && response[i] <= '9') {
            user_list[count++] = response[i];
            user_list[count++] = response[i + 1];
            user_list[count++] = ' ';
            i++;
        }
    }
    user_list[count - 1] = '\0';

}

int containsNULL(const char *message) {
    // Check if the message contains ",NULL,"
    const char *check = strstr(message, ",NULL,");
    return (check == NULL) ? 1 : 0;
}

void extractIDsAndMessage(const char *message, int *id1, int *id2, char *msg) {
    if (containsNULL(message)) {
        sscanf(message, "MSG(%d,%d,%[^)])", id1, id2, msg);
    } else {
        sscanf(message, "MSG(%d,NULL,%[^)])", id1, msg);
        *id2 = -1;
    }
}

void formatarMSG(const char *message, int id1, int id2, char *msg){

    if(id2 == -1)
        sprintf(msg, "MSG(%i,NULL,%s)", id1, message);
    else
        sprintf(msg, "MSG(%i,%i,%s)", id1, id2 , message);

}

void req_rem_format(char *str) {
    const char *prefix = "REQ_REM: ";
    size_t prefix_len = strlen(prefix);
    size_t str_len = strlen(str);

    // Verifica se a string começa com o prefixo "REQ_REM: "
    if (strncmp(str, prefix, prefix_len) == 0) {
        // Move o restante da string para o início da mesma (incluindo o caractere nulo '\0')
        memmove(str, str + prefix_len, str_len - prefix_len + 1);
    }
}
