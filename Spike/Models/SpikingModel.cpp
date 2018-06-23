#include "SpikingModel.hpp"

#include "../Neurons/InputSpikingNeurons.hpp"
#include "Spike/Helpers/TerminalHelpers.hpp"
#include "Spike/Backend/Context.hpp"


// SpikingModel Constructor
SpikingModel::SpikingModel () {
  timestep = 0.0001f;               // Default 0.1ms timestep
  spiking_synapses = nullptr;
  spiking_neurons = nullptr;
  input_spiking_neurons = nullptr;
}


// SpikingModel Destructor
SpikingModel::~SpikingModel () {
}


void SpikingModel::SetTimestep(float timestep_parameter){
  if ((spiking_synapses == nullptr) || (spiking_synapses->total_number_of_synapses == 0)) {
    timestep = timestep_parameter;
  } else {
    print_message_and_exit("You must set the timestep before creating any synapses.");
  }
}


int SpikingModel::AddNeuronGroup(neuron_parameters_struct * group_params) {
  if (spiking_neurons == nullptr) print_message_and_exit("Please set neurons pointer before adding neuron groups.");

  int neuron_group_id = spiking_neurons->AddGroup(group_params);
  return neuron_group_id;
}


int SpikingModel::AddInputNeuronGroup(neuron_parameters_struct * group_params) {
  if (input_spiking_neurons == nullptr) print_message_and_exit("Please set input_neurons pointer before adding inputs groups.");

  int input_group_id = input_spiking_neurons->AddGroup(group_params);
  return input_group_id;
}


void SpikingModel::AddSynapseGroup(int presynaptic_group_id, 
              int postsynaptic_group_id, 
              synapse_parameters_struct * synapse_params) {
  if (spiking_synapses == nullptr) print_message_and_exit("Please set synapse pointer before adding synapses.");

  spiking_synapses->AddGroup(presynaptic_group_id, 
              postsynaptic_group_id, 
              spiking_neurons,
              input_spiking_neurons,
              timestep,
              synapse_params);
}


void SpikingModel::AddSynapseGroupsForNeuronGroupAndEachInputGroup(int postsynaptic_group_id, 
              synapse_parameters_struct * synapse_params) {

  for (int i = 0; i < input_spiking_neurons->total_number_of_groups; i++) {

    AddSynapseGroup(CORRECTED_PRESYNAPTIC_ID(i, true), 
              postsynaptic_group_id,
              synapse_params);

  }

}

void SpikingModel::AddPlasticityRule(STDPPlasticity * plasticity_rule){
  // Adds the new STDP rule to the vector of STDP Rule
  plasticity_rule_vec.push_back(plasticity_rule);
}

void SpikingModel::AddActivityMonitor(ActivityMonitor * activityMonitor){
  // Adds the activity monitor to the vector
  monitors_vec.push_back(activityMonitor);
}

void SpikingModel::finalise_model() {
  if (!model_complete){
    model_complete = true;

    timestep_grouping = spiking_synapses->minimum_axonal_delay_in_timesteps;
    
    // Outputting Network Overview
    printf("\nBuilding Model with:\n");
    if (input_spiking_neurons)
      printf("  %d Input Neuron(s)\n", input_spiking_neurons->total_number_of_neurons);
    if (spiking_neurons)
      printf("  %d Neuron(s)\n", spiking_neurons->total_number_of_neurons);
    if (spiking_synapses)
      printf("  %d Synapse(s)\n", spiking_synapses->total_number_of_synapses);
    if (plasticity_rule_vec.size() > 0)
      printf("  %d Plasticity Rule(s)\n", (int)plasticity_rule_vec.size());
    if (monitors_vec.size() > 0)
      printf("  %d Activity Monitor(s)\n", (int)monitors_vec.size());
    printf("\n");

    // If any component does not exist, create at least a stand-in
    if (!input_spiking_neurons)
      input_spiking_neurons = new InputSpikingNeurons();
    if (!spiking_neurons)
      spiking_neurons = new SpikingNeurons();
    if (!spiking_synapses)
      spiking_synapses = new SpikingSynapses();
    

    spiking_synapses->model = this;
    spiking_neurons->model = this;
    input_spiking_neurons->model = this;
    for (int plasticity_id = 0; plasticity_id < plasticity_rule_vec.size(); plasticity_id++){
      plasticity_rule_vec[plasticity_id]->model = this;
    }
    for (int monitor_id = 0; monitor_id < monitors_vec.size(); monitor_id++){
      monitors_vec[monitor_id]->model = this;
    }
    
    init_backend();
    prepare_backend();
    reset_state();
  }
}
  

