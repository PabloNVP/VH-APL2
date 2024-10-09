all : servidor cliente

servidor: servidor.cpp servidor.hpp
	g++ -o servidor servidor.cpp

cliente: cliente.cpp cliente.hpp
	g++ -o cliente cliente.cpp

force:
	kill -9 $(ps -e | grep servidor | awk '{ print $1 }')

cleanall:
	rm -f servidor cliente