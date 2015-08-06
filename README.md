# barndoor
Software for a microcontroller to run a barn-door astrophotography mount.

The basic idea of this firmware is to control a stepper motor for a "barndoor" style astrophotography mount.   The barndoor consists of two pieces of wood, attached with a hinge.  A good description is given here: https://en.wikipedia.org/wiki/Barn_door_tracker

A stepper motor turns a threaded rod, to push apart the two blocks.  Traditional approaches have used a quartz oscillator running at a well-known (and tuned!) rate, but this leads to a problem where the angular velocity of the mount is not constant--in fact, it goes as 1/sin(theta).  One approach to this problem is to increase the mechanical complexity of the mount with additional hinges and boards acting as fulcrums.  Another approach is to simply adjust the rate at which the stepper motor is driven.

The latter approach is the one to be used for this project.  A soft-clock is generated (using code borrowed from the WWVB project) to step the motor at the appropriate rate.  Control of the bi-polar stepper is accomplished by driving a dual H-bridge directly from the MSP430 pins.  

A pre-computed array of bit-patterns is stored in an array.  An index pointer (AND-ed with an appropriate constant to ensure that it does not over-run the array) is incremented at each step and added to the array base pointer before the dereferenced value is copied to the appropriate port register.
