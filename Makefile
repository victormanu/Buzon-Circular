all: producer initializer consumer finalizer

initializer:
	gcc sharedMemoryMapping.c initializer.c -o exec/init -lm -lrt -pthread

producer:
	gcc sharedMemoryMapping.c producer.c -o exec/prod -lm -lrt -pthread
	
consumer:
	gcc sharedMemoryMapping.c consumer.c -o exec/consu -lm -lrt -pthread

finalizer:
	gcc sharedMemoryMapping.c finalizer.c -o exec/fin -lm -lrt -pthread
