![tinnitus](pics/tinnitus_logo.png "logo")

A digital Theremin based on the STM32 microcontroller.

* Easy to build: Only an STM32 evaluation board and 2 oscillators for a Theremin
* Excellent audio quality: 16/24bit audio DAC with headphone amplifier
* Loads waveforms with Theremin sound directly from an USB stick
* Very fast autotune: 1sec
* Powerful ARM Cortex-M4 microcontroller with DSP, FPU and 168MHz clock

![tinnitus basic](pics/tinnitus_basic.png "tinnitus basic")

# Hardware
The STM32F407G-DISC1 evaluation board can be used.
So you need only 2 additional oscillators and some pots.
![tinnitus](pics/stm32F407_disco.png "STM32F407G-DISC1")

# Example of an LC colpitts oscillator
### Schematic
![tinnitus](pics/tinnitus_osc_sch.png "tinnitus oscillator schematic")

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
| IC            | SN74LVC1GX04 | SN74LVC1GX04 | 0.40$ | [m](http://www.mouser.com), [f](http://www.farnell.com), [d](http://www.digikey.com) |
| Total         |              |              | **1.20$** |            |

**Suppliers:**
m: www.mouser.com
f: www.farnell.com / www.newark.com
d: www.digikey.com



## Breadboard
The oscillators can be build up with SMD components on a breadboard.
So no extra PCB is necessary:
![tinnitus](pics/tinnitus_osc_pcb.png "tinnitus oscillator build on a breadboard")


## Pin maps

| Name       | PIN Name | Connector | Description                                    |
| ---------- | -------- | --------- | ---------------------------------------------- |
| GND        | GND      | P1        | Ground for oscillators and potentiometers      |
| VDD        | VDD      | P1        | 3V supply for oscillators and potentiometers   |
| PITCH_OSC  | PE9      | P1        | Signal from pitch oscillator                   |
| VOLUME_OSC | PE11     | P1        | Signal from volume oscillator                  |
| ANALOG_1   | PA1      | P1        | Analog input from volume potentiometer         |
| ANALOG_2   | PA2      | P1        | Analog input from zoom volume potentiometer    |
| ANALOG_3   | PA3      | P1        | Analog input from shift pitch potentiometer    |
| ANALOG_4   | PC4      | P1        | Analog input from zoom pitch potentiometer     |
| ANALOG_5   | PC5      | P1        | Analog input from waveform potentiometer       |



## Auto-tune
The blue button on the STM32 discovery board starts the Auto-Tune procedure.

![auto tune button](pics/auto_tune.png "auto tune")
You can also use an additional button and connect it to:

| Name       | PIN Name | Connector | Description                   |
| ---------- | -------- | --------- | ----------------------------- |
| VDD        | VDD      | P1        | 3V supply auto-tune button    |
| Auto-tune  | PA0      | P1        | Signal from auto-tune button  |


## Programming software for STM32
Program the STM32 board with the [STM32CUBEPROG](http://www.st.com/content/st_com/en/products/development-tools/software-development-tools/stm32-software-development-tools/stm32-programmers/stm32cubeprog.html) tool.

