
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
#include <getopt.h> // Para getopt_long
#include <cstdlib>  // Para atoi y exit
#include <cstring>  // Para strcpy
#include <cerrno>   // Para strerror y errno
#include <csignal>  // Para manejar señales
#include <cstdlib>  // Para exit()

#define MAX_STRING 256
#define OK 0
#define ERROR 1
using namespace std;
// VARIABLES GLOBALES
const char *fifo_path = "/tmp/mi_fifo";
FILE *id_f = nullptr;
int fd;

void mostrar_ayuda();
void signal_handler(int signo); /// la defino e implemento aca para usar la variable global
int main(int argc, char *argv[])
{
    // Variables para almacenar los argumentos
    int cantidad_mensajes = 0;
    int intervalo_segundos = 0;
    int numero_sensor = 0;
    long long id_huella = 0;
    char id_file[MAX_STRING]="";  // Definir un string para almacenar el nombre del archivo
    char mensaje[MAX_STRING]="";  // Aumentado para evitar posibles desbordamientos
    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);

    // Estructura para getopt_long
    static struct option long_options[] =
    {
        {"mensajes", required_argument, 0, 'm'},
        {"ids", required_argument, 0, 'i'},
        {"segundos", required_argument, 0, 's'},
        {"numero", required_argument, 0, 'n'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0} // Indicador de finalización
    };

    int opt;
    int option_index = 0;

    // Procesar los argumentos
    while ((opt = getopt_long(argc, argv, "n:s:m:i:h", long_options, &option_index)) != -1)
    {
        switch (opt)
        {
        case 'n':
            numero_sensor = atoi(optarg);  // Convertir a entero
            break;
        case 'i':
            strcpy(id_file, optarg);  // Guardar el archivo de IDs
            break;
        case 'h':
            mostrar_ayuda();  // Mostrar la ayuda
            return OK;
        case 'm':
            cantidad_mensajes = atoi(optarg);  // Convertir a entero
            break;
        case 's':
            intervalo_segundos = atoi(optarg);  // Convertir a entero
            break;
        default:
            mostrar_ayuda();  // Mostrar ayuda si hay un argumento incorrecto
            return ERROR;
        }
    }

    // Validación de argumentos
    if (numero_sensor == 0 || cantidad_mensajes <=0 || strlen(id_file) == 0 ||  intervalo_segundos<0)
    {
        cerr << "Error en los argumentos ingresados.\n";
        mostrar_ayuda();
        return ERROR;
    }

    // Abrir el archivo de IDs para lectura
    id_f = fopen(id_file, "rt");
    if (!id_f)
    {
        cerr << "Error: No se pudo abrir el archivo de IDs (LECTURA SIMULADA).\n";
        return ERROR;
    }

    // Abrir el FIFO para escritura
    fd = open(fifo_path, O_WRONLY);
    if (fd == -1)
    {
        cerr << "Error abriendo el FIFO para escritura: " << strerror(errno) << endl;
        fclose(id_f);
        return ERROR;
    }

    // Enviar mensajes al FIFO
    while (cantidad_mensajes > 0)
    {
        if (fscanf(id_f, "%lld", &id_huella) != 1)  // Leer IDs del archivo
        {
            cerr << "Error leyendo un ID del archivo o no hay más IDs." << endl;
            break;  // Salir si no se pueden leer más IDs
        }

        sprintf(mensaje, "Numero_Sensor: %d ID_huella: %lld\n", numero_sensor, id_huella); // le doy el formato que quiero
        if (write(fd, mensaje, strlen(mensaje)) == -1)  // Enviar el mensaje al FIFO
        {
            cerr << "Error escribiendo en el FIFO: " << strerror(errno) << endl;
            break;
        }

        // Mostrar un mensaje para seguimiento
      //  cout << "Mensaje enviado: " << mensaje << endl;

        sleep(intervalo_segundos);  // INTERVALO ENTRE MENSAJES
        cantidad_mensajes--;
    }

    fclose(id_f);
    close(fd);
   // cout << "Finalizando el proceso del sensor N° " << numero_sensor << endl;
    return OK;
}

void signal_handler(int signo)
{
    if (signo == SIGTERM)
    {
        cout << "Proceso finalizando... Cerrando archivos y FIFO" << endl;

        // Cerrar el archivo de log si está abierto
        if (id_f != nullptr)
        {
            fclose(id_f);
            cout << "Archivo de log cerrado." << endl;
        }

        // Cerrar el FIFO si está abierto
        if (fd != -1)
        {
            close(fd);
            cout << "FIFO cerrado." << endl;
        }

        exit(0);
    }

}
void mostrar_ayuda()
{
    cout << "Uso: proceso_sensor [opciones] &\n";
    cout << "Opciones:\n";
    cout << "  -n, --numero    Número del sensor (Requerido)\n";
    cout << "  -s, --segundos  Intervalo en segundos (Requerido)\n";
    cout << "  -m, --mensajes  Cantidad de mensajes a enviar (Requerido)\n";
    cout << "  -i, --ids       Archivo con números de ID (Requerido)\n";
    cout << "  -h, --help      Mostrar ayuda\n";
}
