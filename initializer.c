#include "dataStructures.h"
#include "sharedMemoryMapping.h"


char * setMemObjName(char *_name, const char *_tag);

//***************** Función principal **********************
int main(int argc, char *argv[]){
	if(argc != 3){
		printf("%s\n", "\033[1;31mError: Faltan argumentos\033[0m");
		exit(EXIT_FAILURE);
	}
	//***************** Argumentos del inicializador **********************
	char *bufferName = argv[1];
	int bufferSize = atoi(argv[2]);


	//***************** Creación de memoria compartida **********************
	newSharedMemory(bufferName, bufferSize * sizeof(struct Messages));

	//***************** Semáforos **********************
	createSemaphore(setMemObjName(bufferName, "SemaphoreProducers"), bufferSize);		// Productores
	createSemaphore(setMemObjName(bufferName, "SemaphoreCustomers"), 0);				// Consumidores

	//***************** Variables del productor **********************
	struct Producers struProducer;
	struProducer.numTotalProducers = 0;
	struProducer.bufferPosition = 0;
	struProducer.bufferState = 1;				// 1 activo | 0 no activo
	struProducer.numMessages = 0;
    struProducer.numProducers = 0;
	struProducer.waitingTime = 0;
	struProducer.blockedTime = 0;
	struProducer.kernelTime = 0;

	//***************** Memoria datos productor **********************
	newSharedMemory(setMemObjName(bufferName, "SharedMemProducer"), sizeof(struct Producers));

	struct Producers * sharedMemProdPointer = (struct Producers *) 
	mapSharedMemory(setMemObjName(bufferName, "SharedMemProducer"));

	writeSharedMemory(sharedMemProdPointer, &struProducer, sizeof(struct Producers), 0);
	createSemaphore(setMemObjName(bufferName, "SemaphoreSharedMemProducer"), 1);

	//***************** Variables del consumidor **********************
	struct Consumers struConsumer;
	struConsumer.numTotalConsumers = 0;
	struConsumer.bufferPosition = 0;
	struConsumer.numConsumers = 0;
	struConsumer.numKeyDeleted = 0;
	struConsumer.waitingTime = 0;
	struConsumer.blockedTime = 0;
	struConsumer.userTime = 0;

	//***************** Memoria datos consumidor **********************
	newSharedMemory(setMemObjName(bufferName, "SharedMemCustomer"), sizeof(struct Consumers));
	struct Consumers * sharedMemConsumPointer = mapSharedMemory(setMemObjName(bufferName, "SharedMemCustomer"));
	writeSharedMemory(sharedMemConsumPointer, &struConsumer, sizeof(struct Consumers), 0);
	createSemaphore(setMemObjName(bufferName, "SemaphoreSharedMemConsumer"), 1);

	printf("\033[1;33mBuffer \033[0;33m%s \033[1;33mcreado correctamente\033[0m\n", bufferName);
	return 0;
}

// *********** Función para concatenar el nombre del buffer con el tipo de objeto de mem a crear **************
char * setMemObjName(char *_name, const char *_tag){
	char *tagName = (char *) calloc(strlen(_name) + strlen(_tag), sizeof(char));
	strcpy(tagName, _name);
	strcat(tagName, _tag);
	return tagName;
}
