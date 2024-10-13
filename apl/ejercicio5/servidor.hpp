#ifndef SERVIDOR_HPP
#define SERVIDOR_HPP

#include <iostream>
#include <fstream> 
#include <cstring>  
#include <arpa/inet.h> 
#include <sys/socket.h>  
#include <unistd.h>   
#include <thread>
#include <csignal>
#include <map>
#include <vector>
#include <algorithm>
#include <random>    
#include <mutex>
#include <condition_variable>

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
 * Estructura de datos del cliente
******************************************/
struct client {
    string name;
    int score;
    int questions;

    void setName(string name){
        if(this->name == "")
            this->name = name;
    }
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

/******************************************
 * Estructura de datos de la sala
******************************************/
struct room{
    int num;
    vector<int> serieQuestions;
    condition_variable cv;
    int endThread = 0;
    mutex mtxroom;

    void loadSerieQuestions(int count, int limit) {
        vector<int> numbers;

        for (int i = 0; i < count; i++) 
            numbers.push_back(i);

        random_device rd;  // Semilla aleatoria
        mt19937 gen(rd()); // Generador de números aleatorios
        shuffle(numbers.begin(), numbers.end(), gen);

        serieQuestions = vector<int>(numbers.begin(), numbers.begin() + limit);
    }
};

static bool running = false;

class Servidor {
    private:
        int port; // Puerto del servidor.
        int usersLimit; // Limite de usuarios por partida.
        int questionsLimit; // Cantidad de preguntas por partida.
        FILE* file; // Archivo de preguntas.
        struct sockaddr_in socketConfig; // Socket de configuración del servidor.
        int socketListening; // Socket de escucha del servidor.
        map<int, client> users; // Map de socket por usuario.
        map<int, map<int, client>> games; // Map de partida por usuario.
        map<int, question> questions;
        int gamesCount; // Cantidad de partidas.
        int questionsCount; // Cantidad de preguntas en el archivo.
        mutex mtxUsers; // Semaforo para el Map de usuarios.
        mutex mtxGames; // Semaforo para el Map de partidas.
        mutex mtxQuestions; // Semaforo para el Map de preguntas.

    public:
        Servidor(): gamesCount(0) , questionsCount(0) {};

        ~Servidor() {
            running = false;
        }

        int getUserLimit(){ return this->usersLimit; }

        static void signal_handler(int signal_number){ 
            running = false;
        }

