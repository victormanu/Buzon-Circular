#include "dataStructures.h"
#include "sharedMemoryMapping.h"

#define ENTER_ASCII_CODE 10


// Declaracion de las funciones
void initConsumer(char *_name, int _mean, int _mode);
void readMessage(int _position);
void killConsumer();
unsigned long factorial(unsigned long _num);
double poisson(double _lambda);
char * setMemObjName(char *_name, const char *_tag);

// Struct con la informacion del consumidor
struct Consumer consumer;

// Variable para establecer el modo de ejecucion (0 = Automatico, 1 = Manual)
int mode = 0;
// Tamaño del buffer
int bufferSize;

// Struct para obtener el tiempo de usuario
struct rusage userTime;
// Variable utilizada para terminar el consumidor
int kill = 0;

// Variables para almacenar los tiempos de bloqueo y espera
clock_t waitBegin, waitEnd;
clock_t blockBegin, blockEnd;


int main(int argc, char *argv[]){
	if (argc != 4){
		printf("%s\n", "\033[1;31mError: Se deben escribir 3 parámetros, nombre del buffer, media para tiempos y modo de operación\033[0m");
		exit(1);
	}

	// Invoca al inicializador
	initConsumer(argv[1], atoi(argv[2]), atoi(argv[3]));

	// Ciclo para verificar si se finaliza
	while (!kill){
		// Verifica el modo de ejecución
		if (mode){
			// Espera a input del teclado
			printf("\033[0;36mPresione \033[1;33mEnter\033[0;36m para obtener un mensaje...\033[0m\n");

			// Guarda el tiempo de espera inicial
			waitBegin = clock();

			while (getchar() != ENTER_ASCII_CODE);

			// Guarda el tiempo espera final
			waitEnd = clock();

			// Guarda el tiempo esperado
			consumer.inWait += (double)(waitEnd - waitBegin) / CLOCKS_PER_SEC;
		}
		else{
			// Guarda el tiempo de espera inicial
			waitBegin = clock();

			// Espera  un tiempo generado con Poisson
			sleep(poisson(consumer.mean));

			// Guarda el tiempo espera final
			waitEnd = clock();

			// Guarda el tiempo esperado
			consumer.inWait += (double)(waitEnd - waitBegin) / CLOCKS_PER_SEC;
		}

		// Guarda el tiempo de bloqueo inicial
		blockBegin = clock();

		// Disminuye el semaforo de consumidores
		sem_wait(consumer.semConsumers);

		// Almacena la posicion del buffer de consumidores para evitar conflictos
		consumer.bufferPosition = consumer.consumers->bufferPosition;

		// Aumenta la posicion en el buffer
		consumer.consumers->bufferPosition++;
		consumer.consumers->bufferPosition = consumer.consumers->bufferPosition % (bufferSize / sizeof(struct Messages));

		// Aumenta el semaforo de consumidores
		sem_post(consumer.semConsumers);

		// Disminuye el buffer de consumidores
		sem_wait(consumer.semConsumersBuffer);

		// Guarda el tiempo de bloqueo final
		blockEnd = clock();

		// Guarda el tiempo bloqueado
		consumer.inBlock += (double)(blockEnd - blockBegin) / CLOCKS_PER_SEC;

		// Leer un nuevo mensaje
		readMessage(consumer.bufferPosition);
	}

	// Mensaje de finalización de un consumidor
	printf("\033[1;34m|-------------------------------------------------|\n");
	printf("|\033[1;33mEl consumidor con el ID %-5i ha sido finalizado \033[1;34m|\n", consumer.Id);
	printf("|-------------------------------------------------|\n");
	printf("|\033[0;36mIndice buffer        \033[1;34m|         \033[1;36m%-10i        \033[1;34m|\n", consumer.bufferPosition);
	printf("|-------------------------------------------------|\n");
	printf("|\033[0;36mMsjs consumidos      \033[1;34m|         \033[1;36m%-10d        \033[1;34m|\n", consumer.consumedMsgs);
	printf("|-------------------------------------------------|\n");
	printf("|\033[0;36mT en espera (s)      \033[1;34m|         \033[1;36m%-10f        \033[1;34m|\n", (consumer.inWait)*100000);
	printf("|-------------------------------------------------|\n");
	printf("|\033[0;36mT bloqueado (s)      \033[1;34m|         \033[1;36m%-10f        \033[1;34m|\n", (consumer.inBlock)*100000);
	printf("|-------------------------------------------------|\n");
	printf("|\033[0;36mT usuario (us)       \033[1;34m|         \033[1;36m%-10li        \033[1;34m|\n", consumer.inUser);
	printf("|-------------------------------------------------|\033[0m\n\n");
	// Razón de finalización
	printf("\033[1;31mCausa de finalización: %s\033[0m\n", consumer.terminationCause);
	return 0;
}

