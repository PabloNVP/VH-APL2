/**
###                 INTEGRANTES                     ###
###     Collazo, Ignacio Lahuel     - CONFIDENCE    ### 
###     Pozzato, Alejo Martin       - CONFIDENCE    ### 
###     Rodriguez, Emanuel          - CONFIDENCE    ###
###     Rodriguez, Pablo            - CONFIDENCE    ### 
###     Vazquez Petracca, Pablo N.  - CONFIDENCE    ###
*/

#include "servidor.hpp"

bool partida_en_progreso = false; // Variable global para controlar señales
int lf; // Variable global para controlar el lock file

void mostrar_ayuda_servidor() {
    cout << "###############################################################" << endl;

    cout << "Uso: ./servidor4 [opciones]" << endl << endl
         << "+ Opciones:" << endl
         << "\t-h     Muestra esta ayuda." << endl
         << "\t-a     Archivo que contiene las preguntas a leer. [Obligatorio]" << endl
         << "\t-c     Cantidad de preguntas a realizar. [Obligatorio]" << endl << endl
         << "+ Ejemplos:" << endl
         << "\t./servidor4 -a './archivos/preguntas.csv' -c 3" << endl
         << "\t./servidor4 -c 5 -a './archivos/preguntas.csv'" << endl
         << "+ Prerequisitos: " << endl << "\t- Contar con las bibliotecas necesarias instaladas." << endl;


    cout << "###############################################################" << endl;

    exit(EXIT_SUCCESS);
}

void mostrar_params(fs::path archivo, int cant_preguntas) {
    cout << "##### DATOS INGRESADOS #####" << endl
         << "+ Parametro [-a]: " << archivo.filename() << endl
         << "+ Parametro [-c]: " << cant_preguntas << endl; 
}

void validar_params(fs::path archivo, int cant_preguntas) {

    // Validacion del archivo
    struct stat buffer;
    if (archivo.empty()) {
        cerr << "[Servidor] - Error: El parametro '-a' (archivo) es requerido.\n";
        exit(EXIT_FAILURE);
    } else if (stat(archivo.c_str(), & buffer) != 0) {
        cerr << "[Servidor] - Error: El archivo '" << archivo << "' no existe o el path es invalido." << endl;
        exit(EXIT_FAILURE);
    } else if (access(archivo.c_str(), R_OK) != 0) {
        cerr << "[Servidor] - Error: No hay permisos de lectura sobre el archivo '" << archivo << "'." << endl;
        exit(EXIT_FAILURE);
    }


    // Validacion de las preguntas
    if (cant_preguntas <= 0) {
        cerr << "[Servidor] - Error: El parametro '-c' (cantidad) debe ser mayor a 0." << endl;
        exit(EXIT_FAILURE);
    }
}

int contar_lineas(const fs::path & path) {
    ifstream file(path);

    if (!file.is_open()) {
        cerr << "[Servidor] - Error: No se pudo abrir el archivo " << path << endl;
        exit(EXIT_FAILURE);
    }

    string linea;
    int cantidad_lineas = 0;

    while (getline(file, linea)) {
        cantidad_lineas++;
    }

    file.close();
    return cantidad_lineas;
}

void handler_senial(int sigId) {
    if (sigId == SIGUSR1) {
        if (!partida_en_progreso) {
            cout << "[Servidor] - Recibida senial SIGUSR1. Finalizando el servidor..." << endl;
            sem_unlink(NOMBRE_SEM_SERVIDOR);
            sem_unlink(NOMBRE_SEM_CLIENTE);
            sem_unlink(NOMBRE_SEM_CONEXION);
            shm_unlink(NOMBRE_MEMORIA);
            close(lf);
            unlink(LOCK_FILE);
            exit(EXIT_SUCCESS);
        } else {
            cout << "[Servidor] - Senial SIGUSR1 recibida, pero hay una partida en progreso. Ignorando senial." << endl << endl;
        }
    }
}

