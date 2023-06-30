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
    const char* first_quote = strchr(input, '"');
    const char* second_quote = strchr(first_quote + 1, '"');
    size_t length = second_quote - first_quote - 1;

    strncpy(extracted_text, first_quote + 1, length);
    extracted_text[length] = '\0';


}

int extractReceiver(const char* str) {
    while (*str != '\0') {
        if (*str >= '1' && *str <= '9') {
            int digit = *str - '0'; // Convert the character to an integer
            if (digit >= 1 && digit <= 15) {
                return digit; // Return the digit if it is in the range 1 to 15
            }
        }
        str++; // Move to the next character in the string
    }
    return -1; // Return -1 if the desired digit is not found
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