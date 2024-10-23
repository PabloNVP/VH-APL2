#include <string>
#include <cstring>
#include <vector>
#include <filesystem>
#include <fstream>
#include <signal.h>
#include <iostream>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <semaphore.h>

#define NOMBRE_MEMORIA "mem4"
#define NOMBRE_SEM_SERVIDOR "servidor4"
#define NOMBRE_SEM_CLIENTE "cliente4"
#define NOMBRE_SEM_CONEXION "conexion4"
#define LOCK_FILE "/tmp/servidor4.lock"
#define MIN_PARAMS_REQ 4
#define CANT_OPCIONES 3
#define MAX_TEXTO 128

using namespace std;
namespace fs = filesystem;

struct Pregunta {
    char pregunta[MAX_TEXTO];
    char opciones[CANT_OPCIONES][MAX_TEXTO];
    char nombre_cliente[MAX_TEXTO];
    int respuesta_correcta;
    int respuesta_cliente;
    int puntaje;
};
