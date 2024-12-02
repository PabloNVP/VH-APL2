#include "cliente.hpp"

/**############ INTEGRANTES ###############
###     Collazo, Ignacio Lahuel         ### 
###     Pozzato, Alejo Martin           ### 
###     Rodriguez, Emanual              ###
###     Rodriguez, Pablo                ### 
###     Vazquez Petracca, Pablo N.      ### 
#########################################*/

int main(const int argc, const char* argv[]) {

    Cliente client; 


    signal(SIGINT, SIG_IGN);
    signal(SIGUSR1, client.signal_handler);

    if(client.validateParameters(argc, argv))
        return EXIT_FAILURE;

    if(client.init())
        return EXIT_FAILURE;
    
    client.printWelcome();

    if(client.run())
        return EXIT_FAILURE;
    
    client.printClose();
     
    return EXIT_SUCCESS; 
}