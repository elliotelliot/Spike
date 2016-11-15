#pragma once

#include "Spike/Neurons/Neurons.hpp"
#include "Spike/Backend/CUDA/CUDABackend.hpp"
#include "Spike/Helpers/CUDAErrorCheckHelpers.hpp"
#include <cuda.h>
#include <vector_types.h>

namespace Backend {
  namespace CUDA {
    class Neurons : Generic {
    public:

      ::Neurons* front;
      
      dim3 number_of_neuron_blocks_per_grid;		/**< CUDA Device number of blocks */
      dim3 threads_per_block;						/**< CUDA Device number of threads */

      /**  
       *  Exclusively for the allocation of device memory. This class requires allocation of d_current_injections only.
       \param maximum_axonal_delay_in_timesteps The length (in timesteps) of the largest axonal delay in the simulation. Unused in this class.
       \param high_fidelity_spike_storage A flag determining whether a bit mask based method is used to store spike times of neurons (ensure no spike transmission failure).
      */
      virtual void allocate_device_pointers();
	
      /**  
       *  Unused in this class. Allows copying of static data related to neuron dynamics to the device.
       */
      virtual void copy_constants_to_device();

      /**  
       *  A local, non-polymorphic function called in to determine the CUDA Device thread (Neurons::threads_per_block) and block dimensions (Neurons::number_of_neuron_blocks_per_grid).
       */
      void set_threads_per_block_and_blocks_per_grid(int threads);
    };
  } // namespace CUDA
} // namespace Backend
