#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <vector>

using namespace std;

typedef struct _potato{
  int hops;
  int remain;
  int currid;
  int trace[128];
  
}Potato;
