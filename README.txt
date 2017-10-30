Kevin Tsao
Elevators

Originally Homework #6: Concurrent Elevator Controller
for CS361: Computer Systems Architecture
Parallel computing, multithreading, C programming.

The purpose of this assignment was to create a multithreaded program that simulates passenger and elevator travel. An M-story highrise has an N-amount of elevators, each able to serve every floor. However, each elevator only has room for one passenger. A randomized set of passengers is created, and each requests an elevator immediately at the execution of the program. Since each elevator can only serve one passenger at a time, they must travel to the correct floors, pick up their assigned passengers, travel to the correct floors, and drop them off.

There is one thread per elevator, and one thread per passenger. All work is done by the controller file, which is "elevator.c". The main.c file came with the assignment and was not to be tampered in any way, however settings in the header file can be altered to test other cases such as changing the amount of elevators or passengers. Therefore all of my actual work was done in elevator.c.

While it would be trivial to manually set up the elevator assignments to remove the possibility of runtime errors such as elevators not stopping at the correct floor to pick up their passengers, this would make the program run extremely slow. The skeleton code given with the assignment accomplishes this goal, however, the actual grading was based on performance. Therefore a more sophisticated resource management system was required to make the elevators go fast. Most solutions used a two-struct solution and semaphores, mine uses a single struct implementation and multiple synchronization methods.