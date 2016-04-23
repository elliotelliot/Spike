// Define this to turn on error checking
#define PRINT_MESSAGES

#include <stdio.h>
#include <iostream>
#include <stdlib.h>


inline void begin_simulation_message(float timestep, int number_of_stimuli, int number_of_epochs, bool save_spikes, bool present_stimuli_in_random_order, int total_number_of_neurons, int total_number_of_synapses)
{
	#ifndef QUIETSTART

		printf("\n\n----------------------------------\n");

		printf("Simulation Beginning\n");

		printf("Time Step: %f\nNumber of Stimuli: %d\nNumber of Epochs: %d\n\n", timestep, number_of_stimuli, number_of_epochs);

		printf("Total Number of Neurons: %d\n", total_number_of_neurons);

		printf("Total Number of Synapses: %d\n\n", total_number_of_synapses);

		if (present_stimuli_in_random_order) printf("Stimuli to be presented in a random order.\n");
		
		if (save_spikes) printf("Spikes shall be saved.\n");
		
		printf("----------------------------------\n\nBeginning ...\n\n");

	#endif
}


inline void print_message_and_exit(const char * message)
{
    
    // #ifdef PRINT_MESSAGES
        // printf("JI PRINT MESSAGES\n");
    // #endif

	printf("%s\n", message);
    printf("Exiting...\n");
    exit(-1);

    return;
}
