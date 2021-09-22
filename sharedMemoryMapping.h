#ifndef _SHAREDMEMORYMAPPING_H
#define _SHAREDMEMORYMAPPING_H

#include <stdio.h> // Para los prints
#include <stdlib.h> // Libreria estandar
#include <sys/stat.h> // Obtener un struct de datos del sistema
#include <sys/types.h> // Para asuntos relacionados a time, pthread
#include <sys/resource.h> // Se utiliza para obtener datos como tiempo de kernel
#include <sys/mman.h> // Para utilizar mmap
#include <fcntl.h> // Manejar fd
#include <unistd.h> // Libreria estandar de constantes simbolicas y tipos (sleep, ftruncate, getpid)
#include <semaphore.h> // Define el tipo sem_t para manejo de semaforos
#include <string.h> // Para strings
#include <time.h> // Para tiempo
#include <math.h> // Para operaciones matematicas
#include <errno.h> // Manejar errores

		
// ********************** Funciones de la biblioteca ****************************
void newSharedMemory(char * _bufferName, int _size);
void * mapSharedMemory(char * _bufferName);
void writeSharedMemory(void * _ptr, void * _data, int _size, int _offset);
void removeSharedMemory(char * _bufferName);
int getMemorySize(char * _bufferName);

sem_t * getSemaphore(char *_name);
void createSemaphore(char *_name, int _value);


#endif