#ifndef CLIENTE_HPP
#define CLIENTE_HPP

#include <iostream>
#include <cstring> 
#include <unistd.h> 
#include <arpa/inet.h> 
#include <sys/socket.h>  
#include <netdb.h>
#include <csignal>

/******************************************************
###                 INTEGRANTES                     ###
###     Collazo, Ignacio Lahuel     - CONFIDENCE    ### 
###     Pozzato, Alejo Martin       - CONFIDENCE    ### 
###     Rodriguez, Emanual          - CONFIDENCE    ###
###     Rodriguez, Pablo            - CONFIDENCE    ### 
###     Vazquez Petracca, Pablo N.  - CONFIDENCE    ###
******************************************************/

#define MIN_PORT 1024
#define MAX_PORT 65535

using namespace std;

struct message {
    char type;
    char from[128];
    char content[1024];
};

struct question{
    char quest[256];
    int opctionCorrect;
    char optionOne[64];
    char optionTwo[64];
    char optionThree[64];
};

struct cli{
    static int getUserResponse(char type, question q){
        int option;

        cout << type << ") "<< q.quest << endl;
        cout << "   1)" << q.optionOne << endl;
        cout << "   2)" << q.optionTwo << endl;
        cout << "   3)" << q.optionThree << endl;
                    
        do {
            cout << "Ingresa una opción: ";
            cin >> option;

            if (option < 1 || option > 3) {
                cout << "Opción inválida. Por favor, ingrese una opción: " << endl;
            }
                        
            if (cin.fail()) {
                cin.clear();
                cin.ignore(10000, '\n');
            }

        } while (option < 1 || option > 3);

        return option;
    }
};

class Cliente{
    private:
        int port;
        string ip;
        string nickname;
        struct sockaddr_in socketConfig;
        int socketCommunication;
        int bytesRecv = 0;
        message request, response;
        int begin = true;

    public:
        ~Cliente() {
            this->bytesRecv = -1;
        }

        static void signal_handler(int signal_number){
            return;
        }

        void printHelp(const char* script){
            cout << "Uso: " <<  script << " -p|--puerto <PUERTO> -s|--servidor <IP> -n|--nickname <NICKNAME> " << endl;
            cout << "Cliente para conectarse al servidor de Preguntados." << endl;
            cout << "Opciones:" << endl;
            cout << "  -p, --puerto         Nro de puerto (Requerido)" << endl;
            cout << "  -s, --servidor       Dirección IP o nombre del servidor (Requerido)" << endl;
            cout << "  -s, --nickname       Nickname del usuario (Requerido)" << endl;
            cout << "  -h, --help           Muestra la ayuda" << endl;
        }

        void printError(const char* script){ 
            cout << "Error de sintaxis: Utilice " << script << " -h o --help para obtener ayuda." << endl;
        }

        bool validateParameters(int argc, const char* argv[]){
            if(argc == 2 && (string(argv[1]) == "-h" || string(argv[1]) == "--help")){
                this->printHelp(argv[0]);
                return EXIT_FAILURE;
            }

            if(argc != 7){
                this->printError(argv[0]);
                return EXIT_FAILURE;
            }

            while(argc > 1){
                string option = argv[argc-2];
                string value = argv[argc-1]; 

                try{
                    if(option == "-p" || option == "--puerto"){
                        int port =  stoi(value);

                        if(port < MIN_PORT || port > MAX_PORT){
                            cout << "Error de parámetro: El rango válido del puerto es de " <<  MIN_PORT << " a " << MAX_PORT << "." << endl;
                            return EXIT_FAILURE;
                        }

                        this->port = port;
                    }else if(option == "-s" || option == "--servidor"){
                        this->ip = value;
                    }else if(option == "-n" || option == "--nickname")
                        this->nickname = value;
                    else{
                        this->printError(argv[0]);
                        return EXIT_FAILURE;
                    }
                }catch(invalid_argument& e){
                    cout << "Error de tipo en el parámetro: El parámetro " << option << " no es válido." << endl;
                    return EXIT_FAILURE;
                }
                
                argc-=2;
            }

            return EXIT_SUCCESS;
        }