void leer_preguntas(const fs::path & archivo, Pregunta * shm, int cant_preguntas) {
    ifstream file(archivo);
    if (!file.is_open()) {
        cerr << "[Servidor] - Error al abrir el archivo de preguntas." << endl;
        exit(EXIT_FAILURE);
    }

    string linea, pregunta, op1, op2, op3, correcta;
    int num_linea = 1;
    while (getline(file, linea)) {
        stringstream ss(linea);

        getline(ss, pregunta, ',');
        getline(ss, correcta, ',');
        getline(ss, op1, ',');
        getline(ss, op2, ',');
        getline(ss, op3, ',');

        strncpy(shm[num_linea - 1].pregunta, pregunta.c_str(), MAX_TEXTO);
        strncpy(shm[num_linea - 1].opciones[0], op1.c_str(), MAX_TEXTO);
        strncpy(shm[num_linea - 1].opciones[1], op2.c_str(), MAX_TEXTO);
        strncpy(shm[num_linea - 1].opciones[2], op3.c_str(), MAX_TEXTO);
        shm[num_linea - 1].respuesta_correcta = stoi(correcta);
        shm[num_linea - 1].respuesta_cliente = -1;

        num_linea++;
        if (num_linea > cant_preguntas) {
            break; // Limitar a la cantidad de preguntas de la partida a la cantidad deseada
        }  
    }

    file.close();
}

int crear_shm(int cant_preguntas) {
    int shm_id = shm_open(NOMBRE_MEMORIA, O_CREAT | O_RDWR, 0600);
    if (shm_id == -1) {
        cerr << "[Servidor] - Error: No se pudo crear la memoria compartida." << endl;
        exit(EXIT_FAILURE);
    }

    if (ftruncate(shm_id, sizeof(int) + sizeof(Pregunta) * cant_preguntas) == -1) {
        cerr << "[Servidor] - Error: No se pudo ajustar el tamanio de la memoria compartida." << endl;
        exit(EXIT_FAILURE);
    }

    return shm_id;
}

void crear_sem(sem_t** sem_servidor, sem_t** sem_cliente, sem_t** sem_conexion) {
    *sem_servidor = sem_open(NOMBRE_SEM_SERVIDOR, O_CREAT, 0600, 0);
    if (*sem_servidor == SEM_FAILED) {
        cerr << "[Servidor] - Error: No se pudo crear el semaforo del servidor." << endl;
        exit(EXIT_FAILURE);
    }

    *sem_cliente = sem_open(NOMBRE_SEM_CLIENTE, O_CREAT, 0600, 0);
    if (*sem_cliente == SEM_FAILED) {
        cerr << "[Servidor] - Error: No se pudo crear el semaforo del cliente." << endl;
        exit(EXIT_FAILURE);
    }

    *sem_conexion = sem_open(NOMBRE_SEM_CONEXION, O_CREAT, 0600, 0);
    if (*sem_conexion == SEM_FAILED) {
        cerr << "[Servidor] - Error: No se pudo crear el semaforo de conexion." << endl;
        exit(EXIT_FAILURE);
    }
}

int crear_lock_file(const char * path) {
    int fd = open(path, O_CREAT | O_RDWR, 0666);
    if (fd == -1) {
        cerr << "[Servidor] - Error: No se pudo crear el archivo de bloqueo." << endl;
        exit(EXIT_FAILURE);
    }

    if (lockf(fd, F_TLOCK, 0) == -1) {
        cerr << "[Servidor] - Error: Ya hay un servidor ejecutandose." << endl;
        close(fd);
        exit(EXIT_FAILURE);
    }

    return fd;
}

void liberar_lock_file(int fd, const char * path) {
    close(fd);
    unlink(path);
}

