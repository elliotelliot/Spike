// -*- mode: c++ -*-
#include "Spike/Backend/CUDA/ActivityMonitor/VoltageActivityMonitor.hpp"

SPIKE_EXPORT_BACKEND_TYPE(CUDA, VoltageActivityMonitor);

namespace Backend {
  namespace CUDA {
    VoltageActivityMonitor::~VoltageActivityMonitor() {
      free(neuron_measurements);
    }

    void VoltageActivityMonitor::reset_state() {
      ActivityMonitor::reset_state();
      num_measurements = 0;
    }

    void VoltageActivityMonitor::prepare() {
      ActivityMonitor::prepare();
      neurons_frontend = frontend()->neurons;
      neurons_backend =
        dynamic_cast<::Backend::CUDA::LIFSpikingNeurons*>(neurons_frontend->backend());
      allocate_pointers_for_spike_count();
    }

    void VoltageActivityMonitor::allocate_pointers_for_spike_count() {
      neuron_measurements = (float*)malloc(sizeof(float)*max_num_measurements);
    }

    void VoltageActivityMonitor::copy_data_to_host(){
      frontend()->neuron_measurements = (float*)realloc(frontend()->neuron_measurements, sizeof(float)*(frontend()->num_measurements + num_measurements));
      for (int i = 0; i < num_measurements; i++){
        frontend()->neuron_measurements[frontend()->num_measurements + i] = neuron_measurements[i];
      }

      frontend()->num_measurements += num_measurements;
      reset_state();
    }

    void VoltageActivityMonitor::collect_measurement
    (unsigned int current_time_in_timesteps, float timestep) {
      CudaSafeCall(cudaMemcpy(neuron_measurements + num_measurements,
                              neurons_backend->membrane_potentials_v + frontend()->neuron_id,
                              sizeof(int), 
                              cudaMemcpyDeviceToHost));

      num_measurements++;
    }


  }
}

