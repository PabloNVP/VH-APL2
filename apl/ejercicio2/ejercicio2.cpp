#include "ejercicio2.hpp"

/**############ INTEGRANTES ###############
###     Collazo, Ignacio Lahuel         ### 
###     Pozzato, Alejo Martin           ### 
###     Rodriguez, Emanual              ###
###     Rodriguez, Pablo                ### 
###     Vazquez Petracca, Pablo N.      ### 
#########################################*/

int main(int argc, char *argv[]) {
    if (argc < 5) {
        mostrarAyuda();
        return 1;
    }

    string arg = argv[1];
    if(arg == "-h" || arg == "--help") {
        mostrarAyuda();
        return 1;
    }

    string dirPath;
    int numThreads = 0;
    string cadenaABuscar;

    for (int i = 1; i < argc; i++) {
        arg = argv[i];
        if (arg == "-d" || arg == "--directorio")
            dirPath = argv[++i];
        else
        {
            if (arg == "-t" || arg == "--threads")
            {
                numThreads = stoi(argv[++i]);
                if (numThreads <= 0)
                {
                    cerr << "Error: El n�mero de hilos debe ser un entero positivo.\n";
                    return 1;
                }
            }
            else
                cadenaABuscar = arg;
        }
    }

    if (dirPath.empty())
    {
        cerr << "Error: El directorio es un parametro obligatorio.\n";
        return 1;
    }

    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    for (const auto &entry : filesystem::directory_iterator(dirPath)) {
        if (entry.is_regular_file()) {
            fileQueue.push(entry.path().string());
        }
    }

    vector<thread> threads;
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back(trabajo, i + 1, cadenaABuscar);
    }

    {
        lock_guard<mutex> lock(mtx);
        done = true;
    }
    cv.notify_all();

    for (auto &thread : threads) {
        thread.join();
    }

    return 0;
}
