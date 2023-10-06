#include "send_thread.h"


void startSendThread()
{
  while(1) {
    switch(state) {
      case Ground: {

        sleep(rand() % 6);
        selectedDirection = 1;
        println("Elevator capacity:%d", elevatorCapacity);

        setWeight = rand() % elevatorCapacity + 1;

        pthread_mutex_lock(&stateMutex);
        state = RequestingElevatorUp;
        pthread_mutex_unlock(&stateMutex);
        break;
      }
      case RequestingElevatorUp: {

        pthread_mutex_lock(&elevatorQueueMutex);
        pthread_mutex_lock(&timestampMutex);
        elevatorRequestQueue[procesId].timestamp = ts;
        pthread_mutex_unlock(&timestampMutex);
        elevatorRequestQueue[procesId].weight = setWeight;
        elevatorRequestQueue[procesId].direction = selectedDirection;
        pthread_mutex_unlock(&elevatorQueueMutex);
        println("Courier:%d  wants to go with a package weighing:%d by elevator to the floor.", procesId, setWeight);
        broadcastElevatorRequest();
        // printElevatorQueue();

        while(!hasAccessElevator) {
        }
        pthread_mutex_lock(&stateMutex);
        state = RideElevatorUp;
        pthread_mutex_unlock(&stateMutex);
        break;
      }
      case RideElevatorUp: {

        println("Courier:%d  rides with package weighing:%d by elevator to the floor.", procesId, setWeight);
        sleep(rand() % 6);
        
        pthread_mutex_lock(&elevatorQueueMutex);
        elevatorRequestQueue[procesId].timestamp = -1;
        elevatorRequestQueue[procesId].direction = -1;
        elevatorRequestQueue[procesId].weight = 0;
        pthread_mutex_unlock(&elevatorQueueMutex);
        elevatorAckCount = 0;
        hasAccessElevator = 0;
        sendElevatorBroadcast = 0;
        releaseElevator();
        println("Courier:%d  leaves the elevator on the floor.", procesId);

        pthread_mutex_lock(&stateMutex);
        state = ShipmentDelivery;
        pthread_mutex_unlock(&stateMutex);
        break;
      }

      case ShipmentDelivery: {

        sleep(rand() % 6);
        println("Courier:%d  delivers the package.", procesId);

        selectedDirection = 0;
        setWeight = 0;
        pthread_mutex_lock(&stateMutex);
        state = RequestingElevatorDown;
        pthread_mutex_unlock(&stateMutex);
        break;
      }
      case RequestingElevatorDown: {

        pthread_mutex_lock(&elevatorQueueMutex);
        pthread_mutex_lock(&timestampMutex);
        elevatorRequestQueue[procesId].timestamp = ts;
        pthread_mutex_unlock(&timestampMutex);
        elevatorRequestQueue[procesId].direction = selectedDirection;
        elevatorRequestQueue[procesId].weight = setWeight;
        pthread_mutex_unlock(&elevatorQueueMutex);
        broadcastElevatorRequest();
        // printElevatorQueue();
        println("Courier:%d  wants to take elevator to the ground.", procesId);
        while(!hasAccessElevator) {
        }
        pthread_mutex_lock(&stateMutex);
        state = RideElevatorDown;
        pthread_mutex_unlock(&stateMutex);
        break;
      }
      case RideElevatorDown: {

        println("Courier:%d  rides by the elevator to the ground.", procesId);
        sleep(rand() % 6);

        pthread_mutex_lock(&elevatorQueueMutex);
        elevatorRequestQueue[procesId].timestamp = -1;
        elevatorRequestQueue[procesId].direction = -1;
        elevatorRequestQueue[procesId].weight = 0;

        pthread_mutex_unlock(&elevatorQueueMutex);
        elevatorAckCount = 0;
        hasAccessElevator = 0;
        sendElevatorBroadcast = 0;
        releaseElevator();
        println("Courier:%d  leaves the elevator on the ground.", procesId);

        pthread_mutex_lock(&stateMutex);
        state = Ground;
        pthread_mutex_unlock(&stateMutex);

        break;
      }
      
      default: break;
    }
  }
}