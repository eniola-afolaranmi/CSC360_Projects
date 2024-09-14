# Multi-Threaded Train Simulation

This program simulates a multi-threaded train system with the following priorities:

- If two loaded trains have the same priority:
  a. If they are both traveling in the same direction, the train which finished loading first gets the clearance to cross first. If they finished loading at the same time, the one that appeared first in the input file gets the clearance to cross first.
  b. If they are traveling in opposite directions, pick the train which will travel in the direction opposite of which the last train to cross the main track traveled. If no trains have crossed the main track yet, the Westbound train has the priority.
  c. If there are three trains in the same direction traveling through the main track back to back, the trains waiting in the opposite direction get a chance to dispatch one train if any.

## Building and Running the Program

To build the program, use the following command:

```bash
gcc mts.c -lpthread -o mts

To run the program, provide an input file as an argument, like this:
./mts input.txt
