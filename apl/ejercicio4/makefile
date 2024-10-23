all : servidor cliente

servidor: servidor.cpp servidor.hpp -I
	g++ -o servidor4 servidor.cpp

cliente: cliente.cpp cliente.hpp -I
	g++ -o cliente4 cliente.cpp

force:
	pkill -SIGUSR1 servidor4

cleanall:
	rm -f servidor4 cliente4
