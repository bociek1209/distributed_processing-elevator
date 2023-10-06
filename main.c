#include "main.h"
#include "utils.h"

pthread_mutex_t stateMutex          = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t timestampMutex      = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t elevatorACKMutex       = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t elevatorTimestampMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t elevatorQueueMutex     = PTHREAD_MUTEX_INITIALIZER;


int numberOfProceses, procesId, ts, elevatorTs, elevatorAckCount, selectedDirection, setWeight, elevatorCapacity, hasAccessElevator, sendElevatorBroadcast;

state_type state;
elevator_queue_item_type *elevatorRequestQueue;
pthread_t recvThread;
MPI_Datatype MPI_PACKET_T;


void check_thread_support(int provided)
{
  if (provided != MPI_THREAD_MULTIPLE)
  {
    fprintf(stderr, "Inadequate support for threads!");
    MPI_Finalize();
    exit(-1);
  }
}


void init(int *argc, char ***argv) {

  if (*argc == 2) {
    elevatorCapacity =  atoi((*argv)[1]);
  }

  ts = 0;
  elevatorTs = 0;
  hasAccessElevator = 0;
  sendElevatorBroadcast = 0;

  int provided;
  MPI_Init_thread(argc, argv, MPI_THREAD_MULTIPLE, &provided);
  check_thread_support(provided);

  int block_lengths[5] = {1, 1, 1, 1, 1};
  MPI_Datatype types[5] = {MPI_INT,MPI_INT,MPI_INT, MPI_INT, MPI_INT};

  MPI_Aint offsets[5];
  offsets[0] = offsetof(packet_type, ts);
  offsets[1] = offsetof(packet_type, elevatorTs);
  offsets[2] = offsetof(packet_type, procesId);
  offsets[3] = offsetof(packet_type, direction);
  offsets[4] = offsetof(packet_type, weight);

  MPI_Type_create_struct(4, block_lengths, offsets, types,&MPI_PACKET_T);
  MPI_Type_commit(&MPI_PACKET_T);
  MPI_Comm_rank(MPI_COMM_WORLD, &procesId);
  MPI_Comm_size(MPI_COMM_WORLD, &numberOfProceses);

  state = Ground;

  initiateElevatorQueue();
  srand(procesId);

  pthread_create(&recvThread, NULL, startRecvThread, 0);
}


void finalize() {
  clearElevatorQueue();

  pthread_mutex_destroy(&stateMutex);
  pthread_mutex_destroy(&timestampMutex);
  pthread_mutex_destroy(&elevatorTimestampMutex);
  pthread_mutex_destroy(&elevatorACKMutex);
  pthread_mutex_destroy(&elevatorQueueMutex);
  pthread_join(recvThread, NULL);
  MPI_Type_free(&MPI_PACKET_T);
  MPI_Finalize();
}


void incrementTimestamp()
{
  pthread_mutex_lock(&timestampMutex);
  ts += 1;
  pthread_mutex_unlock(&timestampMutex);
}


void maxTimestamp(int value)
{
  pthread_mutex_lock(&timestampMutex);
  ts = max(value, ts) + 1;
  pthread_mutex_unlock(&timestampMutex);
}


void changeState(state_type newState) {
  pthread_mutex_lock(&stateMutex);
  state = newState;
  pthread_mutex_unlock(&stateMutex);
}


// Define display colors for testing printf
#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"

const char* processColors[] = {
    RED,
    GREEN,
    YELLOW,
    BLUE
};


int getNumProcessColors() {
    return sizeof(processColors) / sizeof(processColors[0]);
}


const char* getColorForProcessId(int processId) {
    int numColors = getNumProcessColors();
    return processColors[processId % numColors];
}


