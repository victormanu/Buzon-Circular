#include "dataStructures.h"
#include "sharedMemoryMapping.h"

#define ENTER_ASCII_CODE 10

// Declaracion de las funciones
void initProducer(char *_bufferName, double _randomMeanTime, int _mode);
void killProducer();
void writeMessage(int index);
float exponential(double _lambda);
char * setMemObjName(char *_name, const char *_tag);


// Struct con la informacion del productor
struct Producer producer;
// Variable para establecer el modo de ejecucion (0 = Automatico, 1 = Manual)
int mode = 0;
// Tamaño del buffer
int bufferSize;
// Struct para obtener el tiempo de kernel
struct rusage ktime;
// Variable utilizada para terminar el productor
int kill = 0;

// Variables para almacenar los tiempos de bloqueo y espera
clock_t waitedTimeInit, waitedTimeEnd;
clock_t blockedTimeInit, blockedTimeEnd;

int main(int argc, char *argv[]){
	if (argc != 4)	{
		printf("%s\n", "\033[1;31mError: Se deben escribir 3 parámetros, nombre del buffer, media para tiempos y modo de operación\033[0m");
		exit(1);
	}
	// Invoca al inicializador
	initProducer(argv[1], atoi(argv[2]), atoi(argv[3]));

	// Ciclo para verificar si se finaliza
	while (!kill){
		if (mode){
			// Espera a input del teclado
			printf("\033[0;36mPresione \033[1;33mEnter\033[0;36m para generar un mensaje...\033[0m\n");
			
			// Guarda el tiempo de espera inicial
			waitedTimeInit = clock();

			while (getchar() != ENTER_ASCII_CODE);

			// Guarda el tiempo espera final
			waitedTimeEnd = clock();

			// Guarda el tiempo esperado
			producer.inWait += (double)(waitedTimeEnd - waitedTimeInit) / CLOCKS_PER_SEC;
		}
		else{
			// Guarda el tiempo de espera inicial
			waitedTimeInit = clock();
			
			// Espera  un tiempo generado con distribucion exponencial
			sleep(exponential(producer.meanTimes));
			
			// Guarda el tiempo espera final
			waitedTimeEnd = clock();

			// Guarda el tiempo esperado
			producer.inWait += (double)(waitedTimeEnd - waitedTimeInit) / CLOCKS_PER_SEC;
		}
		
		// Guarda el tiempo de bloqueo inicial
		blockedTimeInit = clock();

		// Disminuye el semaforo de productores
		sem_wait(producer.shmSemphoreProd); // Duermo semaforo para determinar direccion en el buffer

		// Almacena la posicion del buffer de productores para evitar conflictos
		producer.bufferPosition = producer.struProducer->bufferPosition;

		// Aumenta la posicion en el buffer
		producer.struProducer->bufferPosition++;
		producer.struProducer->bufferPosition = producer.struProducer->bufferPosition % 
		(bufferSize / sizeof(struct Messages));
		
		// Aumenta el semaforo de productores
		sem_post(producer.shmSemphoreProd);

		// Disminuye el buffer de productores
		sem_wait(producer.semProducersBuffer); // Duermo semaforo para acceso a buffer compartido

		// Guarda el tiempo de bloqueo final
		blockedTimeEnd = clock();

		// Guarda el tiempo bloqueado
		producer.inBlock += (double)(blockedTimeEnd - blockedTimeInit) / CLOCKS_PER_SEC;
		
		// Esbricir un nuevo mensaje al buffer compartido
		writeMessage(producer.bufferPosition);

		// Aumenta el numero de mensajes producidos
		producer.producedMsgs++;

		
		sem_post(producer.semConsumersBuffer);

		// Indica que se generó un mensaje
		printf("\033[1;37m|--------------------------------------------|\n");
		printf("|  \033[0;33mUn nuevo mensaje fue escrito en el buzón  \033[1;37m|\n");
		printf("|--------------------------------------------|\n");
		printf("|\033[0;36mIndice Buffer        \033[1;37m|    \033[1;36m%-10i        \033[1;37m|\n", producer.bufferPosition);
		printf("|--------------------------------------------|\n");
		printf("|\033[0;36mConsumidores vivos   \033[1;37m|    \033[1;36m%-10i        \033[1;37m|\n", producer.struConsumer->numTotalConsumers);
		printf("|--------------------------------------------|\n");
		printf("|\033[0;36mProductores vivos    \033[1;37m|    \033[1;36m%-10i        \033[1;37m|\n", producer.struProducer->numTotalProducers);
		printf("|--------------------------------------------|\033[0m\n\n");

		// Verifica si debe escribir un nuevo mensaje al buffer (revisa si debe finalizar)
		if ((producer.struProducer->bufferState) == 0){
			// Finaliza el productor
			killProducer();
		}
	}

	// Mensaje de finalización de un consumidor
	printf("\033[1;34m|-------------------------------------------------|\n");
	printf("| \033[1;33mEl productor con el ID %-5i ha sido finalizado \033[1;34m|\n", producer.Id);
	printf("|-------------------------------------------------|\n");
	printf("|\033[0;36mMensajes generados   \033[1;34m|    \033[1;36m%-10d             \033[1;34m|\n", producer.producedMsgs);
	printf("|-------------------------------------------------|\n");
	printf("|\033[0;36mT en espera (s)      \033[1;34m|    \033[1;36m%-10f             \033[1;34m|\n", (producer.inWait)*100000);
	printf("|-------------------------------------------------|\n");
	printf("|\033[0;36mT bloqueada (s)      \033[1;34m|    \033[1;36m%-10f             \033[1;34m|\n", (producer.inBlock)*100000);
	printf("|-------------------------------------------------|\n");
	printf("|\033[0;36mT Kernel (us)        \033[1;34m|    \033[1;36m%-10li             \033[1;34m|\n", producer.kernelTime);
	printf("|-------------------------------------------------|\033[0m\n\n");

	return 0;
}

