#ifndef EJERCICIO2_HPP_INCLUDED
#define EJERCICIO2_HPP_INCLUDED


/**############ INTEGRANTES ###############
###     Collazo, Ignacio Lahuel         ### 
###     Pozzato, Alejo Martin           ### 
###     Rodriguez, Emanual              ###
###     Rodriguez, Pablo                ### 
###     Vazquez Petracca, Pablo N.      ### 
#########################################*/

#include <iostream>
#include <thread>
#include <vector>
#include <string>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <csignal>

using namespace std;

std::mutex mtx;
std::condition_variable cv;
std::queue<std::string> fileQueue;
bool done = false;

void buscarEnArchivo(const string &nombreArchivo, const string &cadenaABuscar, int threadId) {
    ifstream file(nombreArchivo);
    if (!file.is_open()) {
        cerr << "Error: No se pudo abrir el archivo " << nombreArchivo << "\n";
        return;
    }

    string line;
    int lineNumber = 0;
    while (getline(file, line)) {
        lineNumber++;
        if (line.find(cadenaABuscar) != string::npos) {
            lock_guard<mutex> lock(mtx);
            cout << "Nro de Thread: " << threadId << " - El nombre del archivo: "
                      << nombreArchivo << " - El numero de l�nea: " << lineNumber << endl;
        }
    }
}

void trabajo(int threadId, const string &cadenaABuscar) {
    while (true) {
        string nombreArchivo;

        {
            unique_lock<mutex> lock(mtx);
            cv.wait(lock, [] { return !fileQueue.empty() || done; });

            if (done && fileQueue.empty()) break;

            nombreArchivo = fileQueue.front();
            fileQueue.pop();
        }

        buscarEnArchivo(nombreArchivo, cadenaABuscar, threadId);
    }
}

void signalHandler(int signal) {
    done = true;
    cv.notify_all();
}

void mostrarAyuda() {
    cout << "Uso: ./program -d <path> -t <nro> <cadena_a_buscar> [-h]\n"
              << "Opciones:\n"
              << "  -d / --directorio <path>   Ruta del directorio a analizar (Requerido)\n"
              << "  -t / --threads <nro>       Cantidad de threads a ejecutar concurrentemente (Requerido)\n"
              << "  -h / --help                 Muestra esta ayuda\n";
}

#endif // EJERCICIO2_HPP_INCLUDED
