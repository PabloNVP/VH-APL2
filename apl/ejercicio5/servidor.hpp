#ifndef SERVIDOR_HPP
#define SERVIDOR_HPP

/******************************************
 * Librerias del servidor
******************************************/
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
#include <semaphore.h>
#include <fcntl.h> // O_CREAT, O_EXCL
#include <sys/stat.h> // S_IRUSR, S_IWUSR

/**############ INTEGRANTES ###############
###     Collazo, Ignacio Lahuel         ### 
###     Pozzato, Alejo Martin           ### 
###     Rodriguez, Emanual              ###
###     Rodriguez, Pablo                ### 
###     Vazquez Petracca, Pablo N.      ### 
#########################################*/
 
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
    int socket;
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
    int countUsers;
    vector<int> serieQuestions;
    condition_variable cv;
    int endThread = 0;
    mutex mtxroom;

    room(int numRoom, int limitUsers, int countQuest, int limitQuest){
        this->num = numRoom;
        this->countUsers = limitUsers;
        this->loadSerieQuestions(countQuest, limitQuest);
    }

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

/***********************************************
 * Variable que maneja el estado del servidor
***********************************************/
static bool running = false;

/***********************************************
 * Clase principal del servidor de preguntados
***********************************************/
class Servidor {
    private:
        int port; // Puerto del servidor.
        int usersLimit; // Limite de usuarios por partida.
        int questionsLimit; // Cantidad de preguntas por partida.
        FILE* file; // Archivo de preguntas.
        struct sockaddr_in socketConfig; // Socket de configuración del servidor.
        int socketListening; // Socket de escucha del servidor.
        vector<client> users; // Vector de usuarios.
        map<int, vector<client>> games; // Map de partida por usuario.
        map<int, question> questions;
        int gamesCount; // Cantidad de partidas.
        int questionsCount; // Cantidad de preguntas en el archivo.
        mutex mtxUsers; // Semaforo para el Map de usuarios.
        mutex mtxGames; // Semaforo para el Map de partidas.
        mutex mtxQuestions; // Semaforo para el Map de preguntas.
        const char* sem_path = "/server_semaphore";

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
                        this->port =  stoi(value);

