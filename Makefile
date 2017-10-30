SCHEDULER=elevator.c
BINARIES=elevator
CCOPT=-g --std=gnu99
LIBS=-lpthread

all: $(BINARIES)

elevator: main.c elevator.h $(SCHEDULER)
	gcc $(CCOPT) main.c $(SCHEDULER) -o elevator $(LIBS)

burn-in: clean elevator.c
	gcc $(CCOPT) -DDELAY=1000 -DLOG_LEVEL=5 -DPASSENGERS=100 -DTRIPS_PER_PASSENGER=100 -DELEVATORS=10 -DFLOORS=40 main.c $(SCHEDULER) -o elevator $(LIBS)
	./elevator

test: clean $(SCHEDULER)
	@echo "\n\nNOTE: Logs from stderr stored in /tmp/logs.tmp\n\n"
	@sleep 1
	gcc -DDELAY=10000 -DLOG_LEVEL=9 -DPASSENGERS=10 -DELEVATORS=1 -DFLOORS=5 -g --std=c99 main.c $(SCHEDULER) -o elevator $(LIBS) $(CCOPT)
	@./elevator 2> /tmp/logs.tmp
	@cat /tmp/logs.tmp | egrep -v Passenger > /tmp/testresults
	@cat /tmp/logs.tmp | awk '/trip duration/{sum+=$$5;count++;if(max<$$5){max=$$5}} END {printf "Mean time: %.2f max: %.2f\n",sum/count,max}' >> /tmp/testresults

	@gcc -DDELAY=10000 -DLOG_LEVEL=6 -DFLOORS=2 -g --std=c99 main.c $(SCHEDULER) -o elevator $(LIBS) $(CCOPT)
	@./elevator 2> /tmp/logs.tmp
	@cat /tmp/logs.tmp | egrep -v Passenger >> /tmp/testresults
	@cat /tmp/logs.tmp | awk '/trip duration/{sum+=$$5;count++;if(max<$$5){max=$$5}} END {printf "Mean time: %.2f max: %.2f\n",sum/count,max}' >> /tmp/testresults

	@gcc -DDELAY=10000 -DELEVATORS=10 -DLOG_LEVEL=6 -g --std=c99 main.c $(SCHEDULER) -o elevator $(LIBS) $(CCOPT)
	@./elevator 2> /tmp/logs.tmp
	@cat /tmp/logs.tmp | egrep -v Passenger >> /tmp/testresults
	@cat /tmp/logs.tmp | awk '/trip duration/{sum+=$$5;count++;if(max<$$5){max=$$5}} END {printf "Mean time: %.2f max: %.2f\n",sum/count,max}' >> /tmp/testresults
	cat /tmp/testresults


clean:
	@rm -rf *~ *.dSYM $(BINARIES)