       bool init(){

            // Resolver el nombre del host.
            struct hostent* he = gethostbyname(this->ip.c_str());
            if (he == nullptr) {
                herror("Error al resolver el nombre del servidor");
                return EXIT_FAILURE;
            }

            // Limpiar la estructura socketConfig.
            memset(&this->socketConfig, 0, sizeof(this->socketConfig));
            
            // Configurar los parámetros del socket.
            this->socketConfig.sin_family = AF_INET; // Usar IPv4
            this->socketConfig.sin_port = htons(this->port); // Convertir el puerto a red byte order
            memcpy(&this->socketConfig.sin_addr, he->h_addr, he->h_length); // Copiar la dirección IP

            // Crear el socket
            if ((this->socketCommunication = socket(this->socketConfig.sin_family, SOCK_STREAM, 0)) < 0){
                cout << "Error en la creación del socket." << endl;
                return EXIT_FAILURE;
            }

            // Configurar el timeout del socket.
            struct timeval timeout;
            timeout.tv_sec = 360;  // Segundos
            timeout.tv_usec = 0;  // Microsegundos

            // Establecer timeout para los mensaje en el socket.
            if (setsockopt(this->socketCommunication, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0 || 
                (setsockopt(this->socketCommunication, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout)) < 0)) {
                cout << "Error al configurar el timeout del socket." << endl;
                close(this->socketCommunication);
                return EXIT_FAILURE;
            }

            // Establecer conexión con el servidor.
            if ((connect(this->socketCommunication,(struct sockaddr *)&(this->socketConfig), sizeof(this->socketConfig))) < 0)
            {
                cout << "Error en la conexión con el servidor." << endl;
                close(this->socketCommunication);
                return EXIT_FAILURE;
            }

            return EXIT_SUCCESS;
        }

        bool run(){
            memset(&this->request, 0, sizeof(this->request));
            int option;

            cout << "Esperando rivales para comenzar la partida..." << endl;

            while ((this->bytesRecv = recv(this->socketCommunication, &this->request, sizeof(this->request), 0)) > 0)
            {
                if(this->request.type == 'Q'){
                    question q;
                    memcpy(&q, this->request.content, sizeof(q));

                    if(this->begin){
                        cout << "================================" << endl;
                        cout << "Partida iniciada. ¡Buena suerte!" << endl;
                        cout << "================================" << endl;
                        this->begin = false;
                    }

                    option = cli::getUserResponse(this->request.type, q); //DEVOLVER RESPUESTA

                    if(option == q.opctionCorrect){
                        cout << "--------------------" << endl;
                        cout << "¡Respuesta correcta!" << endl;
                        cout << "--------------------" << endl;
                    }else{
                        cout << "----------------------" << endl;
                        cout << "¡Respuesta incorrecta!" << endl;
                        cout << "----------------------" << endl;
                    }

                    memset(&this->response, 0, sizeof(this->response));
                    //memcpy(msg.from, this->nickname.c_str(), sizeof(this->nickname.c_str()));
                    this->response.type = 'R';
                    send(this->socketCommunication, &this->response, sizeof(this->response), 0);
                }

                if(this->request.type == 'E'){
                    cout << "======================================================" << endl;
                    cout << "Partida finalizada. Esperando a los demás jugadores..." << endl;
                    cout << "======================================================" << endl;
                    this->begin = true;
                }

                if(this->request.type == 'Z'){
                    cout << "¿Desea jugar otra partida?" << endl;
                    cout << "   1) SI" << endl;
                    cout << "   2) NO" << endl;
                    
                    do {
                        cout << "Ingresa una opción (1 o 2): ";
                        cin >> option;

                        // Verificar si la opción es válida
                        if (option < 1 || option > 2) {
                            cout << "Opción inválida. Por favor, ingresa 1 o 2" << endl;
                        }
                        
                        // Limpiar caracter invalido
                        if (cin.fail()) {
                            cin.clear();
                            cin.ignore(10000, '\n');
                        }

                    } while (option < 1 || option > 2);

                    memset(&this->response, 0, sizeof(this->response));
                    if(option == 1)
                        this->response.type = 'S';
                    else 
                        this->response.type = 'N';

                    //memcpy(msg.from, this->nickname.c_str(), sizeof(this->nickname.c_str()));
                    send(this->socketCommunication, &this->response, sizeof(this->response), 0);
                }
                
                memset(&this->response, 0, sizeof(this->response));
                this->bytesRecv = 0;
            }

            this->bytesRecv = 0;
            memset(&this->response, 0, sizeof(this->response));
            memset(&this->request, 0, sizeof(this->request));
            close(this->socketCommunication);

            return EXIT_SUCCESS;
        }

        bool protocol(message msg){


            return EXIT_SUCCESS;
        }
};

#endif