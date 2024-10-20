all : servidor cliente

servidor: servidor.cpp servidor.hpp
	g++ -o servidor servidor.cpp

cliente: cliente.cpp cliente.hpp
	g++ -o cliente cliente.cpp

exit:
	pkill -10 servidor

cleanall:
	rm -f servidor cliente /dev/shm/sem.server_semaphore