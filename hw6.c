#include "hw6.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

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
	for (i = 0; i < ELEVATORS; i++) {
		elevators[i].passenger = 0;
		elevators[i].occupancy = 0;
		elevators[i].location = 0;
		elevators[i].destination = 0;
		elevators[i].direction = -1;
		elevators[i].state = ELEVATOR_ARRIVED;
		pthread_mutex_init(&elevators[i].lock, 0);
		pthread_barrier_init(&elevators[i].barrier, NULL, 2);
	
		for (j = 0; j < FLOORS; j++) {
			elevators[i].pickups[j] = 0;
			elevators[i].dropoff[j] = 0;
		}
	}

	full = choice = 0;
	work = (PASSENGERS / ELEVATORS) + 1;

	pthread_barrier_init(&masterBarrier, NULL, PASSENGERS);
}


void passenger_request(int passenger, int from_floor, int to_floor, void (*enter)(int, int), void(*exit)(int, int))
{	
	// wait for the elevator to arrive at our origin floor, then get in
	int waiting = 1;
	
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
	
	elevators[current].passenger++;
	elevators[current].pickups[from_floor]++;
	elevators[current].dropoff[to_floor]++;
	pthread_barrier_wait(&masterBarrier);

	while(waiting) {
		pthread_barrier_wait(&elevators[current].barrier);
		pthread_mutex_lock(&elevators[current].lock);
	
		if ((elevators[current].location == from_floor) && (elevators[current].state == ELEVATOR_OPEN) && (elevators[current].occupancy == 0)) {
			elevators[current].occupancy++;
			elevators[current].destination = to_floor;
			enter(passenger, current);
			waiting = 0;
			elevators[current].pickups[from_floor]--;
		}

		pthread_mutex_unlock(&elevators[current].lock);
		pthread_barrier_wait(&elevators[current].barrier);
	}

	// wait for the elevator at our destination floor, then get out
	int riding=1;
	while(riding) {
		pthread_barrier_wait(&elevators[current].barrier);
		pthread_mutex_lock(&elevators[current].lock);

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

void elevator_ready(int elevator, int at_floor, void(*move_direction)(int, int), void(*door_open)(int), void(*door_close)(int)) {
	pthread_mutex_lock(&elevators[elevator].lock);

	if (elevators[elevator].passenger == 0) {
		int thread = pthread_self();
		pthread_exit(&thread);
	}

	if (((elevators[elevator].state == ELEVATOR_ARRIVED) && 
	(((elevators[elevator].occupancy == 0) && (elevators[elevator].pickups[at_floor] != 0)) || 
	((elevators[elevator].occupancy != 0) && (elevators[elevator].dropoff[at_floor] != 0)))) && 
	((elevators[elevator].destination == at_floor) || (elevators[elevator].destination == 0))) {
		door_open(elevator);
		elevators[elevator].state = ELEVATOR_OPEN;

		pthread_mutex_unlock(&elevators[elevator].lock);
		pthread_barrier_wait(&elevators[elevator].barrier);
		pthread_barrier_wait(&elevators[elevator].barrier);
		return;
	}
	else if (elevators[elevator].state == ELEVATOR_OPEN) {
		door_close(elevator);
		elevators[elevator].state = ELEVATOR_CLOSED;
	}
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
