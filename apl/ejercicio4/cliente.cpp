/**
###                 INTEGRANTES                     ###
###     Collazo, Ignacio Lahuel     - CONFIDENCE    ### 
###     Pozzato, Alejo Martin       - CONFIDENCE    ### 
###     Rodriguez, Emanual          - CONFIDENCE    ###
###     Rodriguez, Pablo            - CONFIDENCE    ### 
###     Vazquez Petracca, Pablo N.  - CONFIDENCE    ###
*/

#include "cliente.hpp"

int lf; // Variable global para controlar el lock file

void mostrar_ayuda_cliente() {
    cout << "###############################################################" << endl;

    cout << "Uso: ./cliente4 [opciones]" << endl << endl
         << "+ Opciones:" << endl
         << "\t-h     Muestra esta ayuda." << endl
         << "\t-n     Nickname del jugador. [Obligatorio]" << endl << endl
         << "+ Ejemplos:" << endl
         << "\t./cliente4 -n 'Messi'" << endl << endl
         << "+ Prerequisitos: " << endl << "\t- Contar con el proceso 'servidor4' corriendo." << endl;

    cout << "###############################################################" << endl;

    exit(EXIT_SUCCESS);
}

void mostrar_params(string nickname) {
    cout << "##### DATOS INGRESADOS #####" << endl
         << "+ Parametro [-n]: " << nickname << endl;
}

void validar_params(string nickname) {

    // Validacion del nickname
    if (nickname.empty()) {
        cerr << "[Cliente] - Error: El parametro '-n' (nickname) debe tener algun valor." << endl;
        exit(EXIT_FAILURE);
    }

    for (char c : nickname) {
        if (!isalpha(c)) {  // Si no es una letra
            cerr << "[Cliente] - Error: El nickname solo puede contener letras." << endl;
            exit(EXIT_FAILURE);
        }
    }
}

int crear_shm() {
    int shm_id = shm_open(NOMBRE_MEMORIA, O_RDWR, 0600);
    if (shm_id == -1) {
        cerr << "[Cliente] - Error: No se pudo crear la memoria compartida." << endl;
        exit(EXIT_FAILURE);
    }

    return shm_id;
}

void crear_sem(sem_t ** sem_servidor, sem_t ** sem_cliente, sem_t ** sem_conexion) {
    *sem_servidor = sem_open(NOMBRE_SEM_SERVIDOR, 0);
    if (*sem_servidor == SEM_FAILED) {
        cerr << "[Cliente] - Error: No se pudo abrir el semaforo del servidor." << endl;
        exit(EXIT_FAILURE);
    }

    *sem_cliente = sem_open(NOMBRE_SEM_CLIENTE, 0);
    if (*sem_cliente == SEM_FAILED) {
        cerr << "[Cliente] - Error: No se pudo abrir el semaforo del cliente." << endl;
        exit(EXIT_FAILURE);
    }

    *sem_conexion = sem_open(NOMBRE_SEM_CONEXION, 0);
    if (*sem_conexion == SEM_FAILED) {
        cerr << "[Cliente] - Error: No se pudo abrir el semaforo de conexion." << endl;
        exit(EXIT_FAILURE);
    }
}

int crear_lock_file(const char * path) {
    int fd = open(path, O_CREAT | O_RDWR, 0666);
    if (fd == -1) {
        cerr << "[Cliente] - Error: No se pudo crear el archivo de bloqueo." << endl;
        exit(EXIT_FAILURE);
    }

    if (lockf(fd, F_TLOCK, 0) == -1) {
        cerr << "[Cliente] - Error: Ya hay un cliente ejecutandose." << endl;
        close(fd);
        exit(EXIT_FAILURE);
    }

    return fd;
}

void liberar_lock_file(int fd, const char * path) {
    close(fd);
    unlink(path);
}

int main(int argc, char *argv[]) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0) { // Verificar si pasaron '-h'
            mostrar_ayuda_cliente();
        }
    }

    lf = crear_lock_file(LOCK_FILE);

    int opt;
    string nickname;
    signal(SIGINT, SIG_IGN);

    if (argc < MIN_PARAMS_REQ) {
        cout << "[Cliente] - Error: Faltan argumentos obligatorios. Intente nuevamente." << endl; 
        exit(EXIT_FAILURE);
    }

    while ((opt = getopt(argc, argv, "n:")) != -1) {
        switch (opt) {
            case 'n':
                nickname = optarg;
                break;
            default:
                mostrar_ayuda_cliente();
        }
    }

    validar_params(nickname);
    mostrar_params(nickname);

    sem_t * sem_servidor;
    sem_t * sem_cliente;
    sem_t * sem_conexion;
    crear_sem(&sem_servidor, &sem_cliente, &sem_conexion); // Conectar los semaforos

    int shm_id = crear_shm(); // Conectar la memoria compartida

    void * shm = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, shm_id, 0); // Mapear cantidad de preguntas
    if (shm == MAP_FAILED) {
        cerr << "[Cliente] - Error: No se pudo mapear la memoria compartida." << endl;
        exit(EXIT_FAILURE);
    }

    int * shm_cant_preguntas = (int *)shm;
    int cant_preguntas = *shm_cant_preguntas;

    munmap(shm, sizeof(int));

    shm = mmap(NULL, sizeof(int) + sizeof(Pregunta) * cant_preguntas, PROT_READ | PROT_WRITE, MAP_SHARED, shm_id, 0); // Mapear las preguntas
    if (shm == MAP_FAILED) {
        cerr << "[Cliente] - Error: No se pudo mapear la memoria compartida para las preguntas." << endl;
        exit(EXIT_FAILURE);
    }

    shm_cant_preguntas = (int *)shm;
    Pregunta * shm_preguntas = (Pregunta *)(shm_cant_preguntas + 1);

    strncpy(shm_preguntas[0].nombre_cliente, nickname.c_str(), MAX_TEXTO - 1);
    shm_preguntas[0].nombre_cliente[MAX_TEXTO - 1] = '\0';

    int respuesta, puntaje = 0;
    sem_post(sem_conexion);

    for (int i = 0; i < cant_preguntas; i++) {
        sem_wait(sem_cliente);

        cout << endl;

        cout << "[Cliente] - Pregunta recibida: " << shm_preguntas[i].pregunta << endl;
        cout << "Opciones: " << endl;
        for (int j = 0; j < CANT_OPCIONES; j++) {
            cout << j + 1 << ". " << shm_preguntas[i].opciones[j] << endl;
        }

        cout << "Ingrese su respuesta: ";
        cin >> respuesta;

        while (respuesta < 1 || respuesta > 3) {
            cerr << "[Cliente] - Respuesta invalida. Intente nuevamente: ";
            cin >> respuesta;
        }

        shm_preguntas[i].respuesta_cliente = respuesta;

        sem_post(sem_servidor);
    }

    for (int i = 0; i < cant_preguntas; i++) {
        sem_wait(sem_cliente);  // Esperar la notificación del servidor para leer el puntaje
        puntaje += shm_preguntas[i].puntaje;  // Sumar los puntajes de todas las preguntas
    }

    cout << endl;
    cout << "[Cliente] - +----- Puntaje final: " << puntaje << " -----+" << endl << endl;
    cout << endl << "[Cliente] - ##### Partida finalizada #####" << endl;

    sem_close(sem_servidor);
    sem_close(sem_cliente);
    sem_close(sem_conexion);
    munmap(shm, sizeof(int) + sizeof(Pregunta) * cant_preguntas);

    liberar_lock_file(lf, LOCK_FILE);

    return 0;
}
