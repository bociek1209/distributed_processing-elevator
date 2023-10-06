#include "recv_thread.h"

void *startRecvThread(void *ptr)
{
  MPI_Status event;
  packet_type packet;

  while (1) {
    MPI_Recv(&packet, 1, MPI_PACKET_T, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &event);

    maxTimestamp(packet.ts);
    pthread_mutex_lock(&stateMutex);
    state_type currentState = state;
    pthread_mutex_unlock(&stateMutex);

    switch(event.MPI_TAG) {
      case REQ_ELEVATOR: {
       
        pthread_mutex_lock(&elevatorQueueMutex);
        elevatorRequestQueue[packet.procesId].timestamp = packet.elevatorTs;
        elevatorRequestQueue[packet.procesId].direction = packet.direction;
        elevatorRequestQueue[packet.procesId].weight = packet.weight;
        pthread_mutex_unlock(&elevatorQueueMutex);

        packet_type *pkt = malloc(sizeof(packet_type));
        pthread_mutex_lock(&timestampMutex);
        pkt->ts = ts;  
        pthread_mutex_unlock(&timestampMutex);
        pkt->elevatorTs = elevatorRequestQueue[procesId].timestamp;
        pkt->procesId = procesId;
        pkt->direction = selectedDirection;   
        pkt->weight = setWeight;        

        MPI_Send(pkt, 1, MPI_PACKET_T, packet.procesId, ACK_ELEVATOR, MPI_COMM_WORLD);
        
        free(pkt);
        break;
      }
      
      case ACK_ELEVATOR: {

          elevatorAckCount += 1;
          pthread_mutex_lock(&elevatorQueueMutex);
          elevatorRequestQueue[packet.procesId].timestamp = packet.elevatorTs;
          elevatorRequestQueue[packet.procesId].direction = packet.direction;
          elevatorRequestQueue[packet.procesId].weight = packet.weight;
          pthread_mutex_unlock(&elevatorQueueMutex);

          if(elevatorAckCount >= numberOfProceses){
            checkElevatorAccess(procesId);
          }
        
        break;
      }
      case RELEASE_ELEVATOR: {
        
        pthread_mutex_lock(&elevatorQueueMutex);
        elevatorRequestQueue[packet.procesId].timestamp = -1;
        elevatorRequestQueue[packet.procesId].direction = -1;
        elevatorRequestQueue[packet.procesId].weight = 0;
        pthread_mutex_unlock(&elevatorQueueMutex);

        if (!hasAccessElevator && sendElevatorBroadcast == 1 && (currentState == RequestingElevatorUp || currentState == RequestingElevatorDown) ){
          ++ elevatorAckCount;
    
          if(elevatorAckCount >= numberOfProceses){
            checkElevatorAccess(procesId);
          }
        }

        break;
      }
      default: {
        break;
      }
    }
  }
}