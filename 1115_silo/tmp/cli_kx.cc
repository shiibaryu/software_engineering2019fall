#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netdb.h>
#include <signal.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <time.h>
#include "kernel.h"
#include "port.h"
//#include "host.h"
//#include "macro.h"
#include <iostream>
#define FIN "fin"

using namespace std;
char QUERY_DSMS_SERVER_HOST[BUFSIZ];

static int Sfd; // Socket file descriptor

// This must NOT be static although I do not know the exact reason.
// I guess this will be referred by readline library functions which are external.
string Cmd[] = {"show", "help", "exit", "stream", "run", "create", "register", "query"};

static char **myCompletion(const char *, int, int);

static int 
connectCli(const int port, const char hostname[])
{
	int sockfd;
	struct hostent *ent;
	struct sockaddr_in addr;

	bzero((char *)&addr, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	if ((ent = gethostbyname(hostname)) == NULL) {
		SSS(hostname);
		ERR;
	}
	bcopy(ent->h_addr, (char *)&addr.sin_addr, ent->h_length);
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) ERR;
	if (connect(sockfd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) == -1) {
		close(sockfd);
		cout << "Connection refused. Check whether your server is running" << endl;
		exit(1);
	}

	return sockfd;
}

static void 
sighandler(int sig)
{
	switch (sig) {
		case SIGPIPE: cout << "SIGPIPE" << endl;
			break;
		case SIGINT:  cout << "SIGINT"  << endl;
		case SIGKILL: cout << "SIGKILL" << endl;
		case SIGTERM: cout << "SIGTERM" << endl;
		case SIGTSTP: cout << "SIGTSTP" << endl;
		case SIGUSR1: cout << "SIGUSR1" << endl;
		case SIGSEGV: cout << "SIGSEGV" << endl;
			exit(1); 
			break;
	}
}

static void
setSignal(void)
{
	signal(SIGPIPE, sighandler);
	//signal(SIGINT, sighandler); 
	signal(SIGSEGV, sighandler); 
	signal(SIGUSR1, sighandler); 
}

char *
dupstr(char* s) 
{
	char *r;

	if (!(r = (char *)malloc((strlen(s) + 1)))) ERR;
	strcpy(r, s);

	return (r);
}

static char * 
myGenerator(const char* text, int state)
{
	static int index, len;
	char *name;

	if (!state) {
		index = 0;
		len = strlen(text);
	}

	while ((name = ((char *)Cmd[index].c_str()))) {
		index++;
		if (!strncmp(name, text, len))
			return dupstr(name);
	}

	return NULL;
}

static char ** 
myCompletion(const char* text, int start, int end)
{
	char **matches;

	matches = (char **)NULL;
	if (start == 0) {
		matches = rl_completion_matches(text, myGenerator);
	}

	return matches;
}

void
execTransaction(int sfd, char query[])
{
  int ack;

  if (send(sfd, query, BUFSIZ, 0) == -1) ERR;    
  if (recv(sfd, &ack, sizeof(int), MSG_WAITALL) == -1) ERR;
}

static void
execCmd(char cmd[])
{
  if (!strcmp(cmd, "exit")) exit(0);
  else {
    execTransaction(Sfd, cmd);
    cout << "COMMIT" << endl;
  }
}

static void
interactive(void)
{
  char cmd[BUFSIZ];
  char *str;

  rl_attempted_completion_function = myCompletion;
  while (true) {
    str = readline("CMD> ");
    rl_bind_key('\t', rl_complete);
    if (!str) {
      delete str; 
      continue;
    }
 
    else if (str[0] == '\0') {
      delete str; 
      continue;
    }

    add_history(str);
    bzero(cmd, sizeof(cmd)); 
    strcpy(cmd, str); 
    delete str;

    execCmd(cmd); // Sfd is already closed.
  }
}

void
init(void)
{
  setSignal();
}


extern int
main(int argc, char *argv[])
{
  int opt;
  char host[BUFSIZ];
  char *cmd = NULL;

  init();
  //strcpy(host, "localhost");
  gethostname(host, sizeof(host));

  while ((opt = getopt(argc, argv, "h:c:")) != -1){
    switch(opt){
			case 'h': 
				strcpy(host, optarg); 
				break;
			case 'c': 
				cmd = new char[BUFSIZ];
				strcpy(cmd, optarg);
				break;
			default: 
				break;    
    }
  }

  // Host
  Sfd = connectCli(DBMS_PORT, "localhost");    

  // Execute 
  if (!cmd) interactive();
  else { 
    execCmd(cmd); 
    delete cmd;
  }

  return 0;
}
