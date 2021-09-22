#ifndef _DATASTRUCTURES_H
#define _DATASTRUCTURES_H

#include <semaphore.h> // Define el tipo sem_t para manejo de semaforos


// ********************** Structs generales ****************************

// *********** Productor **********
struct Producers {
	int numTotalProducers;
	int bufferPosition;
	int bufferState;
	int numMessages;
	int numProducers;
	double waitingTime;
	double blockedTime;
	int kernelTime;
};

// *********** Consumidor **********
struct Consumers {
	int numTotalConsumers;
	int bufferPosition;
	int numConsumers;
	int numKeyDeleted;
	double waitingTime;
	double blockedTime;
	int userTime;
};

// *********** Fecha **********
struct DateTime {
	int day;
	int month;
	int year;
	int hour;
	int minutes;
	int seconds;
};

// *********** Mensajes **********
struct Messages {
	pid_t Id;
	struct DateTime dateTime;
	int magicNumber;
};

// ********************** Structs para cada tipo de proceso ****************************
// *********** Consumidor **********
struct Consumer {
	pid_t Id;
	struct Messages *msg;
	struct Consumers *consumers;
	struct Producers *producers;
	int mean;
	int operationMode;
	int consumedMsgs;
	int bufferPosition;
	char *terminationCause;
	double inWait;
	double inBlock;
	long int inUser;
	sem_t *semConsumers;
	sem_t *semProducersBuffer;
	sem_t *semConsumersBuffer;
};

// *********** Productor **********
struct Producer
{
	pid_t Id;
	struct Messages *buffer;
	struct Producers *struProducer;
	struct Consumers *struConsumer;
	int producedMsgs;
	int bufferPosition;
	double meanTimes;
	double inWait;
	double inBlock;
	long int kernelTime;
	sem_t *semProducersBuffer;
	sem_t *semConsumersBuffer;
	sem_t *shmSemphoreProd;
};


// *********** Finalizador **********
struct Finalizer {
	pid_t Id;
    struct Messages *buffer;
	struct Producers *struProducer;
	struct Consumers *struConsumer;
	int    numMessagess;
	int    currentBufferPosition;
	double waitingTime;
	double blockedTime;
	long int kernelTime;
	sem_t  *producerSemaphore;
	sem_t  *consumerSemaphore;
	sem_t  *shmSemphoreProd;
	sem_t  *shmSemaphoreConsu; 
};


#endif