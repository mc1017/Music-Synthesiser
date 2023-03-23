# Music Synthesizer Project 

This coursework is a music synthesiser. Embedded software such as RTOS is utilised to make it work. Several real time tasks will need to be executed concurrently, such as detecting key presses and generating the output waveform. The keyboard is controlled using a ST NUCLEO-L432KC microcontroller module that contains a STM32L432KCU6U processor, which has an Arm Cortex-M4 core.

The inputs to the synthesiser are 12 keys (1 octave), 4 control knobs and a joystick. The outputs are 2 channels of audio and an OLED display. There is a serial port for communication with a host via USB, and a CAN bus that allows multiple modules to be stacked together to make a larger keyboard.

The following documentation provides an overview of the coursework project and mainly focusing on the implementation and analysis of the real-time system. The following sections details the features implemented, structure of the code followed by a coursework report. The report detail the tasks performed, their implementation, execution time, scheduler analysis, CPU utilization, shared data structures, and the possibility of a deadlock.

## Table of Contents

1. [Features](#features)
    - [Core Features](#core-features)
    - [Advanced Features](#advanced-features)
2. [Code Structure](#code-structure)
    - [Libraries](#libraries)
    - [Dependancy Graph](#dependancy-graph)
3. [Coursework Report](#coursework-report)
    - [Task Identification and Implementation](#task-identification-and-implementation)
    - [Task Characterisation](#task-characterisation)
    - [Critical Instant Analysis](#critical-instant-analysis)
    - [CPU Utilization](#cpu-utilization)
    - [Shared Data Structures and Synchronization](#shared-data-structures-and-synchronization)
    - [Inter-Task Blocking and Deadlock Analysis](#inter-task-blocking-and-deadlock-analysis)


## Features

The project consists of several key features designed to fulfill its purpose and provide the required functionality:

### Core Features


1. **Feature 1**: Scanning the key matrix to find out which keys are pressed
    - A keyArray is 
2. **Feature 2**: Using an interrupt to generate a sawtooth wave
    - A dedicated processing module processes the collected data in real-time.
    - Advanced algorithms are used to detect specific patterns and events.
3. **Feature 3**: Using threads to allow the key scan and display tasks to be decoupled and executed concurrently
    - A graphical user interface (GUI) allows users to interact with the system.
    - Real-time data visualization helps users to monitor the system status.
4. **Feature 4**: Use a mutex and queue for thread-safe data sharing
    - The system sends notifications to users when specific events are detected.
    - Users can configure the alert thresholds and notification methods.
5. **Feature 5**: Decode the knobs and implement a volume control
    - The system sends notifications to users when specific events are detected.
    - Users can configure the alert thresholds and notification methods.
6. **Feature 6**: Relay key presses to another keyboard module using the CAN bus
    - The system sends notifications to users when specific events are detected.
    - Users can configure the alert thresholds and notification methods.
7. **Feature 7**: Measure execution time of a task
    - The system sends notifications to users when specific events are detected.
    - Users can configure the alert thresholds and notification methods.

### Advanced Features

## Code Structure

The project follows a modular code structure, organized as follows:

- `main.cpp`: The main entry point of the program, responsible for initializing and running the system.

### Libraries 

### Dependacy Graph

## Coursework Report

### Task Identification and Implementation

The real-time system consists of the following tasks:

Task 1: Description of Task 1, implemented as a thread.
Task 2: Description of Task 2, implemented as an interrupt.
Task 3: Description of Task 3, implemented as a thread.

### Task Characterisation

The following table shows the theoretical minimum initiation interval and measured maximum execution time for each task:

Task	Theoretical Minimum Initiation Interval	Measured Maximum Execution Time
1	Value1	Value2
2	Value3	Value4
3	Value5	Value6

### Critical Instant Analysis

A critical instant analysis of the rate monotonic scheduler was performed, showing that all deadlines are met under worst-case conditions. The following steps were taken:

Identify the least common multiple (LCM) of all task periods.
Calculate the response time of each task.
Compare the response time to the respective task's deadline.
The analysis confirmed that all deadlines are met for the given task set under worst-case conditions.

### CPU Utilization

The total CPU utilization was quantified using the following formula for rate monotonic scheduling:

U = Σ(Ci / Ti)

where Ci is the worst-case execution time of task i, and Ti is the task's period. The calculated CPU utilization is X%.

Shared Data Structures and Synchronization

The system uses the following shared data structures:

Shared Data Structure 1: Description of Shared Data Structure 1
Synchronization method used: Mutex locks
Shared Data Structure 2: Description of Shared Data Structure 2
Synchronization method used: Semaphores
These synchronization methods guarantee safe access to the shared data structures and prevent data corruption and race conditions.

Inter-Task Blocking and Deadlock Analysis

An analysis of inter-task blocking dependencies was conducted to identify any possibility of deadlock. The system uses the following resources:

Resource 1: Description of Resource 1
Resource 2: Description of Resource 2
The tasks follow a strict order when acquiring and releasing resources, which prevents circular wait and eliminates the possibility of deadlock.