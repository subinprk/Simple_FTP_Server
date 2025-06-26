#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/select.h>
#include <signal.h>
#include <fcntl.h>

#define SERV_TCP_PORT /*port number*/
#define SERV_ADDR /*ip of your computer*/
#define MAX_CLIENTS 50
#define LOGIN_FILE "login.txt"

struct client{
        char   name[20];  // this client's name
        char   age[5];      // this client's age as a string
        char   partner[20];     // the socket number of the chatting partner of this client
        int    state; // the state of this client
        int    login_trial;
     };
struct client cli[MAX_CLIENTS]; //max 50 clients

typedef enum{
   STATE_INIT = 0,
   STATE_NAME = 2,
   STATE_READY = 3,
   STATE_PARTNER = 4,
   STATE_CHAT = 1,
   STATE_END = -1
} state_t;

void  handle_protocol(int x, fd_set *pset);
int   handle_state_1(int x, fd_set * pset, char *buf);
int   handle_state_2(int x, fd_set * pset, char *buf);
// int   handle_state_age(int x, fd_set * pset, char *buf, int state[]);
int   handle_state_name(int x, fd_set * pset, char *buf);
int   handle_state_ready(int x, fd_set * pset, char *buf);
int   handle_chat_partner(int x, fd_set * pset, char *buf);
int   handle_state_chat(int x, fd_set * pset, char *buf);
void  handle_state_3(int x, fd_set * pset, char *buf);
bool  is_partner(int x, int i);
int   list_partners(int x);
int   check_login(char *name);


int s1; // make s1 global so the signal handler can access it

void handle_sigint(int sig) {
   printf("\nCaught signal %d, closing server socket and exiting...\n", sig);
   close(s1);
   exit(0);
}

