# Music Synthesizer Project 

This coursework is a music synthesiser. Embedded software such as RTOS is utilised to make it work. Several real time tasks will need to be executed concurrently, such as detecting key presses and generating the output waveform. The keyboard is controlled using a ST NUCLEO-L432KC microcontroller module that contains a STM32L432KCU6U processor, which has an Arm Cortex-M4 core.

The inputs to the synthesiser are 12 keys (1 octave), 4 control knobs and a joystick. The outputs are 2 channels of audio and an OLED display. There is a serial port for communication with a host via USB, and a CAN bus that allows multiple modules to be stacked together to make a larger keyboard.

The following documentation provides an overview of the coursework project and mainly focusing on the implementation and analysis of the real-time system. The following sections details the features implemented, structure of the code followed by a coursework report. The report detail the tasks performed, their implementation, execution time, scheduler analysis, CPU utilization, shared data structures, and the possibility of a deadlock.

## Table of Contents

- [Music Synthesizer Project](#music-synthesizer-project)
  - [Table of Contents](#table-of-contents)
  - [Features](#features)
    - [Core Features](#core-features)
    - [Advanced Features](#advanced-features)
  - [Code Structure](#code-structure)
    - [Libraries](#libraries)
    - [Dependacy Graph](#dependacy-graph)
  - [Coursework Report](#coursework-report)
    - [Task Identification and Implementation](#task-identification-and-implementation)
    - [Task Characterisation](#task-characterisation)
    - [Critical Instant Analysis](#critical-instant-analysis)
    - [CPU Utilization](#cpu-utilization)
    - [Shared Data Structures](#shared-data-structures)
    - [Intertask Blocking Dependencies](#intertask-blocking-dependencies)
## Features

The project consists of several key features designed to fulfill its purpose and provide the required functionality:

### Core Features


Knob Control Functions Layouts

| Type | Knob Location (Left=0, Right=3) | Range | Fucntions |
| --- | --- | --- | --- |
| Wave  | 0th Knob | 0-3 | Sawtooth, Triangle, Square, Sine |
| Octave | 1st Knob | 3-6 | Octave Level 3-6 (Extendable with multiple boards) | 
| Modes |  2nd Knob | 0-2 | Normal, Drum, LFO |
| Volume |  3rd Knob | 0-8 | Volume Level |



**Feature 1**: Scanning the key matrix to find out which keys are pressed
    - A keyArray containing information on each state of the keyboard is being updated via the `scanKeyTask`
**Feature 2**: Using an interrupt to generate a sawtooth wave
    - `sampleISR` is used to interrupt and sample the `keyArray` at a frequency of 22kHz to generate sound
**Feature 3**: Using threads to allow the key scan and display tasks to be decoupled and executed concurrently 
    - `scanKeyTask` and `updateDisplayTask` are implemented as threads to be executed concurrently
**Feature 4**: Use a mutex and queue for thread-safe data sharing
    - Mutexes and Queues such as `keyArrayMutex`, `rotationMutex`, and `currentStepSizeMutex` are implemented to eliminate the shared data problem
**Feature 5**: Decode the knobs and implement a volume control
    - Knobs are decoded in functions within class `Knob` to obtain the current state and displacement of oreintation
    - Volume can be controlled with knob 4 (rightmost) 
**Feature 6**: Relay key presses to another keyboard module using the CAN bus
    - Handshaking and autodetection is used to identify neighbouring boards
    - CAN coommunication between keybaords are implemented so that sound can be transmitted from transmitter to receiver
**Feature 7**: Measure execution time of a task
    - Each task's latency is measured separated and obtained
    - Execution time is analysed and obtained critical instant and CPU utilisation

### Advanced Features

1. **Hand Shaking**: Using handshake to inform the board of whether there are multiple boards, whether it is a transmitter or reciever, and what position it is at among multiple connected keyboards at start-up. 
2. **Multiple Waveforms**: There are four types of waveforms implemented for the main oscillator of the synthesizer; sine, sawtooth, triangle, and square waves. The implementation of the phase accumulator is the same for all four waves. The phase accumulator, array variable `phaseAcc[12]`, takes some value from 0 to INT32, above which it overflows and loops back to 0.
3. **LFO**: The two variables `currentPitchLFOStepSize` and `currentVolLFOStepSize` control the frequency of the LFOs. They are incremented to the phase accumulators of their respective LFOs for pitch and volume automation so that they can have separate frequencies independent of each other. The LFO’s waveforms for both pitch and volume automation are triangle waves.

4. **Drum Sound Effects**: With a toggle, the synthesizer is able to play multiple different sound effects. 
5. **Display**: The display is capable of showing the following information: 
   - Module position among multiple connected keyboards. (Top-right corner)
   - Module role as either transmitter, reciever, or lone module. (Top-right corner)
   - Current Key being pressed (including other keyboards on the reciever keyboard)
   - Current playing waveform type 
   - Current playing octave number 
   - Current mode for other advanced features 
   - Current playing volume 
6. **CAN Communication**: Transmitter keyboards will send key array information to the reciever borad, along with it's position among multiple connected keyboards. The reciever board will play the note according the the recieved information. (With connectoin up to 2 keyboards!) The CAN message is in the following format: Byte 0-3: the keyArray of the keyboard. if no key is pressed, the code will show 0xf for all bytes. Byte 5 is set as position variable which store the position of the keyboard in the handshake. It is used to determine which octave it is in for the key.
7. **Polyphony** : The keyboard is capable of playing all 12 keys simutaneuously (possibly more with connected keyboards). This is done by scanning through all 12 keys during the interrupt, and incrementing all keys based on its step size. The Vout is then divided by the number of active keys to avoid clipping.

## Code Structure

The project follows a modular code structure, organized as follows:
`main.cpp`: The main entry point of the program, responsible for initializing and running the system. 

All scheduling related tasks are implemented in `main.cpp`: 
- Initialisation of ISRs, CAN Bus Communitcation, and Handshakes: 
  - Performs initialization that sets filters, registers CAN Bus related ISRs, etc. 
- Data Sharing: 
  - Creates Mutexes and Saraphones for variables such as the `keyArrayMutex`, `RX_MessageMutex` for safe data sharing between tasks. 
- RTOS Tasks: 
  - Scans Key matrix and updates variables for displaying / playing sound. 
  - Displays relevant information that is provided by the ScanKeys task. 
  - Plays notes according to the information provided by the ScanKeys task. 
  - Sends information provided by the ScanKeys task or recieves information from the other boards depending on their configuration. 
- Starts RTOS: 
  - Initializes and runs appropreate threads according to their configuration (Only module, transmitter, or reciever). 
  - Starts the RTOS scheduler. 

All feature-specific code are included as [Libraries](#libraries). 


### Libraries 

The following imported libraries are included in `main.cpp`: 
- `<Arduino.h>`: Utilize an Arduino-like environment that makes it easy to access microcontroller hardware features. 
- `<U8g2lib.h>`: Access useful functions to program the display on the keyboard. 
- `<STM32FreeRTOS.h>`: Utilize an RTOS system for handling multithreading. 

The following custom libraries are included in `main.cpp`: 
- `<ES_CAN.h>`: 
  - (From ES-synth-starter)For initialization and configuration of the CAN module on the STM32 using the HAL library. 
- `<Keyboard.h>`: 
  - Defines the STM32 pin assignments, input/outputs which include the key matrix, audio outputs, and the joystick input. 
  - Defines the class "Knob"  used to read and track the rotary encoders' input.
  - Defines functions to update the state of the rotary encoders. 
- `<CAN_HandShake.h>`:
  - Initializes various variables and constants for assigning the keyabord as either a transmitter / reciever. 
  - Runs a Handshake Routine to inform the board of whether there are multiple boards, whether it is a transmitter or reciever, and what position it is at among multiple connected keyboards. 
  - Includes other functions to perform tasks such as sending CAN messages, printing debug information to a display. 
- `<Waveform.h>`: 
  - Defines several functions to generate different types of waveforms (sawtooth, triangle, square, sine). 
  - Defines arrays of step sizes for each octave and uses these arrays to generate the waveforms at the appropriate frequencies.
  - Implements LFO (low-frequency oscillator) function that could be used to add volume and pitch automation
- `<Display.h>`: 
  - Uses the u8g2 library to display various peices of infromation on the screen such as Volume, Octave, Waveform, Keys pressed, and includes a loading screen for the handshake process. 


### Dependacy Graph
```
Dependency Graph
|-- U8g2 @ 2.34.15
|   |-- SPI @ 1.0.0
|   |-- Wire @ 1.0.0
|-- STM32duino FreeRTOS @ 10.3.1
|-- CAN_HandShake
|   |-- Display
|   |   |-- Keyboard
|   |   |-- U8g2 @ 2.34.15
|   |   |   |-- SPI @ 1.0.0
|   |   |   |-- Wire @ 1.0.0
|   |-- ES_CAN
|-- Display
|   |-- Keyboard
|   |-- U8g2 @ 2.34.15
|   |   |-- SPI @ 1.0.0
|   |   |-- Wire @ 1.0.0
|-- Drum
|   |-- Keyboard
|-- ES_CAN
|-- Keyboard
|-- Waveform
|   |-- Keyboard
```
## Coursework Report

### Tasks Identification

The real-time music synthesiser consists of the following tasks:


1. A task that reads input from the matrix keypad and updates a shared data structure that represents the state of the keys in `keyArray`.
2. A task that updates the display with information about the current state of the system.
3. A task that sends messages over the CAN bus and act as the transmitter.
4. A task that receives messages over the CAN bus and act as the receiver.
5. An interrupt that samples the waveform output to a DAC (digital-to-analog converter) at a fixed frequency and sets the output voltage based on the active keys on a matrix keypad. This task also ensures that the output voltage does not clip.
6. An interrupt that sends CAN dataframe whenever buffer is full.
7. An interrupt that reads CAN dataframe whenever buffer is full. 

### Task Characterisation

The following table shows the theoretical minimum initiation interval and measured maximum execution time for each task:


| Task              | Priority   | Initiation Interval ($τ_i$)| Execution Time  ($T_i$)|
|-------------------|------------|---------------------|-----------------|
| displayUpdateTask | 1          | 100 ms              | 16.021 ms       |                   
| scanKeysTask      | 2          | 20 ms               | 65.6 µs         |                   
| CAN_RX_Task       | 3          | 25.2 ms             | 30 µs           |                  
| CAN_TX_Task       | 3          | 60 ms               | 459 µs          |                   
| sampleISR         | High       | 45.5 µs             | 29 µs           |               
| CAN_RX_ISR        | High       | 0.7 ms              |  1  µs          |         
| CAN_TX_ISR        | High       | 0.7 ms              |  1 µs           |

Rate-monotonic scheduling (RMS) is used to determine the priority of tasks. It ensures an optimal CPU utilisation when shortest initiation interval task gets highest priority. 

Following the rules from RMS, interrupts generally have the highest priority because they need to respond quickly to events. In this case, the ISRs have a very short theoretical initiation interval, making them the highest priority tasks. `updateDisplayTask` has the longest interval (100 ms) and should have the lowest priority.

There are 3 noteworthy implementation details from the above table: 

Firstly, you might notice that the priority of CAN_RX_Task and CAN_TX_Task are the same. This is because for each board, it can either be transmitter or receiver. Thus, there is a maximum of 3 threads running for each board.

Secondly, CAN_RX_Task and CAN_TX_Task have a higher priority than scanKeysTask although the initiaion interval is higher. CAN_RX_Task and CAN_TX_Task has a shorter interval (25.2 ms and 60ms) than ScanKeyTask (20 ms), but they also have to execute 36 times during that interval. This means they have higher frequency of execution and thus requires a higher priority to ensure all of its instances are executed within the given interval.

Lastly, CAN_RX_Task is implemented rather than the decodeTask from the Lab2 instructions. Instead of decoding a message, we have chosen to send across the keyArray directly from the transmitter board to the receiver board. Octave information is obtained from the position of the transmitter board. The information from octave and keyArray are sufficient for our feature implementation, thus complex encoding and decoding is not required. This avoids the latency from performing decoding and resulted in a much quicker communication between boards. 


### Critical Instant Analysis

A critical instant analysis of the rate monotonic scheduler was performed, showing that all deadlines are met under worst-case conditions. The following steps were taken:

1. Consider the latency the lowest-priority task ($τ_n$)at the worst-case instant in time
2. Calculate the number of iterations each task has to run per highest latency 

The latentcy of lowest-priority task ($τ_n$) = 100ms from displayUpdateTask

| Initiation Interval ($τ_i$)| Execution Time  ($T_i$)|  $[\frac{τ_n}{τ_i}]$| Latency $[\frac{τ_n}{τ_i}]$($T_i$)|
|---------------------|-----------------|---------------------|-----------------|
| 100 ms              | 16.021 ms       | 1            | 16.021 ms       |                  
| 20 ms              | 65.6 µs         |  5               | 0.328 ms       |                  
| 25.2 ms             | 30 µs           |  4              | 0.12 ms       |                 
| 60 ms               | 459 µs          | 1.6              | 0.7344 ms       |                   
| 45.5 µs             | 29 µs           | 2198              | 63.742 ms       |               
| 0.7 ms              |  1  µs          | 143              | 0.143 ms       |         
| 0.7 ms              |  1 µs           | 143              | 0.143 ms       |
|  -             |  -           | -              |      81.2314 ms  |

Total Latency = 81.2314 ms < 100ms

Since the critical instant is lower than the latency of the lowest-priority task, the scheduller will run everything on time. The analysis confirmed that all deadlines are met for the given task set under worst-case conditions.

### CPU Utilization

The total CPU utilization was quantified using the following formula for rate monotonic scheduling:

`CPU Utilization (%) = (Execution Time / Total Time) * 100`

The LCM of all tasks is found to be close to 100ms. Thus, the tasks will run at a period of 100ms.

Since the total execution time is 81.2314 ms for a period of 100ms, based on the formula above, the CPU utilisation is 81.2314%


### Shared Data Structures 

The code deploys mutexes and semaphores to guarantee safe access and synchronisation between tasks. 

These are the descriptions of Mutexes and Semaphores used: 
- `keyArrayMutex`: protects the `keyArray` which stores the state of the matrix keypad.
- `minMaxMutex`: protects the initialisation of knobs minimum and maximum values.
- `rotationMutex`: protects the variable that controls the output voltage, using the rotation of a knob.
- `currentStepSizeMutex`: protects the variable that sets the step size for sampling the waveform output. 
- `RX_MessageMutex`: protects the CAN RX message queue used for receiving messages over the CAN bus.
- `CAN_TXSemaphore`: protects the CAN TX queue, allowing only one task to transmit data over the CAN bus at once.

### Intertask Blocking Dependencies and Deadlock Analysis 

An analysis of inter-task blocking dependencies was conducted to identify any possibility of deadlock.

A deadlock occurs when two or more tasks are unable to proceed because they are each waiting for the other to release a resource. Four necessary conditions must hold simultaneously for a deadlock to occur:

1. Mutual Exclusion: At least one resource must be held in a non-shareable mode, meaning only one task can use the resource at a time.
2. Hold and Wait: A task must be holding at least one resource and waiting for additional resources that are currently held by other tasks.
3. No Preemption: Resources cannot be forcibly removed from a task once they have been allocated; the task must release the resource voluntarily.
4. Circular Wait: A circular chain of tasks exists, where each task is waiting for a resource held by the next task in the chain.

`keyArrayMutex`, `rotationMutex`, and `currentStepSizeMutex` are mutexes used to protect variables from the shared data problem. The other mutexes are to prevent interrupt from obtaining an intermediate values. 

It is possible for any tasks that take any of the above mutexes to block other tasks that rely on the same mutex. Thus creating a dependancies between these tasks. For example, `scanKeyTask` and `updateDisplayTask` both require the use of `keyArray`, thus creating dependancies between them. 

There is no situation where Hold and Wait and Circular Wait occurs simutaneously as there is no task that wait for a resource held by next task in the chain. Thus, there is no possibility of a deadlock from 




