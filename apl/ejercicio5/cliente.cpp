#include "cliente.hpp"

/**
###                 INTEGRANTES                     ###
###     Collazo, Ignacio Lahuel     - CONFIDENCE    ### 
###     Pozzato, Alejo Martin       - CONFIDENCE    ### 
###     Rodriguez, Emanual          - CONFIDENCE    ###
###     Rodriguez, Pablo            - CONFIDENCE    ### 
###     Vazquez Petracca, Pablo N.  - CONFIDENCE    ###
*/

int main(const int argc, const char* argv[]) {

    Cliente client; 

    signal(SIGINT, SIG_IGN);
    signal(SIGUSR1, client.signal_handler);

    if(client.validateParameters(argc, argv))
        return EXIT_FAILURE;

    if(client.init())
        return EXIT_FAILURE;
    
    cout << "==============================" << endl;
    cout << "¡¡¡ Bienvenido a Pregunta2 !!!" << endl;
    cout << "==============================" << endl;

    if(client.run())
        return EXIT_FAILURE;
    
    cout << "El cliente se ha cerrado correctamente." << endl;
     
    return EXIT_SUCCESS; 
}