![tinnitus32](pics/tinnitus32_logo.png "logo")

A digital Theremin based on the STM32 microcontroller.

* Easy to build: Only a STM32 evaluation board and 2 oscillators for a basic
and cheap Theremin
* Excellent audio quality: 16/24bit audio DAC with headphone amplifier
* Loads waveforms with Theremin sound directly from an USB stick
* Very fast autotune: 1sec
* 2 variants: A simple to build basic theremin and a more complex theremin
 with synthesizer functionality
* Powerful ARM Cortex-M4 microcontroller with DSP, FPU and 168MHz clock

![tinnitus32 basic](pics/tinnitus32_basic.png "tinnitus32 basic")

# Hardware
The STM32F407G-DISC1 evaluation board can be used.
So you need only 2 additional oscillators for basic operation.  
![tinnitus32](pics/stm32F407_disco.png "STM32F407G-DISC1")

# Example of an LC colpitts oscillator 
### Schematic
![tinnitus32](pics/tinnitus32_osc_sch.png "tinnitus32 oscillator schematic")

### Bill of material
| Component     | Pitch        | Volume       | Price | Supplier   |
| ------------- | ------------ | ------------ | ----- | ---------- |
| C1            | 82pF         | 100pF        | 0.05$ | [m](http://www.mouser.com), [f](http://www.farnell.com), [d](http://www.digikey.com) |
| C2            | 330pF        | 390pF        | 0.05$ | [m](http://www.mouser.com), [f](http://www.farnell.com), [d](http://www.digikey.com) |
| C3            | 330pF        | 390pF        | 0.05$ | [m](http://www.mouser.com), [f](http://www.farnell.com), [d](http://www.digikey.com) |
| C4            | 100nF        | 100nF        | 0.05$ | [m](http://www.mouser.com), [f](http://www.farnell.com), [d](http://www.digikey.com) |
| R1            | 2M2          | 2M2          | 0.05$ | [m](http://www.mouser.com), [f](http://www.farnell.com), [d](http://www.digikey.com) |
| R2            | 1k           | 1k           | 0.05$ | [m](http://www.mouser.com), [f](http://www.farnell.com), [d](http://www.digikey.com) |
| L1            | 1mH          | 1mH          | 0.50$ | [m](http://www.mouser.com), [f](http://www.farnell.com), [d](http://www.digikey.com) |
| IC            | SN74LVC1GU04 | SN74LVC1GU04 | 0.40$ | [m](http://www.mouser.com), [f](http://www.farnell.com), [d](http://www.digikey.com) |
| Total         |              |              | **1.20$** |            |

**Suppliers:**  
m: www.mouser.com  
f: www.farnell.com / www.newark.com  
d: www.digikey.com  



## Breadboard
The oscillators can be build up with SMD components on a breadboard.
So no extra PCB is necessary:  
![tinnitus32](pics/tinnitus32_osc_pcb.png "tinnitus32 oscillator build on a breadboard")


## Pin maps

### Variant "BASIC THEREMIN"
| Name       | PIN Name | Connector | Description                                    |
| ---------- | -------- | --------- | ---------------------------------------------- |
| GND        | GND      | P2        | Ground for oscillators and multiplexers        |
| 3V         | 3V       | P2        | Supply for oscillators and multiplexers        |
| PITCH_OSC  | PE9      | P1        | Signal from pitch oscillator                   |
| VOLUME_OSC | PE11     | P1        | Signal from volume oscillator                  |
| ANALOG_1   | PA1      | P1        | Analog input from volume potentiometer         |
| ANALOG_2   | PA2      | P1        | Analog input from zoom volume potentiometer    |
| ANALOG_3   | PA3      | P1        | Analog input from shift pitch potentiometer    |
| ANALOG_4   | PC4      | P1        | Analog input from zoom pitch potentiometer     |
| ANALOG_5   | PC5      | P1        | Analog input from waveform potentiometer       |

### Variant "SYNTH THEREMIN"
| Name       | PIN Name | Connector | Description                                    |
| ---------- | -------- | --------- | ---------------------------------------------- |
| GND        | GND      | P2        | Ground for oscillators and multiplexers        |
| 3V         | 3V       | P2        | Supply for oscillators and multiplexers        |
| PITCH_OSC  | PE9      | P1        | Signal from pitch oscillator                   |
| VOLUME_OSC | PE11     | P1        | Signal from volume oscillator                  |
| POT_MUX_A  | PC6      | P2        | Control signal A to multiplexer                |
| POT_MUX_B  | PC8      | P2        | Control signal B to multiplexer                |
| POT_MUX_C  | PC9      | P2        | Control signal C to multiplexer                |
| ANALOG_1   | PA1      | P1        | Analog input from volume potentiometer         |
| ANALOG_2   | PA2      | P1        | Analog input from zoom volume potentiometer    |
| ANALOG_3   | PA3      | P1        | Analog input from shift pitch potentiometer    |
| ANALOG_4   | PC4      | P1        | Analog input from zoom pitch potentiometer     |
| ANALOG_6   | PB0      | P1        | Analog input from multiplexer 6                |
| ANALOG_7   | PB1      | P1        | Analog input from multiplexer 7                |
| ANALOG_8   | PC1      | P1        | Analog input from multiplexer 8                |
| ANALOG_9   | PC2      | P1        | Analog input from multiplexer 9                |