int main(){
   int s2, i, x, y;
   struct sockaddr_in serv_addr, cli_addr;
   socklen_t  xx;

   printf("Hi, I am the server\n");

   // Register signal handler for SIGINT (Ctrl-C)
   signal(SIGINT, handle_sigint);

   bzero((char *)&serv_addr, sizeof(serv_addr));
   serv_addr.sin_family=PF_INET;
   serv_addr.sin_addr.s_addr=inet_addr(SERV_ADDR);
   serv_addr.sin_port=htons(SERV_TCP_PORT);

   //open a tcp socket
   if ((s1=socket(PF_INET, SOCK_STREAM, 0))<0){
     printf("socket creation error\n");
     exit(1);
   }
   printf("socket opened successfully. socket num is %d\n", s1);
      //added this line to allow reusing the address
      int optval = 1;
      setsockopt(s1, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
   // bind ip
   x =bind(s1, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
   if (x < 0){
     printf("binding failed\n");
     exit(1);
   }
   printf("binding passed\n");
   listen(s1, 5);
   xx = sizeof(cli_addr);

   // pset remembers all sockets to monitor
   // rset is the copy of pset passed to select
   fd_set rset, pset;
   int maxfd=MAX_CLIENTS; // just monitor max 50 sockets

   FD_ZERO(&rset); // init rset
   FD_ZERO(&pset); // init pset

   // step 1. s1 is a socket to accept new connection request. 
   // monitor connection request packet at s1
   FD_SET(s1, &pset);
   // and loop on select
   for(;;){ 
     rset=pset;  // step 2
     select(maxfd, &rset, NULL, NULL, NULL); // step 3
     // now we have some packets
     for(x=0;x<maxfd;x++){ // check which socket has a packet
       if (FD_ISSET(x, &rset)){ // skcket x has a packet
         // s1 is a special skcket for which we have to do "accept"            
         if (x==s1){ // new client has arrived
            // create a socket for this client
            s2 = accept(s1, (struct sockaddr *)&cli_addr, &xx);
            cli[s2].state = STATE_INIT;
            printf("new cli at socket %d\n",s2);
            FD_SET(s2, &pset); // and include this socket in pset
         }else{ // if x is not s1, it must be already connected client.
            handle_protocol(x, &pset);
         }
       }
     }
   }
}

void handle_protocol(int x, fd_set * pset){
// we have data packet in socket x. state[x] shows the state of socket x.
   int y; char buf[100];
   y=read(x, buf, 100); // read the data
   if (y <= 0){
      printf("socket %d disconnected\n", x);
      close(x);   // end the connection
      FD_CLR(x, pset); // remove from the watch list. 
                        // we don't monitor socket x any more
      return;
   }
   buf[y]=0; // make it a string
   printf("we have received %s at socket %d in state %d\n", buf, x, cli[x].state);
   if (cli[x].state==STATE_INIT){ // the state of this socket is 1 meaning we are
                    // expecting "ping" from this socket
      if(handle_state_1(x, pset, buf)){
         cli[x].state = STATE_NAME;
         write(x, "name? ", 6);
      }
   }else if (cli[x].state==STATE_NAME){
      int login = handle_state_name(x, pset, buf);
      if(!login){
         cli[x].state = STATE_READY;
         write(x, "ready? ", 7);
      }
      else if (login == 2) {
         //cli[x].state = STATE_END; // if login failed, go to end state
      }
      else if (login == 1) {
         cli[x].state = STATE_END; // if login failed, go to end state
         write(x, "Login failed. Disconnecting.", 30);
      }
   }
   else if (cli[x].state == STATE_READY){
	   if(handle_state_ready(x, pset, buf)){
         cli[x].state = STATE_PARTNER;
         write(x, "partner? ", 9);
         list_partners(x);
      }
   }
   else if (cli[x].state == STATE_PARTNER){
	   handle_chat_partner(x, pset, buf);
      cli[x].state = STATE_CHAT;
      write(x, "start chatting", 15);
   }
   else if (cli[x].state == STATE_CHAT){
	   handle_state_chat(x, pset, buf);
   }
   else if (cli[x].state == STATE_END){
      close(x);
      FD_CLR(x, pset);
   }
}

int handle_state_1(int x, fd_set *pset, char* buf){
// socket x is in state 1. Expecting "ping" in buf. if we have ping, send "pong" and
// just update state[x]=2; otherwise send error message and disconnect the connection
     if (strcmp(buf, "hello")==0){ // yes we have "hello"
         printf("yes it is hello\n");
         cli[x].state=2; // now we are waiting for "pang" from this client
         return 1;
     }else{ // no we didn't receive "hello"
         printf("protocol error. disconnected skt %d\n", x);
         write(x, "protocol error", 14); // send err message to the client
         close(x);   // end the connection
         FD_CLR(x, pset); // remove from the watch list. 
                         // we don't monitor socket x any more
         return  0;
     }
}
// int handle_state_2(int x, fd_set *pset, char* buf){
// // socket x is in state 2. we are expecting "pang" in buf. If we have "pang", send "pung"
// // and close the connection. If we didn’t receive “pang”, send “protocol error” to the
// // client and disconnect.
// printf("socket %d is in state 2 waiting for pang\n", x);
//      if (strcmp(buf, "pang")==0){ // yes we have "pang"
//         printf("yes it is pang. send pung\n");
//         write(x, "pung, name?", 11);  // send pong to this client
//         cli[x].state = 3;
//         return 1;
//      }else{ // no we didn't receive "pang"
//         printf("protocol error. disconnected skt %d\n", x);
//         write(x, "protocol error", 14); // send err message to the client
//         close(x);   // end the connection
//         FD_CLR(x, pset); // remove from the watch list. 
//                          // we don't monitor socket x any more
//          return 0;
//      }
// }

int   check_login(char *name){
   FILE *fp = fopen(LOGIN_FILE, "r");
   if(fp == NULL) {
      perror("Failed to open login file");
      return -1; // Error opening file
   }
   char *line = NULL;
   size_t len = 0;
   ssize_t read_bytes;
   while((read_bytes = getline(&line, &len, fp)) != -1){
      if (line[read_bytes - 1] == '\n') {
         line[read_bytes - 1] = '\0';
      }
      if (strcmp(line, name) == 0) {
         free(line);
         fclose(fp);
         return 1;
      }
   }
   if (line)
      free(line);
   fclose(fp);
   return 0;
}

void show_cli_list(int x){
   for (int i = 0; i < MAX_CLIENTS; i ++){
      if (i != x && cli[i].state == STATE_CHAT){
         write(x, cli[i].name, strlen(cli[i].name));
      }
   }
}

int handle_state_name(int x, fd_set *pset, char *buf){
   if (cli[x].login_trial >= 5) {
      write(x, "Too many login attempts. Disconnecting.", 40);
      // cli[x].state = STATE_END;
      return 1; // login failed
   }
   if (check_login(buf) == 0){
      write(x, "Login failed. Try again.", 25);
      cli[x].login_trial ++;
      return 2;
   }
	strcpy(cli[x].name, buf);
	// write(x, "age?", 4);
	printf("cli %d's name is : %s\n", x, cli[x].name);
   return 0;
}

// int handle_state_age(int x, fd_set *pset, char *buf, int state[]){
// 	strcpy(cli[x].age, buf);
// 	printf("cli %d's age is: %s\n", x, cli[x].age);
//    return 1;
// }

int handle_state_ready(int x, fd_set *pset, char *buf){
   if (strcmp(buf, "yes")==0){
      printf("client %d is ready\n", x);
      cli[x].state=STATE_PARTNER;
      return 1;
   }else{ // no we didn't receive "ping"
      printf("protocol error. disconnected skt %d\n", x);
      write(x, "protocol error", 14); // send err message to the client
      close(x);   // end the connection
      FD_CLR(x, pset); // remove from the watch list. 
                         // we don't monitor socket x any more
      return 0;
     }
}

int list_partners(int x){
   static char buf2[1024];
   buf2[0] = 0; // clear the buffer
   for (int i = 0; i < MAX_CLIENTS; i ++){
      if (i != x && cli[i].state == STATE_CHAT){
         strcat(buf2, cli[i].name);
         strcat(buf2, " ");
      }
   }
   if (strlen(buf2) > 0) {
      write(x, buf2, strlen(buf2));
   } else {
      write(x, "No partners now", 16);
   }
}

int handle_chat_partner(int x, fd_set *pset, char *buf){
   strcpy(cli[x].partner, buf);
	printf("cli %d's partner is: %s\n", x, cli[x].partner);
   return 1;
}

bool is_partner(int x, int i){
	if (strcmp(cli[i].name, cli[x].partner) == 0)
		return true;
	return false;
}

int handle_state_chat(int x, fd_set *pset, char *buf){

   if(strcmp(buf, "leave") == 0){
      cli[x].state = 0;
      close(x);   // end the connection
      FD_CLR(x, pset); // remove from the watch list. 
             // we don't monitor socket x any more
      return 1;
   }
	for (int i = 0; i < MAX_CLIENTS; i ++){
		if (i != x && cli[i].state == STATE_CHAT && is_partner(x, i)){
			write(i, buf, strlen(buf));
		}
	}
   return 1;
}
