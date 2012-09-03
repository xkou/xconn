#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <signal.h>

void handle_sigint( int sig ){
	
}

int main(int argc, char **argv){
	char *server = argv[1];
	int port = atoi(argv[2]);
	int fport = atoi(argv[3]);
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	struct hostent * ht = gethostbyname( server );
	if(ht){
		struct sockaddr_in in;
		in.sin_family = AF_INET;
		in.sin_port = htons(port);
		memcpy(&in.sin_addr, ht->h_addr_list[0], 4);
		int ret = connect( sock, (struct sockaddr*)&in, sizeof in);
		if(ret!=0){
			fprintf( stderr, "connnect: %s:%d, error %d, sock %d\n", inet_ntoa(in.sin_addr), port, ret, sock );
			return 1;
		}
	}
	char buf[100];
	sprintf(buf, "U%05d", fport);
	write(sock, buf, strlen(buf));
	
	signal(SIGINT, handle_sigint);
	fcntl(sock, F_SETFL, (fcntl(sock, F_GETFL) | O_NONBLOCK) );

	struct epoll_event stdinev;
	stdinev.events = EPOLLIN | EPOLLERR;
	stdinev.data.fd = 0;

	struct epoll_event sockev;
	sockev.events = EPOLLIN | EPOLLERR;
	sockev.data.fd = sock;
	
	int ep = epoll_create(2);
	epoll_ctl(ep, EPOLL_CTL_ADD, sock, &sockev );
	epoll_ctl(ep, EPOLL_CTL_ADD, 0, &stdinev );
	
	struct epoll_event evs[2];
	char *data = malloc(1024 * 200);
	while(1){
		int n = epoll_wait(ep, evs, 2, 1000);
		if(n == 0) continue;
		if(n < 0) return 3;
		for(int i=0; i<n; i++){
			int fd = evs[i].data.fd;
			int rn = read(fd, data, 1024 * 200);
			if(rn <= 0) return 4;
			if(fd == 0) write(sock, data, rn);
			else if(fd == sock) write(1, data, rn);
		}
	}
	
	return 2;
}
