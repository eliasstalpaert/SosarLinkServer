#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include <assert.h>
#include "config.h"
#include "tcpsock.h"

#define PORT 5678
#define MAX_CONN 1  // state the max. number of connections the server will handle before exiting

#ifdef DEBUG
  #define DEBUG 1
#else
  #define DEBUG 0
#endif

/* Implements a sequential test server (only one connection at the same time)
 */

void notify_empty(tcpsock_t*);
void notify_not_empty(tcpsock_t*);
void send_message(char*, tcpsock_t*);
void send_log_data(tcpsock_t*, FILE*);

int main( void )
{
  tcpsock_t * server, * client;
  sensor_data_t data;
  int bytes, result, size;
  FILE* fp = fopen("log","r+");


  printf("Test server is started\n");
  if (tcp_passive_open(&server,PORT)!=TCP_NO_ERROR) exit(EXIT_FAILURE);
  do
  {
    printf("Waiting for connection\n");
    if (tcp_wait_for_connection(server,&client)!=TCP_NO_ERROR) exit(EXIT_FAILURE);
    printf("Incoming client connection\n");

    fp = fopen("log","r+");
    if(NULL == fp) {
      if(DEBUG) printf("Failed to open log file\n");
      exit(EXIT_FAILURE);
    }
    fseek (fp, 0, SEEK_END);
    size = ftell(fp);

    if (0 == size) {
        printf("file is empty\n");
        notify_empty(client);
    }
    else {
        notify_not_empty(client);
        send_log_data(client,fp);
        notify_empty(client);
    }
    fclose(fp);
  } while (1);
  if (tcp_close( &server )!=TCP_NO_ERROR) exit(EXIT_FAILURE);
  printf("Test server is shutting down\n");
  return 0;
}

void notify_empty(tcpsock_t* client) {
  char message[] = "\nno\n";
  printf("notifying it's empty\n");
  send_message(message, client);
}

void send_message(char* message, tcpsock_t* client) {
  int bytes = sizeof(message)*2;
  int result = tcp_send(client,(void *)message,&bytes);
  if(result != TCP_NO_ERROR){
    if(result == TCP_CONNECTION_CLOSED) {
      printf("Client closed connection\n");
    }
    else {
      if(DEBUG) fprintf(stderr, "Unexpected fail to send message\n");
    }
    tcp_close(&client);
  }
}

void notify_not_empty(tcpsock_t* client) {
  char message[] = "\nyes\n";
  printf("notifying it's not empty\n");
  send_message(message, client);
}

void send_log_data(tcpsock_t* client, FILE* fp) {
  int amount_to_read = 10;
  char message[amount_to_read+1]; //1 extra character for \n
  rewind(fp);

  while(fgets(message, amount_to_read, fp) != NULL) {
    message[amount_to_read] = '\n';
    char cat_message[amount_to_read+2];
    snprintf(cat_message, sizeof(cat_message), "\n%s", message);
    printf("sending: %s", cat_message);
    send_message(cat_message, client);
  }
  printf("Deleting old contents\n");
  freopen("log","w",fp);
  /*
  int size;
  char * message, *old_message;
  char read = fgetc(fp);
  while(fgetc(fp) != EOF) {
    size++;
    if(message == NULL){
      message = (char*)malloc(sizeof(char)*size);
      strcpy(message, &read);
    }
    else {
      //old_message = message;
      message = (char*)realloc(sizeof(char)*size);
      strcpy(message, old_message);
      free(old_message);
      strcat(message, &read);
    }
    read = fgetc(fp);
    //snprintf(message, sizeof(message), "\n%c\n", read);
    //printf("sending: %s", message);
    //send_message(message,client);
  }
  char* real_message = (char*)malloc(sizeof(char)*(size+2));
  snprintf(real_message, sizeof(real_message), "\n%s\n", message);
  free(message);
  printf("sending: %s", real_message);
  send_message(real_message, client);
  free(real_message);
  */
}
