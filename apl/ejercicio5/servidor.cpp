
#include "servidor.hpp"

/**############ INTEGRANTES ###############
###     Collazo, Ignacio Lahuel         ### 
###     Pozzato, Alejo Martin           ### 
###     Rodriguez, Emanual              ###
###     Rodriguez, Pablo                ### 
###     Vazquez Petracca, Pablo N.      ### 
#########################################*/

int main(const int argc, const char* argv[]) {

    Servidor server;

    signal(SIGINT, SIG_IGN);

    if(server.validateParameters(argc, argv))
        return EXIT_FAILURE;

    if(server.init())
        return EXIT_FAILURE;

    cout << "El servidor se ha iniciado correctamente." << endl;

    if(server.run())
        return EXIT_FAILURE;
    
    cout << "El servidor se ha cerrado correctamente." << endl;
     
    return 0; 
}



