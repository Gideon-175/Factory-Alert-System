#include <RTL.h>
#include <LPC23xx.H>
#include <stdio.h>
#include <string.h>
#include "LCD.h"

#define BUTTON_PIN   (1 << 9)   // P2.9 – external push button
#define BUZZER_PIN   (1 << 7)   // P3.7 - external buzzer

// Mutexes for resources
OS_MUT lcd_mutex;
OS_MUT buzzer_mutex;

// LED definitions
#define LED1 0x01  // TaskA arrived
#define LED2 0x02  // TaskB arrived
#define LED3 0x04  // TaskC arrived
#define LED4 0x08  // TaskA running
#define LED5 0x10  // TaskB running
#define LED6 0x20  // TaskC running

// Task IDs
OS_TID taskA_id;
OS_TID taskB_id;
OS_TID taskC_id;

// Ceiling priorities for resources
#define LCD_CEILING_PRIORITY    3   // Highest priority needed for LCD
#define BUZZER_CEILING_PRIORITY 3   // Highest priority needed for Buzzer

// Original task priorities
#define TASK_A_PRIORITY 1
#define TASK_B_PRIORITY 3  
#define TASK_C_PRIORITY 2

// Current active ceiling priority
int current_ceiling_priority = 0;

// Pressure data
int pressure_values[20] = {168,151,148,151,155,160,145,142,139,135,140,150,151,153,149,147,145,120,125,130};

/* --- Delay Function --- */
void delay(int a) {
    volatile int i,j;
    for (i = 0; i < 500000; i++)
        for(j=0; j<a ;j++);
}

/* --- Buzzer Control --- */
void Buzzer_Init(void) {
    PINSEL6 &= ~(3 << 14);
    FIO3DIR |= BUZZER_PIN;
}

void Buzzer_On(void) {
    FIO3SET = BUZZER_PIN;
}

void Buzzer_Off(void) {
    FIO3CLR = BUZZER_PIN;
}

void Buzzer_Alert() {
    int i=0;
    for(i=0;i<3;i++) {
        Buzzer_On();
        delay(3);
        Buzzer_Off();
        delay(3);
    }
}

/* --- Button Control --- */
void Button_Init(void) {
    PINSEL4 &= ~(3 << 20);         
    FIO2DIR &= ~BUTTON_PIN;
}

/* --- LCD Displaying --- */
void LCD_Display(char *msg) {
    LCD_cls();
    LCD_gotoxy(1,1);
    LCD_puts((U8 *)msg);
}

/* --- ICPP Protocol Implementation --- */
// Simplified but correct ICPP implementation
int acquire_resource(OS_MUT *mutex, int ceiling_priority, OS_TID task_id, int original_priority) 
{
    // STEP 1: First wait for and acquire the mutex (may block here)
    while(os_mut_wait(mutex, 0xFFFF) != OS_R_OK) ;
    
    // STEP 2: Now that we HAVE the mutex, raise priority to ceiling
    if (ceiling_priority > original_priority) {
        os_tsk_prio(task_id, ceiling_priority);
    }
    
    // STEP 3: Update global ceiling
    if (ceiling_priority > current_ceiling_priority) {
        current_ceiling_priority = ceiling_priority;
    }
    
    return 1; // Success
}

void release_resource(OS_MUT *mutex, int ceiling_priority, OS_TID task_id, int original_priority) 
{
    // STEP 1: Restore original priority
    os_tsk_prio(task_id, original_priority);
    
    // STEP 2: Release the mutex (now other tasks can acquire it)
    os_mut_release(mutex);
    
    // STEP 3: Reset global ceiling (in real system, you'd recalculate based on all held resources)
    current_ceiling_priority = 0;
}
/* ---------------------------------------------------------------------------------------------------------------- */
/* --- TASKS WITH ICPP --- */

__task void TaskA(void) {
    int my_original_priority = TASK_A_PRIORITY;
    
    while(1) {
				os_dly_wait(1000);
        FIO2SET = LED1;
        
        // Acquire LCD with ceiling protocol
        if (acquire_resource(&lcd_mutex, LCD_CEILING_PRIORITY, taskA_id, my_original_priority)) {
            FIO2SET = LED4; 
            FIO2CLR = LED1;
            
          LCD_Display("Shift Change");
                    
          delay(50);
                 
          LCD_cls();
					
					// Release resourse         
					 FIO2CLR = LED4; 
					release_resource(&lcd_mutex, LCD_CEILING_PRIORITY, taskA_id, my_original_priority);
                    
           
        }
        
        os_dly_wait(3000);
    }
}

__task void TaskB(void) {
    int my_original_priority = TASK_B_PRIORITY;
    int i;
    int pressure;
    char display[32];       
   
    while(1) {
        for(i=0;i<20;i++) {
            pressure = pressure_values[i];
            if (pressure > 155) {
                sprintf(display, "High Pressure! - %d", pressure);
                FIO2SET = LED2;
                
                // Acquire LCD first (maintain consistent order)
                if (acquire_resource(&lcd_mutex, LCD_CEILING_PRIORITY, taskB_id, my_original_priority)) {
                    // Then acquire buzzer
                    if (acquire_resource(&buzzer_mutex, BUZZER_CEILING_PRIORITY, taskB_id, my_original_priority)) {
                        FIO2SET = LED5;
                        FIO2CLR = LED2;
                        
                        LCD_Display(display);
                        Buzzer_Alert();
                        LCD_cls();
                        
                        FIO2CLR = LED5;
                        
                        // Release in reverse order
                        release_resource(&buzzer_mutex, BUZZER_CEILING_PRIORITY, taskB_id, my_original_priority);
                    }
                    release_resource(&lcd_mutex, LCD_CEILING_PRIORITY, taskB_id, my_original_priority);
                }
            }
            os_dly_wait(300);
        }
    }		
}

__task void TaskC(void) {
    int my_original_priority = TASK_C_PRIORITY;
    
    while(1) {
        if (!(FIO2PIN & BUTTON_PIN)) {
                FIO2SET = LED3;
                
                // Acquire buzzer with ceiling protocol
                if (acquire_resource(&buzzer_mutex, BUZZER_CEILING_PRIORITY, taskC_id, my_original_priority)) {
                    FIO2SET = LED6;
                    FIO2CLR = LED3;
                    
                    Buzzer_On();
                    delay(10);
                    Buzzer_Off();
                    
                    FIO2CLR = LED6;
                    
                    release_resource(&buzzer_mutex, BUZZER_CEILING_PRIORITY, taskC_id, my_original_priority);
                }
				}

        os_dly_wait(10);
    }
}

/* --- Task Creation --- */
__task void init(void) {
    os_mut_init(&lcd_mutex);
    os_mut_init(&buzzer_mutex);

    taskA_id = os_tsk_create(TaskA, TASK_A_PRIORITY);
    taskB_id = os_tsk_create(TaskB, TASK_B_PRIORITY);
    taskC_id = os_tsk_create(TaskC, TASK_C_PRIORITY);

    os_tsk_delete_self();
}

/* --- Initialization --- */
void Initial_Init(void) {
    FIO2DIR |= LED1 | LED2 | LED3 | LED4 | LED5 | LED6;
    FIO2PIN  = 0x00000000;
    
    Buzzer_Init();
    Button_Init();
    
    LCD_init();              
    LCD_cls();
    
    current_ceiling_priority = 0; // Initialize ceiling
}

/* --- Main function --- */
int main(void) {
    Initial_Init();                	
    os_sys_init(init);
    return 0;
}
