// Weight Normalization (Spiking) Class Header
// SpikingWeightNormPlasiticity.hpp
//
//	Author: Nasir Ahmad
//	Date: 29/05/2016

#ifndef SPIKINGWEIGHTNORMPLASTICITY_H
#define SPIKINGWEIGHTNORMPLASTICITY_H

class SpikingWeightNormPlasticity; // forward definition

#include "Spike/Backend/Macros.hpp"
#include "Spike/Backend/Context.hpp"
#include "Spike/Backend/Backend.hpp"
#include "Spike/Backend/Device.hpp"

#include "Spike/Plasticity/Plasticity.hpp"
#include "Spike/Synapses/SpikingSynapses.hpp"
#include "Spike/Neurons/SpikingNeurons.hpp"

// stdlib allows random numbers
#include <stdlib.h>
// Input Output
#include <stdio.h>
// allows maths
#include <math.h>

namespace Backend {
  class SpikingWeightNormPlasticity : public virtual Plasticity {
  public:
    SPIKE_ADD_BACKEND_FACTORY(SpikingWeightNormPlasticity);
    ~SpikingWeightNormPlasticity() override = default;

    void weight_normalization();
  };
}

static_assert(std::has_virtual_destructor<Backend::SpikingWeightNormPlasticity>::value,
              "contract violated");

// SpikingWeightNormPlasticity Parameters
struct weightnorm_spiking_plasticity_parameters_struct : plasticity_parameters_struct {
	weightnorm_spiking_plasticity_parameters_struct() {}
	// The normalization can be either done with the initialized total or with a specific target
	bool settarget = false;
	float target = 0.0;
};


class SpikingWeightNormPlasticity : public Plasticity {
public:
  SpikingWeightNormPlasticity(SpikingSynapses* synapses, SpikingNeurons* neurons, SpikingNeurons* input_neurons, plasticity_parameters_struct* parameters);
  ~SpikingWeightNormPlasticity() override;

  SPIKE_ADD_BACKEND_GETSET(SpikingWeightNormPlasticity, SpikeBase);
  void reset_state() override;

  weightnorm_spiking_plasticity_parameters_struct* plasticity_parameters = nullptr;
  SpikingSynapses* syns = nullptr;
  SpikingNeurons* neurs = nullptr;

  float* total_afferent_synapse_initial = nullptr;
  float* total_afferent_synapse_changes = nullptr;

  void init_backend(Context* ctx = _global_ctx) override;
  void prepare_backend_early() override;
  virtual void Run_Plasticity(float current_time_in_seconds, float timestep) override;

private:
  std::shared_ptr<::Backend::SpikingWeightNormPlasticity> _backend;
};

#endif
