/*
Copyright (c) 2014 - ngg123

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
*/

/* msp430.h will use the compiler flags to figure out which chip we are targeting
   This will #include the correct msp430x2yyy.h
 */

#include <msp430.h> 
#include <limits.h>

#define		CPU_DUTYCYCLE

#define     RED_LED               BIT0
#define     GRN_LED               ( BIT3)
#define     LED_DIR               P1DIR
#define     LED_OUT               P1OUT
#define     SIDEREAL_RATE         10885//11000 // ~= 1MHz / 8 /8 * 0.714 * cos(atan(1/10))

unsigned char  DRIVER_TABLE[8] = {
  (BIT0|BIT2),
  (BIT2),
  (BIT2|BIT1),
  (BIT1),
  (BIT1|BIT4),
  (BIT4),
  (BIT4|BIT0),
  (BIT0)
};

void initClocks(void) {
  /*

   */
  BCSCTL3 = LFXT1S_2 | XCAP_0; // LFXT = digital external clock, minimal capacitance
  BCSCTL2 = SELM_0 | DIVS_3 | DIVM_0; // MCLK = DCO, no division, SMCLK = WWVB_LO
  // SMCLK = 1MHZ / 8
  BCSCTL1 = CALBC1_1MHZ;
  DCOCTL = CALDCO_1MHZ;
  
}



void initTimer(void){
  TACTL = TASSEL_2 |ID_3| MC_1;
  // TACLK = SMCLK / 8 = 1MHz / 64 = 15.625 kHz
  TACCTL0 = CCIE;
  TACCR0 = SIDEREAL_RATE; // ~= 15625 * .714
  // We want a timer period of 0.714 sec for half-stepping
  // and 1.43 sec for full stepping
}

void setDriver(unsigned char pins){
  __nop();
  P1OUT = (P1OUT & 0xe8)|pins;
}

int main() {
  int driverPointer = 0;
  // Stop the watchdog
  WDTCTL = WDTPW |  WDTHOLD;
  // enable output for pins connected to LEDs on Launchpad
  // init the peripherals
  initClocks();	
  initTimer();

  P1DIR |= BIT0 | BIT1 | BIT2 | BIT4;
  P1OUT |= BIT0 | BIT1 | BIT2 | BIT4;
  P1REN &= ~(BIT0|BIT1|BIT2|BIT4);

  // enable interrupts
  __bis_status_register(GIE);
  

  while(1) {
    __nop();
    __bis_status_register(CPUOFF);
    if (P1IN & BIT3){
      driverPointer +=1;
      TACCR0 = SIDEREAL_RATE;
    } else {
      driverPointer -= 1;
      TACCR0 = 300;
    }
    driverPointer = driverPointer & ((sizeof DRIVER_TABLE / sizeof *DRIVER_TABLE)-1);
    
    setDriver(DRIVER_TABLE[driverPointer]);
  }
}

/* The TimerA ISRs need to be written in assembler because gcc is too brain damaged
   to do the obvious thing when presented with something like A += B (and it also
   insists on pushing a ton of registers on the stack for no reason whatsoever).
*/
__attribute__ ((__naked__,__interrupt__(TIMERA0_VECTOR))) static void
timerA0_isr(void) {
  __bic_status_register_on_exit(CPUOFF);
  asm("reti");
}

__attribute__ ((__naked__)) static void
TACCR1_isr(void) {
	asm("reti");
}

__attribute__ ((__naked__)) static void
TACCR2_isr(void) {
	asm("reti");  // do nothing -- TA2 devices can't get here
}

__attribute__ ((__naked__)) static void
TAOF_isr(void){
	asm("reti"); // do nothing -- TA overflows are expected and normal
}

__attribute__ ((__interrupt__(TIMERA1_VECTOR))) static void
timerA1_isr(void) {
  /*
  TAIV is set according to which source generated the interrupt (CCR1, CCR2, or
  TA overflow).  Adding TAIV to the PC (r0) causes the CPU to branch 1,2, or 5 words
  forward, so the following 5 instructions must do meaningful things in the interrupt
  context (it is best that they are either "reti" or unconditional branches, which are
  both single-word instructions).
  */
  asm("add %0,r0"::"m" (TAIV)); 
  asm("BR %0"::"" (&TACCR1_isr)); // goto CCR1 interrupt handler
  asm("BR %0"::"" (&TACCR2_isr)); // goto CCR2 interrupt handler
  asm("reti"); // reserved flag -- should never be generated
  asm("reti"); // reserved flag -- should never be generated
  asm("BR %0"::"" (&TAOF_isr)); // goto timer overflow handler

}


