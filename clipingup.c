#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/select.h>
#include <signal.h>

#define SERV_TCP_PORT /*port number*/
#define SERV_ADDR /*ip of your computer*/

int main(){
   int x,y;
   struct sockaddr_in serv_addr;
   char buf[50];
   printf("Hi, I am the client\n");
   
   bzero((char *)&serv_addr, sizeof(serv_addr));
   serv_addr.sin_family=PF_INET;
   serv_addr.sin_addr.s_addr=inet_addr(SERV_ADDR);
   serv_addr.sin_port=htons(SERV_TCP_PORT);

   //open a tcp socket
   if ((x=socket(PF_INET, SOCK_STREAM, 0))<0){
      printf("socket creation error\n");
      exit(1);
   }
   printf("socket opened successfully. socket num is %d\n", x);

   //connect to the server
   if (connect(x, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0){
      printf("can't connect to the server\n");
      exit(1);
   }
   printf("connected to the server\n");

   pid_t pid = fork();
   if (pid == 0){
	   for(;;){
		fgets(buf, sizeof(buf), stdin);
		buf[strcspn(buf, "\n")] = 0;
		write(x, buf, strlen(buf));
	   }
   }else{
	   printf("started reading\n");
	   for(;;){
	   	int len = read(x, buf, sizeof(buf));
		if (len <= 0)
			break;
		buf[len] = 0;
		printf(">> %s\n", buf);
	   }
   }
  // printf("enter ping\n");
  // scanf("%s", buf);
  // write(x, buf, 4);
  // printf("we received %s. ending protocol.\n", buf);
  close(x);
}