// Inicializador del productor
void initProducer(char *_bufferName, double _randomMeanTime, int _mode){
	if (_mode != 0 && _mode != 1){
		printf("\033[1;31m");
		printf("%s\n", "Error: El modo de operación debe ser 0 (automático) o 1 (manual)");
		printf("\033[0m");
		exit(1);
	}

	// Verifica el modo de operacion
	if (_mode == 1){
		mode = 1;
	}
	// Establece valores del struct de productores
	producer.Id = getpid();
	producer.meanTimes = _randomMeanTime;
	producer.buffer = (struct Messages *)mapSharedMemory(_bufferName);

	// Semaforo de acceso al buffer de productores	
	producer.semProducersBuffer = getSemaphore(setMemObjName(_bufferName, "SemaphoreProducers"));
	// Semaforo de acceso al buffer de consumidores
	producer.semConsumersBuffer = getSemaphore(setMemObjName(_bufferName, "SemaphoreCustomers"));
	// Mapeo de las variables globales compartidas del productor
	producer.struProducer = (struct Producers *)mapSharedMemory(setMemObjName(_bufferName, "SharedMemProducer"));
	//Mapeo de las variables globales compartidas del consumidor
	producer.struConsumer = (struct Consumers *)mapSharedMemory(setMemObjName(_bufferName, "SharedMemCustomer"));
	// Semaforo de compartición de los productores
	producer.shmSemphoreProd = getSemaphore(setMemObjName(_bufferName, "SemaphoreSharedMemProducer"));

	// En espera para el semaforo de productores
	sem_wait(producer.shmSemphoreProd);

	// Aumenta la cantidad de productores totales
	producer.struProducer->numTotalProducers++;
	producer.struProducer->numProducers++; // Activos

	// Incrementa el valor del semaforo de productores
	sem_post(producer.shmSemphoreProd);

	// Se obtiene el tamaño del buffer de mensajes para hacer calculos de posiciones
	bufferSize = getMemorySize(_bufferName);

	// Establece valores del struct de productores
	producer.producedMsgs = 0;
	producer.inWait = 0;
	producer.inBlock = 0;
	producer.kernelTime = 0;
}

// Finaliza el productor
void killProducer(){
	// Obtiene el struct del proceso
	getrusage(RUSAGE_SELF, &ktime);

	// Establece el tiempo de kernel
	producer.kernelTime = (long int)ktime.ru_stime.tv_usec;

	// Disminuye el semaforo de productores
	sem_wait(producer.shmSemphoreProd);

	// Establece los valores del productor
	producer.struProducer->numTotalProducers--;	
	producer.struProducer->numMessages += producer.producedMsgs;
	producer.struProducer->waitingTime += producer.inWait;
	producer.struProducer->blockedTime += producer.inBlock;
	producer.struProducer->kernelTime += producer.kernelTime;

	// Aumenta el semaforo de productores
	sem_post(producer.shmSemphoreProd);

	// Establece la variable de finalizacion
	kill = 1;
}

// Realiza la escritura de mensajes
void writeMessage(int index){
	time_t rawTime;
	time(&rawTime);
	struct tm *time_info = localtime(&rawTime);
	srand(time(NULL));

	struct Messages newMessage;
	newMessage.Id = producer.Id; newMessage.dateTime.day = time_info->tm_mday;
	newMessage.dateTime.month = time_info->tm_mon + 1; newMessage.dateTime.year = time_info->tm_year + 1900;
	newMessage.dateTime.hour = time_info->tm_hour; newMessage.dateTime.minutes = time_info->tm_min;
	newMessage.dateTime.seconds = time_info->tm_sec; newMessage.magicNumber = rand() % (6 + 1);
	writeSharedMemory(producer.buffer, &newMessage, sizeof(struct Messages), index);
}

// Distribucion exponencial = 1-e^(-Lambda*r)
float exponential(double _lambda){
	float r = rand() / (RAND_MAX + 1.0);
	float num = 1-exp(-_lambda*r);
	printf("Se realiza una espera de %f s\n", num);
	return num;
}

// *********** Función para concatenar el nombre del buffer con el tipo de objeto de mem a crear **************
char * setMemObjName(char *_name, const char *_tag){
	char *tagName = (char *) calloc(strlen(_name) + strlen(_tag), sizeof(char));
	strcpy(tagName, _name);
	strcat(tagName, _tag);
	return tagName;
}