                        if(this->port < MIN_PORT || this->port > MAX_PORT){
                            cout << "Error de parámetro: El rango válido del puerto es de " <<  MIN_PORT << " a " << MAX_PORT << "." << endl;
                            return EXIT_FAILURE;
                        }
                    }else if(option == "-u" || option == "--usuarios"){
                        this->usersLimit = stoi(value);

                        if(this->usersLimit < 1){
                            cout << "Error de parámetro: La cantidad de usuarios debe ser mayor a 0." << endl;
                            return EXIT_FAILURE;
                        }
                    }else if(option == "-a" || option == "--archivo"){
                        this->file = fopen(value.c_str(), "r");

                        if (this->file == nullptr) {
                            cout << "Error de parámetro: El archivo no existe." << endl;
                            return EXIT_FAILURE; 
                        }
                    }
                    else if(option == "-c" || option == "--cantidad"){
                        this->questionsLimit = stoi(value);

                        if(this->questionsLimit < 1){
                            cout << "Error de parámetro: La cantidad de preguntas debe ser mayor a 0." << endl;
                            return EXIT_FAILURE;
                        }
                    }else{
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

        bool isRunningServer(){
            sem_t* sem = sem_open(this->sem_path, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 1);
    
            if (sem == SEM_FAILED) {
                sem_close(sem);
                return EXIT_FAILURE;
            }

            sem_close(sem);
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

            // Verificar si hay otra instancia del servidor ejecutandose.
            if(this->isRunningServer()){
                cout << "El servidor ya está en ejecución." << endl;
                return EXIT_FAILURE;
            }

            // Configurar el socket de escucha.
            if((this->socketListening = socket(AF_INET, SOCK_STREAM, 0)) < 0){
                cout << "Error al crear el socket de escucha." << endl;
                return EXIT_FAILURE;
            }

            int opt = 1; // Permite reutilizar la dirección y puerto rápidamente
            if (setsockopt(this->socketListening, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
                cout << "Error configurando la opción SO_REUSEADDR\n" << endl;
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

            thread tt(&checkDisconnect, this);
            tt.detach();

            while (running)
            {
                int socketComunicacion = accept(this->socketListening, (struct sockaddr *)NULL, NULL);

                if(running == false){
                    if(this->games.size()>0){
                        cout << " >> Se intenta cerrar el servidor pero hay partidas activas." <<  endl;
                        running = true;
                        continue;
                    }

                    cout << " >> Se esta cerrando el servidor..." <<  endl;

                    mtxUsers.lock();
                    for(auto& user : this->users){
                        close(user.socket);
                    }
                    mtxUsers.unlock();

                    break;
                }   

                if( socketComunicacion >= 0){
                    cout << " >> Se ha conectado un nuevo usuario con id: " << socketComunicacion << endl;

                    this->createNewUser(socketComunicacion);
                }

                this->checkCreateGame();
            }

            running = false;
            sem_unlink(this->sem_path);
            fclose(this->file);
            close(this->socketListening);

            return EXIT_SUCCESS;
        }

        void createNewUser(int socketUser){
            client newUser {socketUser, "", 0, 0};

            mtxUsers.lock();
            this->users.push_back(newUser);
            mtxUsers.unlock();
        }

        void checkCreateGame(){
           
           this->mtxUsers.lock();
        
            if(this->users.size() >= this->usersLimit){
                
                this->mtxGames.lock();

                this->games[this->gamesCount].insert(this->games[this->gamesCount].end(),
                    make_move_iterator(this->users.begin()),
                    make_move_iterator(this->users.begin() + this->usersLimit));

                this->users.erase(this->users.begin(), this->users.begin() + this->usersLimit);

                thread game(gameStart, this, this->gamesCount);
                game.detach();

                this->gamesCount++;

                this->mtxGames.unlock();
            }

            this->mtxUsers.unlock();
        }

        static void gameStart(Servidor* server, int numRoom){
            vector<thread> tts;
            room newRoom(numRoom, server->usersLimit, server->questionsCount, server->questionsLimit);

            for (auto& user : server->games[numRoom]) {        
                tts.push_back(thread(&handleClient, server, &newRoom, &user));
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

        static void handleClient(Servidor* server, room* yourRoom, client* userClient){
            int numQuest = 0;
            int bytesRecv = 0;
            message request, response;

            do{
                if(request.type == 'X'){
                    memset(&response, 0, sizeof(response));
                    memset(&request, 0, sizeof(request));
                    bytesRecv = -1;
                    yourRoom->countUsers--;
                    userClient->score = -1;
                    yourRoom->cv.notify_all();
                    break;
                }

                if(request.type == 'N'){
                    memset(&response, 0, sizeof(response));
                    response.type = 'N';
                    send(userClient->socket, &response, sizeof(response), 0);
                    
                    memset(&request, 0, sizeof(request));
                    break;
                }

                if(request.type == 'R'){
                    userClient->setName(request.from);

                    if(server->questions[yourRoom->serieQuestions[numQuest-1]].opctionCorrect == stoi(request.content)){
                        userClient->score++;
                    }

                    if(numQuest == server->questionsLimit){

                        unique_lock<mutex> lock(yourRoom->mtxroom);
                    
                        yourRoom->endThread++;
                        
                        if (yourRoom->endThread == yourRoom->countUsers) {
                            yourRoom->cv.notify_all();
                        } else {
                            response.type = 'E';
                            memset(response.content, 0, sizeof(response.content));
                            send(userClient->socket, &response, sizeof(response), 0);
                            yourRoom->cv.wait(lock, [&] { return yourRoom->endThread >= yourRoom->countUsers; });
                        }

                        string result = getResults(&server->games[yourRoom->num]);

                        memset(&response, 0, sizeof(response));
                        response.type = 'Z';
                        memcpy(response.content, result.c_str(), result.size());
                        send(userClient->socket, &response, sizeof(response), 0);
                        continue;
                    }
                }

                //ENVIO
                response.type = 'Q';
                memcpy(response.content, &server->questions[yourRoom->serieQuestions[numQuest]], 
                        sizeof(server->questions[yourRoom->serieQuestions[numQuest]]));
                numQuest++;
                send(userClient->socket, &response, sizeof(response), 0);
            }while ((bytesRecv = recv(userClient->socket, &request, sizeof(request), 0)) > 0);
            
            if (bytesRecv <= 0) {
                cout << " >> Se ha cerrado conexión con el usuario con id: " << userClient->socket << endl;
            }else{
                cout << " >> Se ha desconectado el usuario con id: " << userClient->socket << endl;
            }

            close(userClient->socket);
        }

        static void checkDisconnect(Servidor* server) {
            while (running) {
                for (auto it = server->users.begin(); it != server->users.end();) {
                    char buffer[1];
                    int result = recv(it->socket, buffer, sizeof(buffer), MSG_PEEK | MSG_DONTWAIT);
                    if (result == 1) {
                        cout << " >> Se ha cerrado conexión con el usuario en espera con id: " << it->socket << endl;
                        close(it->socket);
                        server->mtxUsers.lock();
                        it = server->users.erase(it);
                        server->mtxUsers.unlock();
                    } else {
                        ++it;
                    }
                }
                
                sleep(2);
            }
        }

        static string getResults(vector<client>* users){
            int max = -1, count = 0;
            string result;
            
            for (auto& user : *users) {        
                if(max < user.score){
                    result = user.name;
                    max = user.score;
                    count = 0;
                }else if(max == user.score){
                    result += ", " + user.name;
                    count++;
                }
            }

            if(max > -1){
                if(count == 0){
                    return "El ganador de la partida es " + result + " con " + to_string(max) + " respuesta/s correcta/s.";
                }else{
                    if (count == (users->size()-1))
                        return "Hubo un empate entre " + result + " con " + to_string(max) + " respuesta/s correcta/s.";
                    else
                        return "Los ganadores de la partida son " + result + " con " + to_string(max) + " respuesta/s correcta/s.";
                }
            }

            return "No hubo ganadores porque todos los participantes se desconectaron.";
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