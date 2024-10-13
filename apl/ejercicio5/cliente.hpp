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

/******************************************
 * Estructura del mensaje cliente/servidor
******************************************/
struct message {
    char type;
    char from[128];
    char content[1024];
};

/******************************************
 * Estructura de formato de las preguntas
******************************************/
struct question{
    char quest[256];
    int opctionCorrect;
    char optionOne[64];
    char optionTwo[64];
    char optionThree[64];
};

/***********************************************
 * Estructura que maneja la salida por pantalla
***********************************************/
struct consolePrinter{
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

    void printBeginGame(){
        cout << "================================" << endl;
        cout << "Partida iniciada. ¡Buena suerte!" << endl;
        cout << "================================" << endl;
    }

    void printCorrectQuestion(){
        cout << "--------------------" << endl;
        cout << "¡Respuesta correcta!" << endl;
        cout << "--------------------" << endl;
    }

    void printIncorrectQuestion(){
        cout << "----------------------" << endl;
        cout << "¡Respuesta incorrecta!" << endl;
        cout << "----------------------" << endl;
    }

    void printWinner(char* winner){
        cout << "=========================================" << endl;
        cout << "El ganador de la partida es: " << winner << endl;
        cout << "=========================================" << endl;
    }

    void printNextGame(){
        cout << "¿Desea jugar otra partida?" << endl;
        cout << "   1) SI" << endl;
        cout << "   2) NO" << endl;
    }

    void printOptionsQuestion(char type, question* q){
        cout << type << ") "<< q->quest << endl;
        cout << "   1)" << q->optionOne << endl;
        cout << "   2)" << q->optionTwo << endl;
        cout << "   3)" << q->optionThree << endl;
    }

    void printWaitingGame(){
        cout << "Esperando rivales para comenzar la partida..." << endl;
    }

};

class Cliente{
    private:
        consolePrinter cp;
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
            cp.printHelp(script);
        }

        void printError(const char* script){ 
            cp.printError(script);
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
            int option, numQuest=0;

            cp.printWaitingGame();

            while ((this->bytesRecv = recv(this->socketCommunication, &this->request, sizeof(this->request), 0)) > 0)
            {
                if(this->request.type == 'Q'){
                    question q;
                    memcpy(&q, this->request.content, sizeof(q));

                    if(this->begin){
                        cp.printBeginGame();
                        this->begin = false;
                    }

                    option = this->getUserResponse('A'+(numQuest++), &q, 1, 3);

                    if(option == q.opctionCorrect){
                        cp.printCorrectQuestion();
                    }else{
                        cp.printIncorrectQuestion();
                    }

                    memset(&this->response, 0, sizeof(this->response));
                    this->response.type = 'R';
                    memcpy(this->response.from, this->nickname.c_str(), sizeof(this->nickname.c_str()));
                    memcpy(this->response.content, to_string(option).c_str(), to_string(option).size());
                    send(this->socketCommunication, &this->response, sizeof(this->response), 0);
                }

                if(this->request.type == 'E'){
                    cout << "Partida finalizada. Esperando a los demás jugadores..." << endl;
                    this->begin = true;
                }

                if(this->request.type == 'Z'){
                    
                    memset(&this->response, 0, sizeof(this->response));
                    
                    cp.printWinner(request.content);

                    option = this->getUserResponse(this->response.type, nullptr, 1, 2);

                    if(option == 1)
                        this->response.type = 'S';
                    else 
                        this->response.type = 'N';

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

        int getUserResponse(char type, question* q, int minOpc, int maxOpc){
            int option;

            if(type == 'Z'){
                cp.printNextGame();
            }else{
               cp.printOptionsQuestion(type, q);
            }
                        
            do {
                cout << "Ingresa una opción: ";
                cin >> option;

                if (option < minOpc || option > maxOpc) {
                    cout << "Opción inválida. Por favor, ingrese una opción: " << endl;
                }
                            
                if (cin.fail()) {
                    cin.clear();
                    cin.ignore(10000, '\n');
                }

            } while (option < minOpc || option > maxOpc);

            return option;
        }
};

#endif