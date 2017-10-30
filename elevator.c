#include "elevator.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

// The struct for the elevators. In order for it to work properly, each
// instant must contain all possible information about the trips the
// elevator is going to take, and each also contains its own mutex and
// barrier to prevent multiple passengers from accessing the same
// elevator. I have also included two arrays to keep track of which floors
// each elevator will have to visit. This prevents unnecessary checks when
// elevators go past a floor they have no awaiting passengers on.
static struct Elevator {
	int passenger;
	int occupancy;
	int location;
	int destination;
	int direction;

	int pickups[FLOORS];
	int dropoff[FLOORS];

	pthread_mutex_t lock;
	pthread_barrier_t barrier;

	enum {ELEVATOR_ARRIVED = 1, ELEVATOR_OPEN = 2, ELEVATOR_CLOSED = 3} state;
} elevators[ELEVATORS];

pthread_barrier_t masterBarrier;

int work, full, choice;

void scheduler_init() {	
	int i, j;
	// Initialize all elevators to be empty.
	for (i = 0; i < ELEVATORS; i++) {
		elevators[i].passenger = 0;
		elevators[i].occupancy = 0;
		elevators[i].location = 0;
		elevators[i].destination = 0;
		elevators[i].direction = -1;
		elevators[i].state = ELEVATOR_ARRIVED;
		pthread_mutex_init(&elevators[i].lock, 0);
		pthread_barrier_init(&elevators[i].barrier, NULL, 2);
	
		// All elevators currently have no work to do.
		for (j = 0; j < FLOORS; j++) {
			elevators[i].pickups[j] = 0;
			elevators[i].dropoff[j] = 0;
		}
	}

	full = choice = 0;
	work = (PASSENGERS / ELEVATORS) + 1;

	// Set up a barrier here so that elevators don't start operation
	// until all of them are set up.
	pthread_barrier_init(&masterBarrier, NULL, PASSENGERS);
}

// The request function. When a passenger thread requests an elevator thread,
// the elevator must (eventually) reach the floor the passenger is on and then
// have the passenger board successfully. Since each elevator by default can
// only house one passenger, this must not happen until the elevator is empty,
// and in order to meet performance requirements, the elevator should pick up
// the closest awaiting passengers and not skip over nearby passengers.
void passenger_request(int passenger, int from_floor, int to_floor, void (*enter)(int, int), void(*exit)(int, int))
{	
	// Wait for the elevator to arrive at our origin floor, then get in
	int waiting = 1;
	
	// Let all elevators have at least one request made of them first
	// before the first elevator starts moving.
	int current;
	if (full < work) {
		current = choice;
		full++;
	}
	else { 
		current = choice + 1;
		full = 1;
		choice++;
	}
	
	// Update elevator information once each passenger has
	// made their request.
	elevators[current].passenger++;
	elevators[current].pickups[from_floor]++;
	elevators[current].dropoff[to_floor]++;
	pthread_barrier_wait(&masterBarrier);

	// While a passenger is 'waiting', the thread is sitting
	// idle at the floor until the elevator thread assigned to
	// it reaches that floor in the right condition. To check
	// for this condition I use an if conditional and check
	// against the elevator structs.
	while(waiting) {
		// Set up our barriers so that no more than one
		// passenger at a time attempts to get into an
		// elevator.
		pthread_barrier_wait(&elevators[current].barrier);
		pthread_mutex_lock(&elevators[current].lock);
	
		// Check the elevator struct to ensure that all
		// conditions are correct before actually boarding.
		if ((elevators[current].location == from_floor) && (elevators[current].state == ELEVATOR_OPEN) && (elevators[current].occupancy == 0)) {
			// Update the elevator struct to reflect
			// that it is now occupied.
			elevators[current].occupancy++;
			elevators[current].destination = to_floor;
			enter(passenger, current);
			waiting = 0;
			elevators[current].pickups[from_floor]--;
		}

		// Once a passenger has successfully boarded, OR if
		// an elevator passes up that passenger because they
		// are not the correct one, remove barriers.
		pthread_mutex_unlock(&elevators[current].lock);
		pthread_barrier_wait(&elevators[current].barrier);
	}

	// Wait for the elevator at our destination floor, then get out
	int riding=1;
	
	// While an elevator is occupied, it has to check each floor that
	// it passes until it gets to the correct floor that the current
	// housed passenger wants to get out at.
	while(riding) {
		// Set up our barriers to that multiple elevators don't
		// stop at the same floor and cause errors.
		pthread_barrier_wait(&elevators[current].barrier);
		pthread_mutex_lock(&elevators[current].lock);

		// Only release passenger if this is the correct
		// floor and the elevator is open.
		if ((elevators[current].location == to_floor) && (elevators[current].state == ELEVATOR_OPEN)) {
			elevators[current].passenger--;
			elevators[current].occupancy--;
			elevators[current].destination = 0;
			exit(passenger, current);
			riding = 0;
			elevators[current].dropoff[to_floor]--;
		}

		pthread_mutex_unlock(&elevators[current].lock);
		pthread_barrier_wait(&elevators[current].barrier);
	}
}

// The function to control the elevator behavior. There are multiple
// errors that must be prevented: an elevator opening when there is
// already a passenger inside, an elevator not moving after picking up
// a passenger, an elevator not moving after dropping off a passenger,
// etc. All elevators should be assigned a near-equal amount of
// passengers to divide the work, so the program cannot exit until
// every single passenger has completed their trips - if even a single
// elevator bugs out on one trip, at least one passenger will be left
// waiting forever.
void elevator_ready(int elevator, int at_floor, void(*move_direction)(int, int), void(*door_open)(int), void(*door_close)(int)) {
	pthread_mutex_lock(&elevators[elevator].lock);

	// If the elevator has no more assigned passengers left,
	// terminate the thread.
	if (elevators[elevator].passenger == 0) {
		int thread = pthread_self();
		pthread_exit(&thread);
	}


	// Only open the doors if the elevator has arrived at the
	// exactly correct floor.
	if (((elevators[elevator].state == ELEVATOR_ARRIVED) && 
	(((elevators[elevator].occupancy == 0) && (elevators[elevator].pickups[at_floor] != 0)) || 
	((elevators[elevator].occupancy != 0) && (elevators[elevator].dropoff[at_floor] != 0)))) && 
	((elevators[elevator].destination == at_floor) || (elevators[elevator].destination == 0))) {
		door_open(elevator);
		elevators[elevator].state = ELEVATOR_OPEN;

		// Set up our barriers so that elevators don't
		// close their doors until the passenger enters.
		pthread_mutex_unlock(&elevators[elevator].lock);
		pthread_barrier_wait(&elevators[elevator].barrier);
		pthread_barrier_wait(&elevators[elevator].barrier);
		return;
	}
	// Now close the elevator doors.
	else if (elevators[elevator].state == ELEVATOR_OPEN) {
		door_close(elevator);
		elevators[elevator].state = ELEVATOR_CLOSED;
	}
	// If the elevator reaches the bottom or top floor, reverse
	// direction. Otherwise, keep moving infinitely as long as
	// the elevator still has assigned passengers left.
	else {
		if (at_floor == 0 || at_floor == FLOORS - 1) { 
			elevators[elevator].direction*=-1;
		}
		move_direction(elevator, elevators[elevator].direction);
		elevators[elevator].location = at_floor + elevators[elevator].direction;
		elevators[elevator].state = ELEVATOR_ARRIVED;
	}
	pthread_mutex_unlock(&elevators[elevator].lock);
}
