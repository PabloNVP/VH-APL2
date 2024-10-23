# Variables
CXX = g++

all: servidor cliente

servidor: servidor.cpp servidor.hpp
	$(CXX) -o servidor4 servidor.cpp -lpthread -lrt

cliente: cliente.cpp cliente.hpp
	$(CXX) -o cliente4 cliente.cpp -lpthread -lrt

force:
	pkill -SIGUSR1 servidor4

cleanall:
	rm -f servidor4 cliente4 /dev/shm/mem4 /dev/shm/sem.cliente4 /dev/shm/sem.conexion4 /dev/shm/sem.servidor4 /tmp/cliente4.lock /tmp/servidor4.lock
