/*  
*   Este código es una modificación de los programas del sitio: https://github.com/WhileTrueThenDream/ExamplesCLinuxUserSpace
*   Programador: WhileTrueThenDream, 08-03-20.
*/
#include "sharedMemoryMapping.h"
#define ERROR -1

int fd;
struct stat sharedMemObj;

// *********** Función para crear nuevo buffer **************
void newSharedMemory(char * _bufferName, int _size){
	fd = shm_open (_bufferName, O_CREAT | O_RDWR , 00700);  // Crear buffer | Obtener File descriptor
	if(fd == ERROR){
	   printf("Error al crear el buffer de memoria compartida.\n");
	   exit(1);
	}
	if(ftruncate(fd, _size) == ERROR) {						// Darle tamaño al buffer
	   printf("Error al definir el tamaño del buffer de memoria compartida.\n");
	   exit(1);
	}
}

// *********** Función para mapear la memoria **************
void * mapSharedMemory(char * _bufferName){
	fd = shm_open (_bufferName,  O_RDWR  , 0);			// Abrir buffer | Obtener File descriptor
	if(fd == ERROR){
	   printf("Error file descriptor %s\n", strerror(errno));
	   exit(1);
	}
	if(fstat(fd, &sharedMemObj) == ERROR){					// Obtener info del objeto de memoria
	   printf("Error getting stat struct.\n");
	   exit(1);
	}
															// Mapear a mem virtual
	void * ptr = mmap(NULL, sharedMemObj.st_size, PROT_WRITE | PROT_READ, MAP_SHARED, fd, 0);
	if(ptr == MAP_FAILED){
	  printf("Map failed in write process: %s\n", strerror(errno));
	  exit(1);
	}
	return ptr;
}

// *********** Función para obtener el tamaño del bloque de memoria **************
int getMemorySize(char * _bufferName){
	fd = shm_open (_bufferName,  O_RDWR  , 0); 			// Abrir buffer | Obtener File descriptor
	if(fd == ERROR){
	   printf("Error file descriptor %s\n", strerror(errno));
	   exit(1);
	}
	if(fstat(fd, &sharedMemObj) == ERROR){					// Obtener info del objeto de memoria
	   printf("Error getting stat struct.\n");
	   exit(1);
	}
	return sharedMemObj.st_size;
}

// *********** Función para escribir en la memoria compartida **************
void writeSharedMemory(void * _ptr, void * _data, int _size, int _offset){
	memcpy(_ptr + (_offset * _size), _data, _size);
}

// *********** Función para eliminar el bloque de memoria **************
void removeSharedMemory(char * _bufferName){
	shm_unlink(_bufferName);
}

// *********** Función para crear los semáforos *********************
void createSemaphore(char *_name, int _value){
	if (sem_open(_name, O_CREAT | O_EXCL, S_IRWXU, _value) == SEM_FAILED){
		perror("sem_open(3) error");
        exit(EXIT_FAILURE);
	}
}

// *********** Función para obtener buffer del semaforo **************
sem_t * getSemaphore(char *_name){
	sem_t * semaphore = sem_open(_name, O_RDWR);

	if (semaphore == SEM_FAILED){
		perror("sem_open(3) error");
        exit(EXIT_FAILURE);
	}
	return semaphore;
}