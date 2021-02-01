#include <stdint.h>

#if !defined(__SOFT_FP__) && defined(__ARM_FP)
#warning "FPU is not initialized, but the project is compiling for an FPU. Please initialize the FPU before use."
#endif

#define SYS_CLK 		16E6 //16MHZ internal clock
#define RAM_START		0x20000000
#define RAM_SIZE		(128* 1024)
#define RAM_END			RAM_START + RAM_SIZE

#define STACK_SIZE		1024
#define TASKS			2
#define TASK1_STACK_START	RAM_END
#define TASK2_STACK_START	RAM_END - 1* STACK_SIZE
#define SCHEDULER_STACK_START	RAM_END - TASKS * STACK_SIZE

uint8_t current_task = 0;

//tasks prototypes
void
task1 (void);
void
task2 (void);

uint32_t *psp[] =
  { (uint32_t*) TASK1_STACK_START, (uint32_t*) TASK2_STACK_START };

uint32_t tasks_ptr[] =
  { (uint32_t) &task1, (uint32_t) &task2 };

void
enable_faults (void)
{
  uint32_t *ptr = (uint32_t*) 0xE000ED24;
  *ptr |= (1 << 16); // mem fault
  *ptr |= (1 << 17); //bus fault
  *ptr |= (1 << 18); //usage fault
}

// set system tick configuration
void
enable_systick (void)
{
  //init registers
  uint32_t *systick_csr = (uint32_t*) 0xE000E010;
  uint32_t *systick_rvr = (uint32_t*) 0xE000E014;
  //set tick period to 1ms
  uint32_t tick = (SYS_CLK / 16E3) - 1; // 1ms
  *systick_rvr = 0x00FFFFFF & tick;
  //enable systick
  *systick_csr |= (1 << 2); //clk source as internal clock
  *systick_csr |= (1 << 1); //enable systick exception
  *systick_csr |= (1 << 0); //enable counter
}

__attribute__((naked)) void
init_scheduler ()
{
  __asm volatile("msr MSP, %0"::"r"(SCHEDULER_STACK_START));
  __asm volatile("bx LR");
}

void
init_task_stack (void)
{
  for (int i = 0; i < TASKS; i++)
    {
      uint32_t *ptr = psp[i];
      //STORE DUMMY STACK FRAME
      ptr = (uint32_t[]
	    )
	      { 0x01000000/*xPSR*/, tasks_ptr[i]/*return addr*/,
		  0xFFFFFFFD/*LR*/, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
      psp[i] = ptr + 16;
    }
}

__attribute__((naked)) void
init_psp (void)
{
  // init psp
  __asm volatile("push {LR}");
  __asm volatile("mov R0, %0"::"r"(psp[current_task]));
  __asm volatile("msr PSP, R0");
  __asm volatile("pop {LR}");
  //change tasks sp to psp
  __asm volatile("mov R0, #0x02");
  __asm volatile("msr CONTROL, R0");
  __asm volatile("bx LR");
}

int
main (void)
{
  enable_faults ();
  init_scheduler ();
  init_task_stack ();
  enable_systick ();
  init_psp ();
  task1 ();
  /* Loop forever */
  for (;;)
    ;
}

void
set_current_task_psp(uint32_t psp_addr)
{
  psp[current_task] = (uint32_t*) psp_addr;
}

void
next_task(void)
{
  current_task = (current_task + 1) % TASKS;
}

__attribute__((naked)) void
SysTick_Handler (void)
{
  /* SAVE THE CURRENT TASK STACK FRAME */

  //1. get the current task psp value
  __asm volatile("mrs R0, PSP");
  //2. push registers R4-R11 to private stack
  __asm volatile("stmdb R0!, {R4-R11}");
  //3. save lr before a series of intermediate calls
  __asm volatile("push {LR}");
  //4. modify lr and save the current R0 value to psp stack
  __asm volatile("bl set_current_task_psp");

  /* POP THE NEXT TASK FRAME AND EXECUTE */

  //1. update the current task index
  __asm volatile("bl next_task");
  //2. get current task psp
  __asm volatile("mov R0, %0"::"r"(psp[current_task]));
  //3. retrieve R4-R11 from psp
  __asm volatile("ldmia R0!, {R4-R11}");
  //4. load sp with current psp
  __asm volatile("msr PSP, R0");
  //5. pop lr
  __asm volatile("pop {LR}");
  //6. resume execution
  __asm volatile("bx LR");
}

void
task1 (void)
{
  while (1)
    {
      ;
    }
}

void
task2 (void)
{
  while (1)
    {
      ;
    }
}