void SpikingModel::init_backend() {

  Backend::init_global_context();
  context = Backend::get_current_context();

  #ifndef SILENCE_MODEL_SETUP
  TimerWithMessages* timer = new TimerWithMessages("Setting Up Network...\n");
  #endif

  context->params.threads_per_block_neurons = 512;
  context->params.threads_per_block_synapses = 512;

  // NB All these also call prepare_backend for the initial state:
  if (spiking_synapses)
    spiking_synapses->init_backend(context);
  if (spiking_neurons)
    spiking_neurons->init_backend(context);
  if (input_spiking_neurons)
    input_spiking_neurons->init_backend(context);
  for (int plasticity_id = 0; plasticity_id < plasticity_rule_vec.size(); plasticity_id++){
    plasticity_rule_vec[plasticity_id]->init_backend(context);
  }
  for (int monitor_id = 0; monitor_id < monitors_vec.size(); monitor_id++){
    monitors_vec[monitor_id]->init_backend(context);
  }

  #ifndef SILENCE_MODEL_SETUP
  timer->stop_timer_and_log_time_and_message("Network set up.", true);
  #endif
}


void SpikingModel::prepare_backend() {
  if (spiking_synapses){
    spiking_synapses->prepare_backend();
    context->params.maximum_axonal_delay_in_timesteps = spiking_synapses->maximum_axonal_delay_in_timesteps;
  }
  if (spiking_neurons)
    spiking_neurons->prepare_backend();
  if (input_spiking_neurons)
    input_spiking_neurons->prepare_backend();
  for (int plasticity_id = 0; plasticity_id < plasticity_rule_vec.size(); plasticity_id++){
    plasticity_rule_vec[plasticity_id]->prepare_backend();
  }
  for (int monitor_id = 0; monitor_id < monitors_vec.size(); monitor_id++){
    monitors_vec[monitor_id]->prepare_backend();
  }
}


void SpikingModel::reset_state() {
  finalise_model();

  if (spiking_synapses)
    spiking_synapses->reset_state();
  if (spiking_neurons)
    spiking_neurons->reset_state();
  if (input_spiking_neurons)
    input_spiking_neurons->reset_state();
  for (int plasticity_id = 0; plasticity_id < plasticity_rule_vec.size(); plasticity_id++){
    plasticity_rule_vec[plasticity_id]->reset_state();
  }
  for (int monitor_id = 0; monitor_id < monitors_vec.size(); monitor_id++){
    monitors_vec[monitor_id]->reset_state();
  }
}


void SpikingModel::perform_per_step_model_instructions(){
  float current_time_in_seconds = current_time_in_timesteps * timestep;
  if (spiking_neurons)
    spiking_neurons->state_update(current_time_in_seconds, timestep);
  if (input_spiking_neurons)
    input_spiking_neurons->state_update(current_time_in_seconds, timestep);
  
  for (int plasticity_id = 0; plasticity_id < plasticity_rule_vec.size(); plasticity_id++)
    plasticity_rule_vec[plasticity_id]->state_update(current_time_in_seconds, timestep);

  if (spiking_synapses)
    spiking_synapses->state_update(spiking_neurons, input_spiking_neurons, current_time_in_seconds, timestep);
  
  for (int monitor_id = 0; monitor_id < monitors_vec.size(); monitor_id++)
    monitors_vec[monitor_id]->state_update(current_time_in_seconds, timestep);

  // Update time
  current_time_in_timesteps += timestep_grouping;

}

void SpikingModel::run(float seconds){
  finalise_model();

  printf("Running model for %f seconds \n", seconds);

  // Calculate the number of computational steps we need to do
  int number_of_timesteps = ceil(seconds / timestep);
  int number_of_steps = ceil(number_of_timesteps / timestep_grouping);

  // Run the simulation for the given number of steps
  for (int s = 0; s < number_of_steps; s++){
    perform_per_step_model_instructions();
  }

  // Carry out any final checks and outputs from recording electrodes
  float current_time_in_seconds = current_time_in_timesteps * timestep;
  for (int monitor_id = 0; monitor_id < monitors_vec.size(); monitor_id++)
    monitors_vec[monitor_id]->final_update(current_time_in_seconds, timestep);

}

