/**
###                 INTEGRANTES                     ###
###     Collazo, Ignacio Lahuel     - CONFIDENCE    ### 
###     Pozzato, Alejo Martin       - CONFIDENCE    ### 
###     Rodriguez, Emanuel          - CONFIDENCE    ###
###     Rodriguez, Pablo            - CONFIDENCE    ### 
###     Vazquez Petracca, Pablo N.  - CONFIDENCE    ###
*/

#include <iostream>
#include <cstring>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <semaphore.h>
#include <signal.h>
#include <cstdlib>

#define NOMBRE_MEMORIA "mem4"
#define NOMBRE_SEM_SERVIDOR "servidor4"
#define NOMBRE_SEM_CLIENTE "cliente4"
#define NOMBRE_SEM_CONEXION "conexion4"
#define LOCK_FILE "/tmp/cliente4.lock"
#define MIN_PARAMS_REQ 2
#define CANT_OPCIONES 3
#define MAX_TEXTO 128

using namespace std;

struct Pregunta {
    char pregunta[MAX_TEXTO];
    char opciones[CANT_OPCIONES][MAX_TEXTO];
    char nombre_cliente[MAX_TEXTO];
    int respuesta_correcta;
    int respuesta_cliente;
    int puntaje;
};