void printElevatorQueue(){

  for(int i = 0; i < numberOfProceses; i++){
        const char* textColor = getColorForProcessId(elevatorRequestQueue[i].procesId);
        printf("%s", textColor);

        printf("PROCES ID: %d\n", elevatorRequestQueue[i].procesId);
        printf("DIRECTION: %d\n", elevatorRequestQueue[i].direction);
        printf("WEIGHT: %d\n", elevatorRequestQueue[i].weight);
        printf("TIMESTAMP: %d\n", elevatorRequestQueue[i].timestamp);

        printf(RESET);
  }
}


void initiateElevatorQueue() {
  elevatorRequestQueue = malloc(sizeof(elevator_queue_item_type) * numberOfProceses);
  for (int i = 0; i < numberOfProceses; i++) {

    elevator_queue_item_type request_queue_item;
    request_queue_item.timestamp = -1;
    request_queue_item.procesId = i;
    request_queue_item.direction = -1;
    request_queue_item.weight = 0;

    elevatorRequestQueue[i] = request_queue_item;
  }
}

                          
int checkIfHasElevatorACK(int otherProcesId, int myProcesId, int countWeight) {
  if(otherProcesId == myProcesId){
    return 1;
  }
  else if(elevatorRequestQueue[myProcesId].direction == elevatorRequestQueue[otherProcesId].direction){
   
    if(elevatorRequestQueue[myProcesId].timestamp > elevatorRequestQueue[otherProcesId].timestamp  > 0){
      if(elevatorRequestQueue[myProcesId].direction == 1 && (countWeight + elevatorRequestQueue[otherProcesId].timestamp) <= elevatorCapacity){
          return 1;
      }

      return 0;
    }
    else if((elevatorRequestQueue[myProcesId].timestamp == elevatorRequestQueue[otherProcesId].timestamp) && (myProcesId > otherProcesId) ){
      return 0;
    }
    else{
      return 1;
    }
  } 
  else{
    return 1;
  }
}


void checkElevatorAccess(int myProcesId) {
  int countElevatorPermission = 0;
  int countWeight = 0;

  for(int i = 0; i < numberOfProceses; i++){
    if (checkIfHasElevatorACK(i, procesId, countWeight)){
      countElevatorPermission += 1;
    }
  }

  if (countElevatorPermission >= (numberOfProceses)){
    hasAccessElevator = 1;
  } else{
    hasAccessElevator = 0;
  }
}


void broadcastElevatorRequest()
{
  incrementTimestamp();
  elevatorAckCount = 1;

  packet_type *pkt = malloc(sizeof(packet_type));
  pthread_mutex_lock(&timestampMutex);
  pkt->ts = ts;
  pkt->elevatorTs = ts;
  pthread_mutex_unlock(&timestampMutex);
  pkt->procesId = procesId;
  pkt->direction = selectedDirection;
  pkt->weight = setWeight;

  for(int i = 0; i < numberOfProceses; i++) {
    if(procesId != i) {
      MPI_Send(pkt, 1, MPI_PACKET_T, i, REQ_ELEVATOR, MPI_COMM_WORLD);
    }
  }
  sendElevatorBroadcast = 1;
  free(pkt);
}


void releaseElevator()
{
  incrementTimestamp();
  packet_type *pkt = malloc(sizeof(packet_type));
  pthread_mutex_lock(&timestampMutex);
  pkt->ts = ts;   
  pthread_mutex_unlock(&timestampMutex);
  pkt->elevatorTs = 0; 
  pkt->procesId = procesId;
  pkt->direction = -1;
  pkt->weight = -1;

  for(int i = 0; i < numberOfProceses; i++) {
    if(procesId != i) {
      MPI_Send(pkt, 1, MPI_PACKET_T, i, RELEASE_ELEVATOR, MPI_COMM_WORLD);
    }
  }
  free(pkt);
}


void clearElevatorQueue(){
  for (int i = 0; i < numberOfProceses; i++) {
    elevatorRequestQueue[i].timestamp = -1;
    elevatorRequestQueue[i].direction = -1;

  }
}


int main(int argc, char **argv) {
  init(&argc, &argv);
  startSendThread();
  finalize();
  return 0;
}