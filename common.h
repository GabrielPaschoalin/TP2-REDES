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

int extractReceiver(const char* str);

void extractTextInQuotes(const char* input, char* extracted_text);

void formatTextToSender(int ReceiverId, const char* message, char* final_message, char* allOrPrivate);

void formatTextToOthers(int clientId, const char* message, char* final_message,char* allOrPrivate);


