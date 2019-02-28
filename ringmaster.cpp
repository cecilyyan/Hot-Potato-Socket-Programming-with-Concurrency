#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>
#include "potato.h"
#include <time.h>


static Potato potato;
using namespace std;
int main(int argc, char* argv[]){

  if(argc<4){
    perror("you should run ringmaster in the correct format as ringmaster <port_num> <num_players> <num_hops>\n");
    return -1;
  }

  if(atoi(argv[2])< 2){
    perror("there must be at least one player\n");
    return EXIT_FAILURE;
  }

  if(atoi(argv[3]) > 512 || atoi(argv[3]) < 0 ){
    perror("num_hops must be greater than or equal to zero and less than or equal to 512\n");
    return EXIT_FAILURE;
  }
  int hops=atoi(argv[3]);
  int PORT = atoi(argv[1]);
  int opt = 1;
  int master_socket = 0;
  int addrlen = 0;
  int new_socket = 0;
  int num_players = atoi(argv[2]);
  int players_socket[num_players] = {0};
  int activity = 0;
  int valread = 0;
  int sd = 0;
  char buffer[1025];
  int count = 0;//used to calaulate how many players are already connected with ringmaster

  int max_sd = 0;
  struct sockaddr_in address;
  fd_set readfds;

  typedef struct _connectinfo{
    int ownid;
    int leftid;
    int rightid;
    int baseport;
    int total;
  }Connectinfo;
  Connectinfo connectinfo;

  printf("Potato Ringmaster\n");
  cout<<"Players = "<<num_players<<endl;
  cout<<"Hops = "<<hops<<endl;
  //create the master socket
  if( (master_socket = socket(AF_INET , SOCK_STREAM , 0)) == 0)
    {
      perror("create socket failed");
      exit(EXIT_FAILURE);
    }

  //allow to make mutiple connections
  if( setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt,sizeof(opt)) < 0 ){
    perror("setsockopt");
    exit(EXIT_FAILURE);
  }
  //type of socket created
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons( PORT );

  //bind the socket to port 8000
  if (bind(master_socket, (struct sockaddr *)&address, sizeof(address))<0){
    perror("bind failed");
    exit(EXIT_FAILURE);
  }



  if (listen(master_socket, 5) < 0){
    perror("listen");
    exit(EXIT_FAILURE);
  }

  //accept the incoming connection
  addrlen = sizeof(address);

 

  while(1){
    //clear the socket set
    FD_ZERO(&readfds);

    //add master socket to set
    FD_SET(master_socket, &readfds);
    max_sd = master_socket;

    for(int i=0; i<num_players; i++){
      //socket descriptor
      sd = players_socket[i];
      //if valid socket descriptor then add to read list
      if(sd > 0){
	FD_SET( sd , &readfds);
      }
      //highest file descriptor number, need it for the select function
      if(sd > max_sd){
	max_sd = sd;
      }

    }

    activity = select( max_sd + 1 , &readfds , NULL , NULL , NULL);
    if ((activity < 0) && (errno!=EINTR)){
      printf("select error");
    }
    //get the current connection
    if (FD_ISSET(master_socket, &readfds)){
      if ((new_socket = accept(master_socket,(struct sockaddr *)&address, (socklen_t*)&addrlen))<0){
	perror("accept");
	exit(EXIT_FAILURE);
      }
      count++;
      printf("Player %d is ready to play\n",count-1 );
      int ownid = count - 1;
      connectinfo.ownid=count - 1;
      connectinfo.baseport = 4000;
      connectinfo.total=num_players;
      if(count==1){
	connectinfo.rightid = ownid + 1;
	if(num_players==2){
	  connectinfo.leftid = ownid + 1;
	}else{
	  connectinfo.leftid = num_players-1;
	}
      }else if(count == num_players){
	connectinfo.rightid = 0;
	connectinfo.leftid = ownid - 1;
      }else{
	connectinfo.rightid = ownid + 1;
	connectinfo.leftid = ownid - 1;
      }
      //send the connection information to player, id, left id, right id, base port;

      if( send(new_socket, (char*)&connectinfo, sizeof(Connectinfo), 0) ){
	//	printf("send message to player\n");
      }
      //printf("send message to client\n");

      //add socket to array of sockets
      for (int i = 0; i < num_players; i++){
	//if position is empty
	if( players_socket[i] == 0 ){
	  players_socket[i] = new_socket;
	  break;
	}
      }


      int len;
      char buf[512];
      while((len=recv(players_socket[num_players-1], buf, 512, 0)) > 0 ){
	buf[len]='\0';
	//	printf("from player %d\n");
	//printf("receive: %s\n", buf);
	char * s1=buf;
	char * s2="please tell first player";
	if(strcmp(s1, s2)==0){
	  char* s3="connect last";
	  if(send(players_socket[0], s3,12, 0 )<0){
	    perror("tell first player error\n");
	    return 1;
	  }
	  break;
	}

      }


      //listen from player 0 to decide when to start
      if(count==num_players){
	int len2;
	char buf2[512];
	while((len2=recv(players_socket[0], buf2, 9, 0)) >0){
	  buf2[len2]='\0';
	  char * m1=buf2;
	  char * m2="can start";
	  if(strcmp(m1, m2)==0){
	    //printf("Ready to start the game,\n");
	    int firstplayer=rand()%num_players;
	    Potato potato;
	    potato.hops=hops;
	    potato.remain=hops;
	    potato.currid=0;
	    potato.trace[0]=firstplayer;
	    send(players_socket[firstplayer], &potato, sizeof(potato), 0);
	    cout<<"Ready to start the game, sending potato to player "<<firstplayer<<endl;
	    break;
	  }
	}


      }




    }else{
      //player socket has activity

      //      cout<<"player has activity"<<endl;
      for (int i = 0; i < num_players; i++)
	{
	  sd = players_socket[i];
	  Potato potato_recv;
	  memset(&potato_recv, 0, sizeof(potato_recv));
	  if (FD_ISSET( sd , &readfds))
	    {
	      valread = recv( sd , &potato_recv, sizeof(potato_recv), 0);
	      if(valread<0){
		perror("receive error");
	      }

	      if (valread == 0)
		{
		  //	  cout<<"player "<< i<< " is goind to quit"<<endl;
		  close( sd );
		  players_socket[i] = 0;
		}else{

		//cout<<"player "<<i<<" send me the last potato"<<endl;

		    //	      cout<<"in master side, remain hops is "<<potato_recv.remain<<endl;
		      if(potato_recv.remain==0){
			//	cout<<"received hops is "<<potato_recv.hops<<endl;
			printf("Trace of potato: \n");
			//get the trace
		
			for(int i=0; i<hops; i++){
			  int player=potato_recv.trace[i];
			  printf(" %d ", player);
			}
		
			printf("\n");
			//it is the end of the game
			//send notification to all players
			//	cout<<"ready to let all players out"<<endl;
			for(int i=0; i<hops; i++){
			  //			  cout<<"sent quitting notification to player "<<i<<endl;
			  send(players_socket[i], &potato_recv, sizeof(potato_recv), 0);
			  close(players_socket[i]);
			  players_socket[i] = 0;
			}
			return 0;
		      }
		      break;
		}
	      // }//end of outer while

	    }//end of outer for loop

	}
      }//end of else, activity is not on master_socket but on players' socket

  }




  return 0;
}