        bool validateParameters(int argc, const char* argv[]){
            if(argc == 2 && (string(argv[1]) == "-h" || string(argv[1]) == "--help")){
                this->printHelp(argv[0]);
                return EXIT_FAILURE;
            }

            if(argc != 9){
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
                    }else if(option == "-u" || option == "--usuarios")
                        this->usersLimit = stoi(value);
                    else if(option == "-a" || option == "--archivo"){
                        this->file = fopen("preguntas.csv", "r");

                        if (this->file == nullptr) {
                            cout << "Error de parámetro: El archivo no existe." << endl;
                            return EXIT_FAILURE; 
                        }
                    }
                    else if(option == "-c" || option == "--cantidad")
                        this->questionsLimit = stoi(value);
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
        
        void printHelp(const char* script){
            cout << "Uso: " << script << " -p|--puerto <PUERTO> -u|--usuarios <NRO USUARIOS> -a|--archivo <ARCHIVO> -c|--cantidad <NRO PREGUNTAS>" << endl;
            cout << "Inicia el servidor de Preguntados." << endl;
            cout << "Opciones:" << endl;
            cout << "  -p, --puerto         Nro de puerto (Requerido)" << endl;
            cout << "  -u, --usuarios       Cantidad de usuarios a esperar para iniciar la sala. (Requerido)" << endl;
            cout << "  -a, --archivo        Archivo con las preguntas (Requerido)" << endl;
            cout << "  -c, --cantidad       Cantidad de preguntas por partida (Requerido)" << endl;
            cout << "  -h, --help           Muestra la ayuda" << endl;
        }

        void printError(const char* script){ 
            cout << "Error de sintaxis: Utilice " << script << " -h o --help para obtener ayuda." << endl;
        }

        bool init(){
            // Configurar el socket de escucha.
            if((this->socketListening = socket(AF_INET, SOCK_STREAM, 0)) < 0){
                cout << "Error al crear el socket de escucha." << endl;
                return EXIT_FAILURE;
            }

            // Limpiar la estructura y configurar
            memset(&this->socketConfig, 0, sizeof(this->socketConfig)); // Limpiar memoria.
            this->socketConfig.sin_family = AF_INET;  // Familia de direcciones (IPv4)
            this->socketConfig.sin_port = htons(this->port);  // Puerto en formato de red
            this->socketConfig.sin_addr.s_addr = htonl(INADDR_ANY);  // Aceptar conexiones desde cualquier IP

            // Asociar el socket con la dirección IP y puerto
            if (bind(this->socketListening, (struct sockaddr*)&(this->socketConfig), sizeof(this->socketConfig)) < 0) {
                cout << "Error al configurar el socket de escucha." << endl;
                close(this->socketListening);
                return EXIT_FAILURE;
            }

            // Cargamos las preguntas.
            if(this->loadQuestions()){
                cout << "Error al cargar las preguntas desde el archivo." << endl;
                fclose(this->file);
                return EXIT_FAILURE;
            }
            
            return EXIT_SUCCESS;
        }

        // Configurar el manejador de señales para instrucciones bloqueantes.
        void configSignal(){
            struct sigaction sa;
            sa.sa_handler = signal_handler;
            sigemptyset(&sa.sa_mask);
            sa.sa_flags = 0;
            sigaction(SIGUSR1, &sa, nullptr);
        }

        bool run(){

            this->configSignal();

            listen(this->socketListening, 10);
            cout << "Esperando nuevos usuarios..." << endl;

            running = true;

            while (running)
            {
                int socketComunicacion = accept(this->socketListening, (struct sockaddr *)NULL, NULL);

                 if(running == false){
                    if(this->games.size()>0){
                        cout << " >> Se intenta cerrar el servidor pero hay partidas existentes." <<  endl;
                        running = true;
                        continue;
                    }

                    cout << " >> Se esta cerrando el servidor..." <<  endl;
                    break;
                }   

                cout << " >> Se ha conectado un nuevo usuario con id: " << socketComunicacion << endl;

                this->createNewUser(socketComunicacion);

                if(this->users.size() == this->usersLimit){
                    this->mtxUsers.lock();
                    this->mtxGames.lock();
                    this->games[gamesCount] = move(this->users);
                    this->mtxGames.unlock();
                    this->users.clear();
                    this->mtxUsers.unlock();

                    thread game(gameStart, this, this->gamesCount);
                    game.detach();

                    this->gamesCount++;
                }
            }

            fclose(this->file);
            close(this->socketListening);

            return EXIT_SUCCESS;
        }

        void createNewUser(int socketUser){
            client newUser {"", 0, 0};

            mtxUsers.lock();
            this->users.insert(make_pair(socketUser, newUser));
            mtxUsers.unlock();
        }

        static void gameStart(Servidor* server, int numRoom){
            vector<thread> tts;
            room newRoom;
            newRoom.num = numRoom;
            newRoom.loadSerieQuestions(server->questionsCount, server->questionsLimit);

            for (auto& user : server->games[numRoom]) {        
                tts.push_back(thread(&handleClient, server, &newRoom, user.first, &user.second));
            }

            cout << " >> Se ha iniciado una partida con numero de sala: " << numRoom << endl;

            for(auto& tt : tts){
                tt.join();
            }

            server->mtxGames.lock();
            server->games.erase(numRoom);
            server->mtxGames.unlock();

            cout << " >> Se ha finalizado una partida con numero de sala: " << numRoom << endl;
        }

        static void handleClient(Servidor* server, room* yourRoom, int socket, client* userClient){
            
            int numQuest = 0;
            int bytesRecv = 0;
            message msg;
            
            do{
                //RECIBO
                if(msg.type == 'S' || msg.type == 'N'){
                    if(msg.type == 'S' )
                        cout << "QUIERO JUGAR OTRA" << endl;
                    else
                        cout << "NO QUIERO JUGAR OTRA" << endl;
                    
                    memset(&msg, 0, sizeof(msg));
                    break;
                }

                if(msg.type == 'R'){
                    userClient->setName(msg.from);

                    if(server->questions[yourRoom->serieQuestions[numQuest-1]].opctionCorrect == stoi(msg.content)){
                        userClient->score++;
                    }

                    cout << userClient->name << " ; " << userClient->score << endl;

                    if(numQuest == server->questionsLimit){

                        unique_lock<mutex> lock(yourRoom->mtxroom);
                    
                        yourRoom->endThread++;
                        
                        if (yourRoom->endThread == server->usersLimit) {
                            yourRoom->cv.notify_all();
                        } else {
                            msg.type = 'E';
                            memset(msg.content, 0, sizeof(msg.content));
                            send(socket, &msg, sizeof(msg), 0);
                            yourRoom->cv.wait(lock, [&] { return yourRoom->endThread == server->getUserLimit(); });
                        }

                        /** Calcular resultado */
                        int max = -1;
                        memset(&msg, 0, sizeof(msg));
                        msg.type = 'Z';
                        for (auto& user : server->games[yourRoom->num]) {        
                            if(max < user.second.score){
                                memcpy(msg.content, user.second.name.c_str(), user.second.name.size());
                                max = user.second.score;
                            }
                        }
                        
                        send(socket, &msg, sizeof(msg), 0);
                        continue;
                    }
                }

                //LIMPIEZA
                memset(&msg, 0, sizeof(msg));
                bytesRecv = 0;

                //ENVIO
                msg.type = 'Q';
                memcpy(msg.content, &server->questions[yourRoom->serieQuestions[numQuest]], sizeof(server->questions[yourRoom->serieQuestions[numQuest]]));
                numQuest++;
                send(socket, &msg, sizeof(msg), 0);
            }while ((bytesRecv = recv(socket, &msg, sizeof(msg), 0)) > 0);
            
            close(socket);
        }

        bool loadQuestions() {
            try{
                question quest;
               
                while (fscanf(this->file, "%255[^,],%d,%63[^,],%63[^,],%63[^\n]\n", 
                        quest.quest, &quest.opctionCorrect, quest.optionOne, quest.optionTwo, quest.optionThree) == 5) {
                    this->questions[this->questionsCount++] = quest;
                    memset(&quest, 0, sizeof(quest));
                }

                return EXIT_SUCCESS;

            }catch(exception& e){
                return EXIT_FAILURE;
            }
        }   
};

#endif