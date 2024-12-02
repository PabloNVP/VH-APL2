
/**############ INTEGRANTES ###############
###     Collazo, Ignacio Lahuel         ### 
###     Pozzato, Alejo Martin           ### 
###     Rodriguez, Emanual              ###
###     Rodriguez, Pablo                ### 
###     Vazquez Petracca, Pablo N.      ### 
#########################################*/

#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <signal.h>
#include <cstdlib>
#include <cstring>
#include <time.h>
#include <sstream>
#include <string>
#include <fstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <getopt.h>
#include <set>
#include <poll.h>  // Para usar poll()

#define MAX_STRING 256
#define BUFFER_TAM 1024
#define OK 0
#define ERROR 1
using namespace std;

// variables globales
FILE *log_f = nullptr;
int fd = -1;
const char* fifo_path = "/tmp/mi_fifo";

// declaracion de funciones utilitarias
string obtenerFechayHora();
void mostrar_ayuda();
bool validarHuella(long long idHuella, const set<long long> &huellasRegistradas);
void cargarHuellas(FILE *ids_file, set<long long> &huellasRegistradas);
void signal_handler(int signo);
void crearDemonio();

int main(int argc, char *argv[])
{
    crearDemonio();

    // Registrar las señales
    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);
    signal(SIGHUP, signal_handler);

    set<long long> huellasRegistradas;
    char log_file[MAX_STRING] = "";  // Inicializar como cadena vacía
    char id_file[MAX_STRING] = "";    // Inicializar como cadena vacía
    char buffer[BUFFER_TAM];
    FILE *id_f;
    int numeroSensor;
    long long idHuella;

    // Definir opciones para getopt_long
    static struct option long_options[] =
    {
        {"ids", required_argument, 0, 'i'},
        {"log", required_argument, 0, 'l'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    int opt;
    int option_index = 0;

    // Procesar los argumentos
    while ((opt = getopt_long(argc, argv, "i:l:h", long_options, &option_index)) != -1)
    {
        switch (opt)
        {
        case 'l':
            strcpy(log_file, optarg);
            break;
        case 'i':
            strcpy(id_file, optarg);
            break;
        case 'h':
            mostrar_ayuda();
            return OK;
        default:
            mostrar_ayuda();
            return ERROR; // error porque no ingresamos ningun parametro
        }
    }

    // Verificar los argumentos
    if (strlen(log_file) == 0 || strlen(id_file) == 0)
    {
        cerr << "Error: Faltan argumentos requeridos.\n";
        mostrar_ayuda();
        return ERROR;
    }

    // Mensajes para verificar parámetros
//    cout << "Archivo de log: " << log_file << endl;
  //  cout << "Archivo de IDs: " << id_file << endl;

    // Abrir los archivos de log e IDs
    log_f = fopen(log_file, "wt");
    if (!log_f)
    {
        cerr << "Error al abrir el archivo de log." << endl;
        return ERROR;
    }
    id_f = fopen(id_file, "rt");
    if (!id_f)
    {
        cerr << "Error al abrir el archivo de IDs." << endl;
        fclose(log_f);
        return ERROR;
    }

    // Crear y abrir el FIFO
    if (mkfifo(fifo_path, 0666) == -1 && errno != EEXIST)
    {
        cerr << "Error creando el FIFO" << endl;
        fclose(log_f);
        return ERROR;
    }

    fd = open(fifo_path, O_RDONLY | O_NONBLOCK); // Usar O_NONBLOCK para evitar bloqueo en open
    if (fd == -1)
    {
        cerr << "Error abriendo el FIFO para lectura" << endl;
        fclose(log_f);
        return ERROR;
    }

    cargarHuellas(id_f, huellasRegistradas); // Para no recorrer el archivo cada vez que valido una huella
    fclose(id_f); // una vez cargo, ya no necesito el archivo de ids abierto

    // Configurar la estructura poll para monitorear el FIFO
    struct pollfd fds[1];
    fds[0].fd = fd;
    fds[0].events = POLLIN;  // Monitorear cuando hay datos para leer
    int ret;

    // Bucle principal del demonio
    while (true)
    {
        ret = poll(fds, 1, 10000);  // Esperar hasta 10 segundos

        if (ret == -1)
        {
            cerr << "Error en poll()" << endl;
            break;
        }
        else if (ret == 0)
        {
            // Tiempo de espera alcanzado
    //        cout << "Esperando datos en el FIFO..." << endl;
            continue;  // Volver a comprobar
        }

        if (fds[0].revents & POLLIN)
        {
            memset(buffer, 0, sizeof(buffer));
            ssize_t bytes_read = read(fd, buffer, sizeof(buffer));

            if (bytes_read == -1)
            {
                perror("Error leyendo el FIFO");
                break;
            }
            else if (bytes_read == 0)
            {
                cerr << "FIFO cerrado, saliendo..." << endl;
                break;
            }


            // Formato esperado: "Numero_Sensor: <value> ID_huella: <value>"
            sscanf(buffer, "Numero_Sensor: %d ID_huella: %lld", &numeroSensor, &idHuella);
                // Imprimir los datos leídos para tener un control de lo que se va leyendo
               // printf("Numero Sensor: %d, ID Huella: %lld\n", numeroSensor, idHuella);

            if (validarHuella(idHuella, huellasRegistradas)) // valido la huella, con las que cargue en el Set
            {
                fprintf(log_f, "Fecha: %s Nro_Sensor: %d ID_huella: %lld Identificada: reconocible\n",
                        obtenerFechayHora().c_str(), numeroSensor, idHuella);
               // cout << "Huella identificada como reconocible." << endl; // informacion de control
            }
            else
            {
                fprintf(log_f, "Fecha: %s Nro_Sensor: %d ID_huella: %lld Identificada: No reconocible\n",
                        obtenerFechayHora().c_str(), numeroSensor, idHuella);
               // cout << "Huella identificada como no reconocible." << endl; // informacion de control
            }

            fflush(log_f);  // Asegurarse de que los datos se escriban en el archivo
        }
    }

    // Cerrar el log y el FIFO antes de salir
    fclose(log_f);
    close(fd);
    unlink(fifo_path);

    return OK;
}

string obtenerFechayHora()
{
    time_t tiempoActual = time(nullptr);
    char buffer[80];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", localtime(&tiempoActual));
    return string(buffer);
}

void mostrar_ayuda()
{
    cout << "Uso: proceso_central [opciones]\n";
    cout << "Opciones:\n";
    cout << "  -l, --log  Direccion del archivo de log a escribir (Requerido)\n";
    cout << "  -i, --ids  Archivo con números de ID (Requerido)\n";
    cout << "  -h, --help Mostrar ayuda\n";
}

bool validarHuella(long long idHuella, const set<long long> &huellasRegistradas)
{
    return huellasRegistradas.find(idHuella) != huellasRegistradas.end(); // busco la huella en el set
}

void cargarHuellas(FILE *ids_file, set<long long> &huellasRegistradas)
{
    rewind(ids_file);  // Reiniciar el puntero del archivo
    long long huella;
    while (fscanf(ids_file, "%lld\n", &huella) != EOF)
    {
        huellasRegistradas.insert(huella); // agrego las huellas del archivo al set para buscar mas eficientemente
    }
}

void signal_handler(int signo)
{
    if (signo == SIGTERM || signo == SIGHUP || signo == SIGINT)
    {
        cerr << "Finalizando el demonio de manera segura..." << endl;
        // Cerrar el log y el FIFO de manera segura
        fclose(log_f);
        close(fd);
        unlink(fifo_path);
        exit(0);
    }
}

void crearDemonio()
{
    pid_t pid = fork();

    if (pid < 0)
    {
        cerr << "Error al crear el demonio" << endl;
        exit(1);
    }

    if (pid > 0)
    {
        // Terminar el proceso padre
        exit(0);
    }

    // Crear una nueva sesión para que el demonio no tenga terminal asociado
    if (setsid() < 0)
    {
        cerr << "Error al crear la sesión" << endl;
        exit(1);
    }

    // Ignorar señales específicas
    signal(SIGCHLD, SIG_IGN);
    signal(SIGINT, SIG_IGN);

}
