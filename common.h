#pragma once

#include<stdlib.h>
#include <arpa/inet.h>

void logexit(const char *msg);


int addrparse(const char *addrstr, const char *portstr, 
              struct sockaddr_storage *storage);

void addrtostr(const struct sockaddr *addr, char *str, size_t strsize);

int server_sockaddr_init(const char *proto, const char* portstr, 
                        struct sockaddr_storage *storage);


void formatTextWithTime(int clientId, char* message);

void extractTextInQuotes(const char* input, char* extracted_text);

void formatTextToSender(int ReceiverId, const char* message, char* final_message, char* allOrPrivate);

void formatTextToOthers(int clientId, const char* message, char* final_message,char* allOrPrivate);

void formatJoinMessage(int userId, char* message);

void print_ids(const char* response, char* user_list);

int containsNULL(const char *message);

void extractIDsAndMessage(const char *message, int *id1, int *id2, char *msg);

void formatarMSG(const char *message, int id1, int id2, char *msg);

void req_rem_format(char *str);
