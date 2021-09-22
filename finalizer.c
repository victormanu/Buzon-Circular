#include "dataStructures.h"
#include "sharedMemoryMapping.h"

void initializesFinalizer(char *bufferName);
void newMessages(int index);
char * setMemObjName(char *_name, const char *_tag);


//********** Struct del finalizador *********
struct Finalizer finalizer;

//********* Variables de los semáforos ***********
char *semaphoreProdName;
char *semaphoreConsuName;
char *shmSemphoreProdName;
char *shmSemaphoreConsuName;

//********* Buffers productor y consumidor ***********
char *struProducerName;
char *struConsumerName;

//********* Tamaño buffer mensajes ***********
int bufferSize;

//********* Función Principal ***********
int main(int argc, char *argv[]){
	if(argc != 2) {
		printf("%s\n", "\033[1;31mError: Se debe ingresar 1 argumento, nombre del buffer\033[0m");
		exit(1);
	}
	initializesFinalizer(argv[1]);
	sem_wait(finalizer.shmSemphoreProd); 		// Decrementa el buffer del semáforo de productores
	finalizer.struProducer->bufferState = 0;	// Se cambia el estado del buffer (inactivo)
	sem_post(finalizer.shmSemphoreProd);		// Incrementa el buffer del semáforo del finalizador
	for (int i = 0; i <= finalizer.struProducer->numTotalProducers; ++i){ // Para todos los productores
		sem_post(finalizer.producerSemaphore); // Se va liberando el semaforo por cada productor
	}
	while(finalizer.struConsumer->numTotalConsumers > 0){ // Para todos los consumidores vivos
		sem_wait(finalizer.shmSemphoreProd);
		finalizer.currentBufferPosition = finalizer.struProducer->bufferPosition;
		finalizer.struProducer->bufferPosition++;
		finalizer.struProducer->bufferPosition = finalizer.struProducer->bufferPosition % (bufferSize / sizeof(struct Messages));
		sem_post(finalizer.shmSemphoreProd);
		sem_wait(finalizer.producerSemaphore);
		newMessages(finalizer.currentBufferPosition);
		sem_post(finalizer.consumerSemaphore);
	}

	// Estadísitcas
	printf("\033[1;37m|----------------------------------------------------|\n");
	printf("|               \033[1;33mEstadísticas finales                 \033[1;37m|\n");
	printf("|----------------------------------------------------|\n");
	printf("|\033[0;36mConsumidores Totales               \033[1;37m|    \033[1;36m%-10i  \033[1;37m|\n", finalizer.struConsumer->numConsumers);
	printf("|----------------------------------------------------|\n");
	printf("|\033[0;36mProductores Totales                \033[1;37m|    \033[1;36m%-10i  \033[1;37m|\n", finalizer.struProducer->numProducers);
	printf("|----------------------------------------------------|\n");
	printf("|\033[0;36mConsumidores borrados por llave    \033[1;37m|    \033[1;36m%-10d  \033[1;37m|\n", finalizer.struConsumer->numKeyDeleted);
	printf("|----------------------------------------------------|\n");
	printf("|\033[0;36mTiempo total en espera (s)         \033[1;37m|    \033[1;36m%-10f  \033[1;37m|\n", (finalizer.struConsumer->waitingTime + finalizer.struProducer->waitingTime)*100000);
	printf("|----------------------------------------------------|\n");
	printf("|\033[0;36mTiempo total en bloqueo (s)        \033[1;37m|    \033[1;36m%-10f  \033[1;37m|\n", (finalizer.struConsumer->blockedTime + finalizer.struProducer->blockedTime)*100000);
	printf("|----------------------------------------------------|\n");
	printf("|\033[0;36mTiempo total en usuario (us)       \033[1;37m|    \033[1;36m%-10i  \033[1;37m|\n", finalizer.struConsumer->userTime);
	printf("|----------------------------------------------------|\n");
	printf("|\033[0;36mTimepo total en kernel (us)        \033[1;37m|    \033[1;36m%-10i  \033[1;37m|\n", finalizer.struProducer->kernelTime);
	printf("|----------------------------------------------------|\033[0m\n");
	
	// **************** Liberación de espacio ************* 
	sem_unlink(setMemObjName(argv[1], "SemaphoreProducers"));
	sem_unlink(setMemObjName(argv[1], "SemaphoreCustomers"));
	sem_unlink(setMemObjName(argv[1], "SemaphoreSharedMemConsumer"));
	sem_unlink(setMemObjName(argv[1], "SemaphoreSharedMemProducer"));
	removeSharedMemory(setMemObjName(argv[1], "SharedMemProducer"));
	removeSharedMemory(setMemObjName(argv[1], "SharedMemCustomer"));
	removeSharedMemory(argv[1]);

	return 0;
}

//********* Función para inicializar el finalizador ***********
void initializesFinalizer(char *_bufferName){
	finalizer.buffer = (struct Messages *) mapSharedMemory(_bufferName);

	finalizer.producerSemaphore = getSemaphore(setMemObjName(_bufferName, "SemaphoreProducers"));
	finalizer.consumerSemaphore = getSemaphore(setMemObjName(_bufferName, "SemaphoreCustomers"));
	finalizer.shmSemaphoreConsu = getSemaphore(setMemObjName(_bufferName, "SemaphoreSharedMemConsumer"));
	finalizer.shmSemphoreProd = getSemaphore(setMemObjName(_bufferName, "SemaphoreSharedMemProducer"));
	finalizer.struProducer = (struct Producers *) mapSharedMemory(setMemObjName(_bufferName, "SharedMemProducer"));
	finalizer.struConsumer = (struct Consumers *) mapSharedMemory(setMemObjName(_bufferName, "SharedMemCustomer"));

	bufferSize = getMemorySize(_bufferName);
	finalizer.waitingTime = 0;
	finalizer.blockedTime = 0;
	finalizer.kernelTime = 0;
}

//********* Función para escribir mensajes ***********
void newMessages(int _index){
	time_t rawTime;
 	time(&rawTime);
  	struct tm *time_info = localtime(&rawTime);
	struct Messages newMessage;	
	newMessage.Id = finalizer.Id; newMessage.dateTime.day = time_info->tm_mday;
	newMessage.dateTime.month = time_info->tm_mon + 1; newMessage.dateTime.year = time_info->tm_year + 1900;
	newMessage.dateTime.hour = time_info->tm_hour; newMessage.dateTime.minutes = time_info->tm_min;
	newMessage.dateTime.seconds = time_info->tm_sec; 
	newMessage.magicNumber = -1; // Pone el magicNumber en -1 para finalizar consumidores
	writeSharedMemory(finalizer.buffer, &newMessage, sizeof(struct Messages), _index);
}

// *********** Función para concatenar el nombre del buffer con el tipo de objeto de mem a crear **************
char * setMemObjName(char *_name, const char *_tag){
	char *tagName = (char *) calloc(strlen(_name) + strlen(_tag), sizeof(char));
	strcpy(tagName, _name);
	strcat(tagName, _tag);
	return tagName;
}
