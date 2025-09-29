/* hello_signal.c */
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <stdbool.h>

bool return_to_main = false;

void handler(int signum) //signal handler
{ 
  printf("Hello World!\n");
  return_to_main = true;
}

int main(int argc, char * argv[])
{
  signal(SIGALRM,handler); //register handler to handle SIGALRM
  alarm(1);
  
  while(1){ //loop indefinitely to print
  if(return_to_main){ 
    printf("Turing was right!\n");
    return_to_main=false;
    alarm(1);
  }
  };

  return 0; 
}