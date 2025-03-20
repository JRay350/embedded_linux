#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <asm/io.h>
#include "../address_map_arm.h"
#include "../interrupt_ID.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Altera University Program");
MODULE_DESCRIPTION("DE1SoC Pushbutton Interrupt Handler");

void * LW_virtual;         // Lightweight bridge base address
volatile int *LEDR_ptr;    // virtual address for the LEDR port
volatile int *KEY_ptr;     // virtual address for the KEY port
volatile int *HEX0_ptr;    // virtual address for the HEX0 port

irq_handler_t irq_handler(int irq, void *dev_id, struct pt_regs *regs)
{
   if (*LEDR_ptr >= 521) // wrap around once 9 is counted
      *LEDR_ptr = 0x200;
   else
      *LEDR_ptr = *LEDR_ptr + 1;
   
   switch (*LEDR_ptr) {
      case 512:
         *HEX0_ptr = 0b111111; // indicate 0 on seven-segment display
         break;
      case 513:
         *HEX0_ptr = 0b110; // indicate 1 on seven-segment display
         break;
      case 514:
         *HEX0_ptr = 0b1011011; // indicate 2 on seven-segment display
         break;
      case 515:
         *HEX0_ptr = 0b1001111; // indicate 3 on seven-segment display
         break;
      case 516:
         *HEX0_ptr = 0b1100110; // indicate 4 on seven-segment display
         break;
      case 517:
         *HEX0_ptr = 0b1101101; // indicate 5 on seven-segment display
         break;
      case 518:
         *HEX0_ptr = 0b1111101; // indicate 6 on seven-segment display
         break;
      case 519:
         *HEX0_ptr = 0b0000111; // indicate 7 on seven-segment display
         break;
      case 520:
         *HEX0_ptr = 0b1111111; // indicate 8 on seven-segment display
         break;
      case 521:
         *HEX0_ptr = 0b1100111; // indicate 9 on seven-segment display
         break;
      default: // this shouldn't happen, so indicate nothing meaning something is wrong
         *HEX0_ptr = 0;
         break;
   }

   // Clear the edgecapture register (clears current interrupt)
   *(KEY_ptr + 3) = 0xF; 
    
   return (irq_handler_t) IRQ_HANDLED;
}

static int __init initialize_pushbutton_handler(void)
{
   int value;
   // generate a virtual address for the FPGA lightweight bridge
   LW_virtual = ioremap_nocache (LW_BRIDGE_BASE, LW_BRIDGE_SPAN);

   LEDR_ptr = LW_virtual + LEDR_BASE;  // init virtual address for LEDR port
   *LEDR_ptr = 0x200;                  // turn on the leftmost light
   
   HEX0_ptr = LW_virtual + HEX3_HEX0_BASE; // init virtual address for HEX0 port
   *HEX0_ptr |= 0b111111; // indicate 0 on seven-segment display

   KEY_ptr = LW_virtual + KEY_BASE;    // init virtual address for KEY port
   // Clear the PIO edgecapture register (clear any pending interrupt)
   *(KEY_ptr + 3) = 0xF; 
   // Enable IRQ generation for the 4 buttons
   *(KEY_ptr + 2) = 0xF; 

   // Register the interrupt handler.
   value = request_irq (KEYS_IRQ, (irq_handler_t) irq_handler, IRQF_SHARED, 
      "pushbutton_irq_handler", (void *) (irq_handler));
   return value;
}

static void __exit cleanup_pushbutton_handler(void)
{
   *LEDR_ptr = 0; // Turn off LEDs and de-register irq handler
   *HEX0_ptr = 0; // Turn off HEX0 seven-segment display
   free_irq (KEYS_IRQ, (void*) irq_handler);

}

module_init(initialize_pushbutton_handler);
module_exit(cleanup_pushbutton_handler);