// Inicializador del consumidor
void initConsumer(char *_bName, int _mean, int _mode){
	if (_mode != 0 && _mode != 1)	{
		printf("\033[1;31m");
		printf("%s\n", "Error: El modo de operación debe ser 0 (automático) o 1 (manual)");
		printf("\033[0m");
		exit(1);
	}

	// Verifica el modo de operacion
	if (_mode == 1)	{
		mode = 1;
	}

	// Establece valores del struct de consumidores
	consumer.Id = getpid();
	consumer.mean = _mean;
	consumer.operationMode = _mode;

	// Mapeo de la memoria compartida de mensajes
	consumer.msg = (struct Messages *)mapSharedMemory(_bName);

	// Semaforo de acceso al buffer de productores
	consumer.semProducersBuffer = getSemaphore(setMemObjName(_bName, "SemaphoreProducers"));
	// Semaforo de acceso al buffer de consumidores
	consumer.semConsumersBuffer = getSemaphore(setMemObjName(_bName, "SemaphoreCustomers"));
	//Mapeo de las variables globales compartidas del consumidor
	consumer.consumers = (struct Consumers *)mapSharedMemory(setMemObjName(_bName, "SharedMemCustomer"));
	// Mapeo de las variables globales compartidas del productor
	consumer.producers = (struct Producers *)mapSharedMemory(setMemObjName(_bName, "SharedMemProducer"));
	// Semaforo de compartición de los consumidores
	consumer.semConsumers = getSemaphore(setMemObjName(_bName, "SemaphoreSharedMemConsumer"));

	// En espera para el semaforo de consumidores
	sem_wait(consumer.semConsumers);
	// Aumenta la cantidad de consumidores totales
	consumer.consumers->numTotalConsumers++;
	consumer.consumers->numConsumers++;
	// Incrementa el valor del semaforo de consumidores
	sem_post(consumer.semConsumers);

	// Se obtiene el tamaño del buffer de mensajes para hacer calculos de posiciones
	bufferSize = getMemorySize(_bName);

	// Establece valores del struct de consumidores
	consumer.consumedMsgs = 0;
	consumer.inWait = 0;
	consumer.inBlock = 0;

	//Se obtiene estruct de estadisticas
	getrusage(RUSAGE_SELF, &userTime);
}

// Realiza la lectura e imprime mensajes
void readMessage(int _position){

	struct Messages *msg = consumer.msg + _position;

	if (msg->magicNumber == -1)	{
		killConsumer();
		consumer.terminationCause = "Finalización por el proceso finalizer";
	}
	else if (msg->magicNumber == consumer.Id % 6)	{
		killConsumer();

		// Disminuye el semáforo de consumidores
		sem_wait(consumer.semConsumers);
		consumer.consumers->numKeyDeleted++;

		// Incrementa el semáforo de consumidores
		sem_post(consumer.semConsumers);
		consumer.terminationCause = "La llave leida es igual a id % 6.";
	}

	// Aumenta el número de mensajes consumidos
	consumer.consumedMsgs++;

	// Aumenta el semáforo de productores
	sem_post(consumer.semProducersBuffer);

	printf("\033[1;37m|-------------------------------------------------|\n");
	printf("|    \033[0;33mSe leyó un mensaje de memoria compartida     \033[1;37m|\n");
	printf("|-------------------------------------------------|\n");
	printf("|\033[0;36mIndice buffer        \033[1;37m|         \033[1;36m%-10i        \033[1;37m|\n", consumer.bufferPosition);
	printf("|-------------------------------------------------|\n");
	printf("|\033[0;36mConsumidores vivos   \033[1;37m|         \033[1;36m%-10i        \033[1;37m|\n", consumer.consumers->numTotalConsumers);
	printf("|-------------------------------------------------|\n");
	printf("|\033[0;36mProductores vivos    \033[1;37m|         \033[1;36m%-10i        \033[1;37m|\n", consumer.producers->numTotalProducers);
	printf("|-------------------------------------------------|\n");
	printf("|\033[0;36mPID de Productor     \033[1;37m|         \033[1;36m%-10i        \033[1;37m|\n", msg->Id);
	printf("|-------------------------------------------------|\n");
	printf("|\033[0;36mFecha                \033[1;37m|         \033[1;36m%-2i-%-2i-%-8i    \033[1;37m|\n", msg->dateTime.day, msg->dateTime.month, msg->dateTime.year);
	printf("|-------------------------------------------------|\n");
	printf("|\033[0;36mTiempo               \033[1;37m|         \033[1;36m%-2i:%-2i:%-2i          \033[1;37m|\n", msg->dateTime.hour, msg->dateTime.minutes, msg->dateTime.seconds);
	printf("|-------------------------------------------------|\n");
	printf("|\033[0;36mMagic Number         \033[1;37m|         \033[1;36m%-10i        \033[1;37m|\n", msg->magicNumber);
	printf("|-------------------------------------------------|\033[0m\n\n");
}

// Finaliza el consumidor
void killConsumer(){
	// Establece el tiempo de usuario
	consumer.inUser = (long int)userTime.ru_utime.tv_usec;

	// Disminuye el semaforo de consumidores
	sem_wait(consumer.semConsumers);

	// Establece los valores de tiempos y disminuye la cantidad de consumidores (global)
	consumer.consumers->numTotalConsumers--;
	consumer.consumers->waitingTime += consumer.inWait;
	consumer.consumers->blockedTime += consumer.inBlock;
	consumer.consumers->userTime += consumer.inUser;

	// Aumenta el semaforo de consumidores
	sem_post(consumer.semConsumers);

	// Establece la variable de finalizacion
	kill = 1;
}

unsigned long factorial(unsigned long _num){
	if (_num == 0)
		return 1;
	return (_num * factorial(_num - 1));
}

// Distribucion de Poisson = (Lambda^r/r!)*e^(-Lambda)*10
double poisson(double _lambda){
	float r = rand() / (RAND_MAX + 1.0);
	float num = (powf(_lambda, r) / factorial(r)) * exp(-_lambda);
	printf("Se realiza una espera de %f s\n", num * 10);
	return num * 10;
}

// *********** Función para concatenar el nombre del buffer con el tipo de objeto de mem a crear **************
char * setMemObjName(char *_name, const char *_tag){
	char *tagName = (char *) calloc(strlen(_name) + strlen(_tag), sizeof(char));
	strcpy(tagName, _name);
	strcat(tagName, _tag);
	return tagName;
}