int main(int argc, char * argv[]) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0) { // Verificar si pasaron '-h'
            mostrar_ayuda_servidor();
        }
    }

    lf = crear_lock_file(LOCK_FILE);

    fs::path archivo;
    int cant_preguntas, opt;
    struct sigaction action;
    action.sa_handler = handler_senial;

    signal(SIGINT, SIG_IGN);
    sigaction(SIGUSR1, &action, NULL);

    if (argc < MIN_PARAMS_REQ) {
        cout << "[Servidor] - Error: Faltan argumentos obligatorios. Intente nuevamente." << endl; 
        exit(EXIT_FAILURE);
    }

    while ((opt = getopt(argc, argv, "a:c:")) != -1) {
        switch (opt) {
            case 'a':
                archivo = optarg;
                break;
            case 'c':
                try {
                    cant_preguntas = stoi(optarg);
                } catch (const invalid_argument & e) {
                    cerr << "Error - El parametro '-c' debe ser un numero valido.\n";
                    exit(EXIT_FAILURE);
                }
                break;
            default:
                mostrar_ayuda_servidor();
        }
    }

    validar_params(archivo, cant_preguntas);
    mostrar_params(archivo, cant_preguntas);

    cout << endl;

    int cant_preguntas_archivo = contar_lineas(archivo);
    if (cant_preguntas > cant_preguntas_archivo) {
        cerr << "[Servidor] - Error: El archivo especificado no contiene esta cantidad de preguntas." << endl;
        exit(EXIT_FAILURE);
    }

    int shm_id = crear_shm(cant_preguntas);  // Crear memoria compartida
    sem_t * sem_servidor;
    sem_t * sem_cliente;
    sem_t * sem_conexion;

    crear_sem(&sem_servidor, &sem_cliente, &sem_conexion);  // Crear semaforos

    void * shm = mmap(NULL, sizeof(int) + sizeof(Pregunta) * cant_preguntas, PROT_READ | PROT_WRITE, MAP_SHARED, shm_id, 0); // Mapear memoria compartida
    if (shm == MAP_FAILED) {
        cerr << "[Servidor] - Error: No se pudo mapear la memoria compartida." << endl;
        exit(EXIT_FAILURE);
    }

    int * shm_cant_preguntas = (int *)shm;  // Puntero al entero para almacenar la cantidad de preguntas
    Pregunta * shm_preguntas = (Pregunta *)(shm_cant_preguntas + 1);  // Puntero a las preguntas

    *shm_cant_preguntas = cant_preguntas;

    cout << endl;

    leer_preguntas(archivo, shm_preguntas, cant_preguntas); // Cargar preguntas en la memoria compartida
    int respuesta_cliente = -1, puntaje;
    while (true) {
        cout << "[Servidor] - ##### Esperando cliente... #####" << endl << endl;
        sem_wait(sem_conexion);

        cout << "[Servidor] - ##### ¡Bienvenido " << shm_preguntas[0].nombre_cliente << "! - Comenzando partida... #####" << endl;
        partida_en_progreso = true;
        puntaje = 0;

        for (int i = 0; i < cant_preguntas; i++) {
            cout << "[Servidor] - Pregunta [" << (i + 1) << "] enviada correctamente." << endl;

            cout << endl;

            sem_post(sem_cliente);
            sem_wait(sem_servidor);

            if (shm_preguntas[i].respuesta_cliente == shm_preguntas[i].respuesta_correcta) {
                cout << "[Servidor] - ¡Muy bien! Respuesta correcta :D" << endl;
                shm_preguntas[i].puntaje = 1;  // Incrementar puntaje en la memoria compartida
                puntaje += shm_preguntas[i].puntaje;
            } else {
                cout << "[Servidor] - Ni cerca. Respuesta incorrecta :P" << endl;
                shm_preguntas[i].puntaje = 0;
            }

            cout << endl;

            sem_post(sem_cliente);
        }

        partida_en_progreso = false;
        cout << "[Servidor] - +----- Puntaje final: " << puntaje << " -----+" << endl << endl;
        cout << "[Servidor] - ##### Partida finalizada. Esperando nuevas partidas... #####" << endl;
    }

    sem_close(sem_servidor);
    sem_close(sem_cliente);
    sem_close(sem_conexion);
    munmap(shm, sizeof(int) + sizeof(Pregunta) * cant_preguntas);

    liberar_lock_file(lf, LOCK_FILE);

    return EXIT_SUCCESS;
}
