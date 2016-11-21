#include "Spike/Backend/CUDA/Synapses/Synapses.hpp"
#include <thrust/device_vector.h>
#include <thrust/host_vector.h>
#include <thrust/count.h>

namespace Backend {
  namespace CUDA {
    Synapses::~Synapses() {
      CudaSafeCall(cudaFree(presynaptic_neuron_indices));
      CudaSafeCall(cudaFree(postsynaptic_neuron_indices));
      CudaSafeCall(cudaFree(temp_presynaptic_neuron_indices));
      CudaSafeCall(cudaFree(temp_postsynaptic_neuron_indices));
      CudaSafeCall(cudaFree(synaptic_efficacies_or_weights));
      CudaSafeCall(cudaFree(temp_synaptic_efficacies_or_weights));
      CudaSafeCall(cudaFree(synapse_postsynaptic_neuron_count_index));
    }

    void Synapses::reset_state() {
      CudaSafeCall(cudaMemset(spikes_travelling_to_synapse, 0, sizeof(int)*total_number_of_synapses));
      // TODO: This is a copy involving the front-end, so need to think about synchrony...
      CudaSafeCall(cudaMemcpy(time_of_last_spike_to_reach_synapse, last_spike_to_reach_synapse, total_number_of_synapses*sizeof(float), cudaMemcpyHostToDevice));
    }
    
    void Synapses::allocate_device_pointers() {
      printf("Allocating synapse device pointers...\n");

      CudaSafeCall(cudaMalloc((void **)&presynaptic_neuron_indices, sizeof(int)*total_number_of_synapses));
      CudaSafeCall(cudaMalloc((void **)&postsynaptic_neuron_indices, sizeof(int)*total_number_of_synapses));
      CudaSafeCall(cudaMalloc((void **)&synaptic_efficacies_or_weights, sizeof(float)*total_number_of_synapses));
      CudaSafeCall(cudaMalloc((void **)&synapse_postsynaptic_neuron_count_index, sizeof(float)*total_number_of_synapses));
    }


    void Synapses::copy_constants_and_initial_efficacies_to_device() {
      printf("Copying synaptic constants and initial efficacies to device...\n");

      CudaSafeCall(cudaMemcpy(presynaptic_neuron_indices, presynaptic_neuron_indices, sizeof(int)*total_number_of_synapses, cudaMemcpyHostToDevice));
      CudaSafeCall(cudaMemcpy(postsynaptic_neuron_indices, postsynaptic_neuron_indices, sizeof(int)*total_number_of_synapses, cudaMemcpyHostToDevice));
      CudaSafeCall(cudaMemcpy(synaptic_efficacies_or_weights, synaptic_efficacies_or_weights, sizeof(float)*total_number_of_synapses, cudaMemcpyHostToDevice));
      CudaSafeCall(cudaMemcpy(synapse_postsynaptic_neuron_count_index, synapse_postsynaptic_neuron_count_index, sizeof(float)*total_number_of_synapses, cudaMemcpyHostToDevice));
    }


    void Synapses::set_threads_per_block_and_blocks_per_grid(int threads) {
      threads_per_block.x = threads;
      number_of_synapse_blocks_per_grid = dim3(1000);
    }
    
    __global__ void set_neuron_indices_by_sampling_from_normal_distribution
    (int total_number_of_new_synapses,
     int postsynaptic_group_id,
     int poststart, int prestart,
     int post_width, int post_height,
     int pre_width, int pre_height,
     int number_of_new_synapses_per_postsynaptic_neuron,
     int number_of_postsynaptic_neurons_in_group,
     int * d_presynaptic_neuron_indices,
     int * d_postsynaptic_neuron_indices,
     float * d_synaptic_efficacies_or_weights,
     float standard_deviation_sigma,
     bool presynaptic_group_is_input,
     curandState_t* d_states) {

      int idx = threadIdx.x + blockIdx.x * blockDim.x;
      int t_idx = idx;
      while (idx < total_number_of_new_synapses) {
		
        int postsynaptic_neuron_id = idx / number_of_new_synapses_per_postsynaptic_neuron;
        d_postsynaptic_neuron_indices[idx] = poststart + postsynaptic_neuron_id;

        int postsynaptic_x = postsynaptic_neuron_id % post_width; 
        int postsynaptic_y = floor((float)(postsynaptic_neuron_id) / post_width);
        float fractional_x = (float)postsynaptic_x / post_width;
        float fractional_y = (float)postsynaptic_y / post_height;

        int corresponding_presynaptic_centre_x = floor((float)pre_width * fractional_x); 
        int corresponding_presynaptic_centre_y = floor((float)pre_height * fractional_y);

        bool presynaptic_x_set = false;
        bool presynaptic_y_set = false;
        int presynaptic_x = -1;
        int presynaptic_y = -1; 

        while (true) {

          if (presynaptic_x_set == false) {
            float value_from_normal_distribution_for_x = curand_normal(&d_states[t_idx]);
            float scaled_value_from_normal_distribution_for_x = standard_deviation_sigma * value_from_normal_distribution_for_x;
            int rounded_scaled_value_from_normal_distribution_for_x = round(scaled_value_from_normal_distribution_for_x);
            presynaptic_x = corresponding_presynaptic_centre_x + rounded_scaled_value_from_normal_distribution_for_x;
            if ((presynaptic_x > -1) && (presynaptic_x < pre_width)) {
              presynaptic_x_set = true;
            }

          }

          if (presynaptic_y_set == false) {
			
            float value_from_normal_distribution_for_y = curand_normal(&d_states[t_idx]);
            float scaled_value_from_normal_distribution_for_y = standard_deviation_sigma * value_from_normal_distribution_for_y;
            int rounded_scaled_value_from_normal_distribution_for_y = round(scaled_value_from_normal_distribution_for_y);
            presynaptic_y = corresponding_presynaptic_centre_y + rounded_scaled_value_from_normal_distribution_for_y;
            if ((presynaptic_y > -1) && (presynaptic_y < pre_height)) {
              presynaptic_y_set = true;
            }

          }

          if (presynaptic_x_set && presynaptic_y_set) {
            d_presynaptic_neuron_indices[idx] = CORRECTED_PRESYNAPTIC_ID(prestart + presynaptic_x + presynaptic_y*pre_width, presynaptic_group_is_input);
            break;
          }
			

        }	
        idx += blockDim.x * gridDim.x;

      }	

      __syncthreads();

    }

  }
}

