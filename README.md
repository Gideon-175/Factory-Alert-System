**Real-Time Monitoring and Alert System (RTOS)**

**Overview**
This project implements a real-time industrial monitoring and alert system using an ARM7 LPC23xx microcontroller and Keil RTX RTOS.
The system continuously monitors pressure values, detects abnormal conditions, and generates alerts using an LCD display and buzzer. 
It demonstrates real-time scheduling, task synchronization, and resource sharing using RTOS concepts.

**Key Features**
Real-time multi-tasking using Keil RTX RTOS
Implementation of Immediate Ceiling Priority Protocol (ICPP)
Detection of high-pressure conditions
LCD-based notifications (e.g., shift changes, alerts)
Buzzer alerts for abnormal conditions and manual triggers
External button interrupt handling
LED-based task monitoring (arrival & execution)

**RTOS Concepts Used**
Task Scheduling (Periodic tasks)
Mutex-based synchronization
Priority management
Deadlock avoidance
Priority inversion handling using ICPP

**Immediate Ceiling Priority Protocol (ICPP)**
The project implements a custom version of ICPP to ensure:
No priority inversion
No deadlocks
Predictable execution timing

How it works:
  Each shared resource is assigned a priority ceiling
  When a task acquires a resource:
  Its priority is immediately elevated to the ceiling
On release:
  Original priority is restored

| **Task**       | **Function**                                      |
| ---------- | --------------------------------------------- |
| **Task A** | Displays "Shift Change" messages periodically |
| **Task B** | Monitors pressure values and triggers alerts  |
| **Task C** | Activates buzzer using external push button   |

**Shared Resources:**
  LCD Display
  Buzzer

**Synchronization:**
  Mutex locks + ICPP-based priority handling

**Hardware Components**
  ARM7 LPC23xx Microcontroller
  LCD Display
  Buzzer
  Push Button
  LEDs (for task indication)

**Software Requirements**
  Keil uVision IDE
  Keil RTX RTOS
  Embedded C
