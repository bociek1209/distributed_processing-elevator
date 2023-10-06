#ifndef GLOBALH
#define GLOBALH

#include <mpi.h>
#include <pthread.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "utils.h"
#include "recv_thread.h"
#include "send_thread.h"

// MPI TAGS
typedef enum mpi_tag
{
  REQ_ELEVATOR,
  ACK_ELEVATOR,
  RELEASE_ELEVATOR
} mpi_tag_t;

// Process state
typedef enum
{
  Ground,
  RequestingElevatorUp,
  ShipmentDelivery,
  RideElevatorUp,
  RequestingElevatorDown,
  RideElevatorDown,

} state_type;


// LAMPORT ELEVATOR
typedef struct {
  int procesId;
  int direction;
  int weight;
  int timestamp;
 
} elevator_queue_item_type;

extern state_type state;
extern int procesId;
extern int numberOfProceses;
extern int ts;

extern int elevatorCapacity;

extern int selectedDirection;
extern int setWeight;

extern int elevatorAckCount;
extern int elevatorTs;
extern elevator_queue_item_type *elevatorRequestQueue;
extern int hasAccessElevator;
extern int sendElevatorBroadcast;


typedef struct packet
{
  int ts;
  int elevatorTs;
  int procesId;
  int direction;
  int weight;
} packet_type;


extern MPI_Datatype MPI_PACKET_T;

extern pthread_mutex_t stateMutex;
extern pthread_mutex_t timestampMutex;
extern pthread_mutex_t elevatorTimestampMutex;
extern pthread_mutex_t elevatorACKMutex;
extern pthread_mutex_t elevatorQueueMutex;


void changeState(state_type);
void incrementTimestamp();
void maxTimestamp(int value);

void printElevatorQueue();


  // Elevator access
void initiateElevatorQueue();
int checkIfHasElevatorACK(int otherProcesId, int myProcesId, int countWeight);
void checkElevatorAccess(int myProcesId);
void broadcastElevatorRequest();
void releaseElevator();
void clearElevatorQueue();



#ifdef DEBUG
#define debug(FORMAT, ...)                                                   \
  printf("%c[%d;%dm [%d]: " FORMAT "%c[%d;%dm\n", 27, (1 + (direction / 7)) % 2,    \
         31 + (6 + direction) % 7, procesId, ##__VA_ARGS__, 27, 0, 37);
#else
#define debug(...) ;
#endif

#define P_WHITE printf("%c[%d;%dm", 27, 1, 37);
#define P_BLACK printf("%c[%d;%dm", 27, 1, 30);
#define P_RED printf("%c[%d;%dm", 27, 1, 31);
#define P_GREEN printf("%c[%d;%dm", 27, 1, 33);
#define P_BLUE printf("%c[%d;%dm", 27, 1, 34);
#define P_MAGENTA printf("%c[%d;%dm", 27, 1, 35);
#define P_CYAN printf("%c[%d;%d;%dm", 27, 1, 36);
#define P_SET(X) printf("%c[%d;%dm", 27, 1, 31 + (6 + X) % 7);
#define P_CLR printf("%c[%d;%dm", 27, 0, 37);

/* printf with colors and aligned columns */
#define println(FORMAT, ...)                                               \
  do {                                                                    \
    printf("%c[%d;%dm [%d]: ", 27, 1, 33 + (procesId % 8), ts);         \
    printf(FORMAT, ##__VA_ARGS__);                                        \
    printf("%c[%d;%dm\n", 27, 0, 37);                                    \
  } while (0);

#endif
