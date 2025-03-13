#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "../address_map_arm.h"

/* Prototypes for functions used to access physical memory addresses */
int open_physical (int);
void * map_physical (int, unsigned int, unsigned int);
void close_physical (int);
int unmap_physical (void *, unsigned int);

/* This program increments the contents of the red LED parallel port */
int main(void)
{
   volatile int * LEDR_ptr;   // virtual address pointer to red LEDs

   int fd = -1;               // used to open /dev/mem for access to physical addresses
   void *LW_virtual;          // used to map physical addresses for the light-weight bridge
    
   // Create virtual memory access to the FPGA light-weight bridge
   if ((fd = open_physical (fd)) == -1)
      return (-1);
   if ((LW_virtual = map_physical (fd, LW_BRIDGE_BASE, LW_BRIDGE_SPAN)) == NULL)
      return (-1);

   // Set virtual address pointer to I/O port
   LEDR_ptr = (unsigned int *) (LW_virtual + LEDR_BASE);
    
   //clear LED parallel port register
   *LEDR_ptr &= ~(0b1111111111);

   // Add 1 to the I/O register to light LEDR0
   *LEDR_ptr = *LEDR_ptr + 1;

   // Initialize delay seconds
   struct timespec tim;
   tim.tv_sec = 0; // 0 full seconds
   tim.tv_nsec = 250000000; // 0.25 seconds

   int led_counter = 0;
   int toggle_counter = 0;
   bool is_increasing = true;
   while (toggle_counter < 6) {
      nanosleep(&tim, NULL); // delay 0.25 seconds

      if (is_increasing) {
         *LEDR_ptr *= 2; // increment LED if increasing
         led_counter++;
      } else {
         *LEDR_ptr /= 2; // decrement LED if decreasing
         led_counter--;
      }

      if (led_counter == 0 || led_counter == 9) {
         is_increasing ^= 1; // toggle
         toggle_counter++;
      }
   }
    
   unmap_physical (LW_virtual, LW_BRIDGE_SPAN);   // release the physical-memory mapping
   close_physical (fd);   // close /dev/mem
   return 0;
}

// Open /dev/mem, if not already done, to give access to physical addresses
int open_physical (int fd)
{
   if (fd == -1)
      if ((fd = open( "/dev/mem", (O_RDWR | O_SYNC))) == -1)
      {
         printf ("ERROR: could not open \"/dev/mem\"...\n");
         return (-1);
      }
   return fd;
}

// Close /dev/mem to give access to physical addresses
void close_physical (int fd)
{
   close (fd);
}

/*
 * Establish a virtual address mapping for the physical addresses starting at base, and
 * extending by span bytes.
 */
void* map_physical(int fd, unsigned int base, unsigned int span)
{
   void *virtual_base;

   // Get a mapping from physical addresses to virtual addresses
   virtual_base = mmap (NULL, span, (PROT_READ | PROT_WRITE), MAP_SHARED, fd, base);
   if (virtual_base == MAP_FAILED)
   {
      printf ("ERROR: mmap() failed...\n");
      close (fd);
      return (NULL);
   }
   return virtual_base;
}

/*
 * Close the previously-opened virtual address mapping
 */
int unmap_physical(void * virtual_base, unsigned int span)
{
   if (munmap (virtual_base, span) != 0)
   {
      printf ("ERROR: munmap() failed...\n");
      return (-1);
   }
   return 0;
}

