#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "potato.h"

using namespace std;
static int socket_fd;//socket between ringmaster and player
static int right_socket_fd;//socket between itself and
static struct addrinfo *host_info_list_server;
static int left_socket_fd;
static int right_listen_socket_fd;
static struct addrinfo *host_info_list_client;
static int baseport;
static int totalnum;
static int ownid;
static int rightid;
static int leftid;
static Potato potato;
//static char * hostname;
void asServer(int ownport){

  //  cout<<"as server port is "<<ownport<<endl;
  int status;
  //static int asServer_socket_fd;
  struct addrinfo host_info;
  //static struct addrinfo *host_info_list_server;
  char *hostname = "127.0.0.1";
  char port[5];
  port[4]='\0';
  sprintf(port, "%d", ownport);

  memset(&host_info, 0, sizeof(host_info));

  host_info.ai_family   = AF_UNSPEC;
  host_info.ai_socktype = SOCK_STREAM;
  host_info.ai_flags    = AI_PASSIVE;

  status = getaddrinfo(hostname, port, &host_info, &host_info_list_server);
  if (status != 0) {
    cerr << "Error: cannot get address info for host" << endl;
    cerr << "  (" << hostname << "," << port << ")" << endl;
    return;
  } //if

  right_listen_socket_fd = socket(host_info_list_server->ai_family,
				  host_info_list_server->ai_socktype,
				  host_info_list_server->ai_protocol);
  if (right_listen_socket_fd == -1) {
    cerr << "Error: cannot create socket" << endl;
    cerr << "  (" << hostname << "," << port << ")" << endl;
    return;
  } //if

  int yes = 1;
  status = setsockopt(right_listen_socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
  status = bind(right_listen_socket_fd, host_info_list_server->ai_addr, host_info_list_server->ai_addrlen);
  if (status == -1) {
    cerr << "Error: cannot bind socket" << endl;
    cerr << "  (" << hostname << "," << port << ")" << endl;
    return;
  } //if

  status = listen(right_listen_socket_fd, 100);
  if (status == -1) {
    cerr << "Error: cannot listen on socket" << endl;
    cerr << "  (" << hostname << "," << port << ")" << endl;
    return;
  }

  //cout << "Waiting for connection on port " << port << endl;

  if(ownid==totalnum-1){
    //the last player, tell master that the frist player can connect

    //    printf("I am the last player\n");
    char *  mess = "please tell first player";
    send(socket_fd, mess, 24, 0);
  }

  struct sockaddr_storage socket_addr;
  socklen_t socket_addr_len = sizeof(socket_addr);
  int client_connection_fd;
  client_connection_fd = accept(right_listen_socket_fd, (struct sockaddr *)&socket_addr, &socket_addr_len);
  right_socket_fd=client_connection_fd;
  if (client_connection_fd == -1) {
    cerr << "Error: cannot accept connection on socket" << endl;
    return;
  }


  char buffer[512];
  recv(client_connection_fd, buffer, 24, 0);
  buffer[24] = 0;

  //cout << "received from right player: " << buffer << endl;
  send(client_connection_fd, "you connect with me", 19, 0);
  cout<<"Connected as player "<<ownid<<" out of "<<totalnum<<" total players "<<endl;
  return;
}

void asClient(int serverport){

  //  cout<<"to connect, server port is "<<serverport<<endl;
  int status;

  struct addrinfo host_info;

  char *hostname = "127.0.0.1";
  char port[5];
  port[4]='\0';
  sprintf(port, "%d", serverport);

  //cout<<"port is "<<port<<endl;

  memset(&host_info, 0, sizeof(host_info));
  host_info.ai_family   = AF_UNSPEC;
  host_info.ai_socktype = SOCK_STREAM;

  status = getaddrinfo(hostname, port, &host_info, &host_info_list_client);
  if (status != 0) {
    cerr << "Error: cannot get address info for host" << endl;
    cerr << "  (" << hostname << "," << port << ")" << endl;
    return;
  }

  left_socket_fd = socket(host_info_list_client->ai_family,
			  host_info_list_client->ai_socktype,
			  host_info_list_client->ai_protocol);
  if (left_socket_fd == -1) {
    cerr << "Error: cannot create socket" << endl;
    cerr << "  (" << hostname << "," << port << ")" << endl;
    return;
  }

  //cout << "Connecting to " << hostname << " on port " << port << "..." << endl;

  status = connect(left_socket_fd, host_info_list_client->ai_addr, host_info_list_client->ai_addrlen);
  if (status == -1) {
    cerr << "Error: cannot connect to socket" << endl;
    cerr << "  (" << hostname << "," << port << ")" << endl;
    return;
  } //if

  const char *message = "linked with right player";
  send(left_socket_fd, message, strlen(message), 0);
  char buf2[512];
  recv(left_socket_fd, buf2, 19, 0);
  buf2[19]='\0';
  //  cout<<"receive from my left player:"<<buf2<<endl;
  return;
}


void sendPotato(Potato  pot){
  int tosendid;
  int tosend_fd;

  srand((unsigned)time(NULL));
  int opt=rand()%2;
  if(opt==0){

    //choose the left player
    tosendid=leftid;
    tosend_fd=left_socket_fd;
  }else{

    //choose the right player
    tosendid=rightid;
    tosend_fd=right_socket_fd;
  }
  int status= send(tosend_fd, &pot, sizeof(potato),0);
  //  cout<<"Sending potato to "<<tosendid<<endl;
  if (status <0)
    perror("error is sendPotato");
  return;


}

int chooseSend(){
  int tosendid;
  int tosend_fd;

  srand((unsigned)time(NULL));
  int opt=rand()%2;
  if(opt==0){

    //choose the left player
    tosendid=leftid;
    tosend_fd=left_socket_fd;
    //cout<<"Sending potato to "<<tosendid<<endl;
  }else{

    //choose the right player
    tosendid=rightid;
    tosend_fd=right_socket_fd;
    //cout<<"Sending potato to "<<tosendid<<endl;
  }
  return tosend_fd;

}



int main(int argc, char* argv[]){
  if(argc < 3){

    cerr << "please run palyer in the correct format as player <machine_name> <port_num>\n" << endl;

    return -1;
  }
  int status;
  int baseport=0;
  struct addrinfo host_info;
  struct addrinfo *host_info_list;
  const char *hostname = argv[1];
  const char *port     = argv[2];
  char connectinfo[1024] = {0};//receive the connection info from ringmaster
  typedef struct _connectinfo{
    int ownid;
    int leftid;
    int rightid;
    int baseport;
    int total;
  }Connectinfo;


  memset(&host_info, 0, sizeof(host_info));
  host_info.ai_family   = AF_UNSPEC;
  host_info.ai_socktype = SOCK_STREAM;

  status = getaddrinfo(hostname, port, &host_info, &host_info_list);
  if (status != 0) {
    cerr << "Error: cannot get address info for host" << endl;
    cerr << "  (" << hostname << "," << port << ")" << endl;
    return -1;
  }

  socket_fd = socket(host_info_list->ai_family,
		     host_info_list->ai_socktype,
		     host_info_list->ai_protocol);
  if (socket_fd == -1) {
    cerr << "Error: cannot create socket" << endl;
    cerr << "  (" << hostname << "," << port << ")" << endl;
    return -1;
  }

  //cout << "Connecting to " << hostname << " on port " << port << "..." << endl;

  status = connect(socket_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
  if (status == -1) {
    cerr << "Error: cannot connect to socket" << endl;
    cerr << "  (" << hostname << "," << port << ")" << endl;
    return -1;
  }

  Connectinfo connectRcvInfo;

  recv(socket_fd, connectinfo, sizeof(connectinfo), 0);
  memcpy(&connectRcvInfo, connectinfo, sizeof(Connectinfo));
  ownid=connectRcvInfo.ownid;
  leftid=connectRcvInfo.leftid;
  rightid=connectRcvInfo.rightid;
  baseport=connectRcvInfo.baseport;
  totalnum=connectRcvInfo.total;
 

  if(ownid==0){
    asServer(ownid+baseport);
  }else{
    asClient(leftid+baseport);
    asServer(ownid+baseport);
  }

  //let the first player connect to last player
  if(ownid==0){
    char buf[512];
    recv(socket_fd, buf, 12, 0);
    string fromMaster;
    buf[12]=0;
    fromMaster=buf;
    string s1="connect last";
    if(fromMaster.compare(s1)==0){
      asClient(leftid+baseport);
    }
  }
  //if is player 0, tell master can start the game

  if(ownid==0){
    char * messstart="can start";
    send(socket_fd, messstart, 9, 0);
  }

  //start play game
  fd_set fds;
  int maxsock=0;
  Potato recv_potato;
 
  while(1){
    FD_ZERO(&fds);
    FD_SET(socket_fd, &fds);
    FD_SET(left_socket_fd, &fds);
    FD_SET(right_socket_fd, &fds);

     if(socket_fd>maxsock){
      maxsock=socket_fd;
    }
    if(left_socket_fd>maxsock){
      maxsock=left_socket_fd;
    }
    if(right_socket_fd>maxsock){
      maxsock=right_socket_fd;
    }

    maxsock+=1;
    if(select(maxsock, &fds, NULL, NULL, NULL)==-1){
      cerr<<"select fails"<<endl;
    }

    if(FD_ISSET(socket_fd, &fds)){
      //receive message from master, master tell me to end the game
      memset(&recv_potato, 0, sizeof(recv_potato));
      size_t recvsize=recv(socket_fd, &recv_potato, sizeof(recv_potato), 0);

      if(recvsize==-1){
	cerr<<"receive potato error"<<endl;
      }
      if(recv_potato.remain==0){
	//it is the end of the game

	freeaddrinfo(host_info_list_client);
	close(left_socket_fd);
	freeaddrinfo(host_info_list_server);
	close(right_socket_fd);
	freeaddrinfo(host_info_list);
	close(socket_fd);
	return 0;
      }else{
	int remain=recv_potato.remain-1;

	recv_potato.remain=remain;
	recv_potato.trace[recv_potato.hops-recv_potato.remain-1]=ownid;
	int tosendfd=chooseSend();
	status=send(tosendfd, &recv_potato, sizeof(recv_potato), 0);
	if(status<0){
	  perror("send error");
	}
	if(tosendfd==left_socket_fd){
	  cout<<"Sending potato to "<<leftid<<endl;
	}
	if(tosendfd==right_socket_fd){
	  cout<<"Sending potato to "<<rightid<<endl;
	}
	
      }
    }
    else if(FD_ISSET(right_socket_fd, &fds)){
      //receive potato from right player
     
      memset(&recv_potato, 0, sizeof(recv_potato));
      size_t recvsize=recv(right_socket_fd, &recv_potato, sizeof(recv_potato), 0);

     
      if(recvsize==-1){
	cerr<<"receive potato error"<<endl;
      }
      if(recv_potato.remain==1){
	//it is the last player
	cout<<"I am it"<<endl;
	//send potato back to master

	int remain=recv_potato.remain-1;
	recv_potato.remain=remain;
	recv_potato.trace[recv_potato.hops-recv_potato.remain-1]=ownid;
	status=send(socket_fd, &recv_potato, sizeof(recv_potato), 0);
	if(status<0){
	  perror("send error");
	}

	//	cout<<"send back to master"<<endl;
      }else{
	int remain=recv_potato.remain-1;
	recv_potato.remain=remain;
	recv_potato.trace[recv_potato.hops-recv_potato.remain-1]=ownid;
	int tosendfd=chooseSend();
	status=send(tosendfd, &recv_potato, sizeof(recv_potato), 0);
	if(status<0){
	  perror("send error");
	}
	if(tosendfd==left_socket_fd){
	  cout<<"Sending potato to "<<leftid<<endl;
	}
	if(tosendfd==right_socket_fd){
	  cout<<"Sending potato to "<<rightid<<endl;
	}
	
	
      }
    }
    else if(FD_ISSET(left_socket_fd, &fds)){
      //receive potato from right player
     
      memset(&recv_potato, 0, sizeof(recv_potato));
      size_t recvsize=recv(left_socket_fd, &recv_potato, sizeof(recv_potato), 0);

      if(recvsize==-1){
	cerr<<"receive potato error"<<endl;
      }
      if(recv_potato.remain==1){
	//it is the last player
	cout<<"I am it"<<endl;
	//send potato back to master

	int remain=recv_potato.remain-1;
	recv_potato.remain=remain;
	recv_potato.trace[recv_potato.hops-recv_potato.remain-1]=ownid;
	status=send(socket_fd, &recv_potato, sizeof(recv_potato), 0);
	if(status<0){
	  perror("send error");
	}

      }else{
	//receive the potato and send to one neighbor
	int remain=recv_potato.remain-1;
	recv_potato.remain=remain;
	recv_potato.trace[recv_potato.hops-recv_potato.remain-1]=ownid;
	int tosendfd=chooseSend();
	status=send(tosendfd, &recv_potato, sizeof(recv_potato), 0);
	if(status<0){
	  perror("send error");
	}
	if(tosendfd==left_socket_fd){
	  cout<<"Sending potato to "<<leftid<<endl;
	}
	if(tosendfd==right_socket_fd){
	  cout<<"Sending potato to "<<rightid<<endl;
	}

      }
    }
    else{
    }



  }





  freeaddrinfo(host_info_list_client);
  close(left_socket_fd);
  freeaddrinfo(host_info_list_server);
  close(right_socket_fd);
  freeaddrinfo(host_info_list);
  close(socket_fd);



  return 0;
}
