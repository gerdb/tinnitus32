![tinnitus32](pics/tinnitus32_logo.png "logo")

A digital Theremin based on the STM32 microcontroller.

* Very simple setup: Only an evaluation board and 2 oscillators for a basic
and cheap Theremin
* 16/24bit audio DAC with headphone amplifier
* Powerful ARM Cortex-M4 microcontroller with DSP, FPU and 168MHz clock
* Loads waveforms with Theremin sound directly from an USB stick
* Very fast autotune: 1sec

# Hardware
The STM32F407G-DISC1 evaluation board can be used.
So you need only 2 additional oscillators for basic operation.  
![tinnitus32](pics/stm32F407_disco.png "STM32F407G-DISC1")

# Example of an LC colpitts oscillator 
Value for the pitch oscillator: C1=82pF  
Value for the volume oscillator: C1=100pF  
## Schematic
![tinnitus32](pics/tinnitus32_osc_sch.png "tinnitus32 oscillator schematic")

## Breadboard
The oscillators can be build up with SMD components on a breadboard.
So no extra PCB is necessary:  
![tinnitus32](pics/tinnitus32_osc_pcb.png "tinnitus32 oscillator build on a breadboard")
