CC = gcc
CFLAGS = -ansi -pedantic-errors -Wall -Wextra -ggdb
LIBPATH = .
# sorted linked list scheduler:
#OBJS = watch_dog.o scheduler.o dllist.o sorted_list.o priority_queue.o uid.o task.o
# heap scheduler:
OBJS = watch_dog.o private_watchdog_utils.o scheduler.o heap.o heap_pq.o dvector.o uid.o task.o
HEADER_DIR = /home/itay/git/ds/include/
SRC_DIR = /home/itay/git/ds/src/
TEST_DIR = /home/itay/git/ds/test/
PROJ_DIR = /home/itay/git/projects/

#make debug and release versions (as a client)
#all : debug release

#make shared lib, watchdog executable, client (testing) executable:
all : libwatchdog.so watchdog_exec.out user_exec.out
	
	
#####
# Shared object compiled from object files:
libwatchdog.so : $(OBJS)
	$(CC) $(CFLAGS) -shared -o $@ $(OBJS) -I $(HEADER_DIR) -I $(PROJ_DIR) -lpthread



####
# seperate executable for watchdog process:
watchdog_exec.o : $(PROJ_DIR)wd_process.c $(PROJ_DIR)private_watchdog_utils.h
	$(CC) $(CFLAGS) -c -o $@ $(PROJ_DIR)wd_process.c -I $(HEADER_DIR) -I $(PROJ_DIR)
	
watchdog_exec.out : watchdog_exec.o private_watchdog_utils.o scheduler.o heap_pq.o task.o uid.o $(HEADER_DIR)scheduler.h $(HEADER_DIR)priority_queue.h $(HEADER_DIR)task.h $(HEADER_DIR)uid.h 
	$(CC) $(CFLAGS) $(PROJ_DIR)watchdog_exec.o private_watchdog_utils.o scheduler.o heap.o heap_pq.o dvector.o uid.o task.o -o $@ -lpthread
#linked with all OBJS except "watch_dog.o"!
	
	
	
####
# client executable for testing:
user_exec.o : $(PROJ_DIR)client_app.c $(PROJ_DIR)watch_dog.o $(PROJ_DIR)watch_dog.h
	$(CC) $(CFLAGS) -c -o $@ $(PROJ_DIR)client_app.c -I $(PROJ_DIR)
	
user_exec.out : user_exec.o watchdog_exec.o libwatchdog.so $(PROJ_DIR)watch_dog.o $(PROJ_DIR)watch_dog.h
	$(CC) $(CFLAGS) $(PROJ_DIR)user_exec.o -o $@ -lwatchdog -L$(PROJ_DIR)



#####
# Src files compiled to object files:
private_watchdog_utils.o : $(PROJ_DIR)private_watchdog_utils.c scheduler.o task.o uid.o $(HEADER_DIR)scheduler.h $(HEADER_DIR)priority_queue.h $(HEADER_DIR)task.h $(HEADER_DIR)uid.h $(PROJ_DIR)private_watchdog_utils.h
	$(CC) $(CFLAGS) -fPIC -c -o $@ $(PROJ_DIR)private_watchdog_utils.c -I $(HEADER_DIR) -I $(PROJ_DIR) -lpthread


watch_dog.o : $(PROJ_DIR)watch_dog.c scheduler.o task.o uid.o $(HEADER_DIR)scheduler.h $(HEADER_DIR)priority_queue.h $(HEADER_DIR)task.h $(HEADER_DIR)uid.h $(PROJ_DIR)watch_dog.h
	$(CC) $(CFLAGS) -fPIC -c -o $@ $(PROJ_DIR)watch_dog.c -I $(HEADER_DIR) -I $(PROJ_DIR) -lpthread


scheduler.o : $(SRC_DIR)scheduler.c heap_pq.o task.o uid.o $(HEADER_DIR)scheduler.h $(HEADER_DIR)priority_queue.h $(HEADER_DIR)task.h $(HEADER_DIR)uid.h
	$(CC) $(CFLAGS) -fPIC -c -o $@ $(SRC_DIR)scheduler.c -I $(HEADER_DIR)


task.o : $(SRC_DIR)task.c uid.o $(HEADER_DIR)task.h $(HEADER_DIR)uid.h
	$(CC) $(CFLAGS) -fPIC -c -o $@ $(SRC_DIR)task.c -I $(HEADER_DIR)


uid.o : $(SRC_DIR)uid.c $(HEADER_DIR)uid.h
	$(CC) $(CFLAGS) -fPIC -c -o $@ $(SRC_DIR)uid.c -I $(HEADER_DIR)


heap.o : $(SRC_DIR)heap.c dvector.o $(HEADER_DIR)heap.h $(HEADER_DIR)dvector.h
	$(CC) $(CFLAGS) -fPIC -c -o $@ $(SRC_DIR)heap.c -I $(HEADER_DIR)


heap_pq.o : $(SRC_DIR)heap_pq.c heap.o dvector.o $(HEADER_DIR)heap.h $(HEADER_DIR)dvector.h $(HEADER_DIR)priority_queue.h
	$(CC) $(CFLAGS) -fPIC -c -o $@ $(SRC_DIR)heap_pq.c -I $(HEADER_DIR)


dvector.o : $(SRC_DIR)dvector.c $(HEADER_DIR)dvector.h
	$(CC) $(CFLAGS) -fPIC -c -o $@ $(SRC_DIR)dvector.c -I $(HEADER_DIR)


#priority_queue.o : $(SRC_DIR)priority_queue.c sorted_list.o $(HEADER_DIR)priority_queue.h $(HEADER_DIR)sorted_list.h
#	$(CC) $(CFLAGS) -fPIC -c -o $@ $(SRC_DIR)priority_queue.c -I $(HEADER_DIR)
	

#sorted_list.o : $(SRC_DIR)sorted_list.c dllist.o $(HEADER_DIR)sorted_list.h $(HEADER_DIR)dllist.h
#	$(CC) $(CFLAGS) -fPIC -c -o $@ $(SRC_DIR)sorted_list.c -I $(HEADER_DIR)


#dllist.o : $(SRC_DIR)dllist.c $(HEADER_DIR)dllist.h
#	$(CC) $(CFLAGS) -fPIC -c -o $@ $(SRC_DIR)dllist.c -I $(HEADER_DIR)






.PHONY : clean
clean :
	rm ./$(OBJS) $(PROJ_DIR)watchdog_exec.o $(PROJ_DIR)user_exec.o /dev/shm/sem.wd_sem
#	rm ./*.o




#/* gd ./projects/watch_dog.c ./ds/src/priority_queue.c ./projects/client_app.c ./ds/src/sorted_list.c ./ds/src/dllist.c ./ds/src/scheduler.c ./ds/src/task.c ./ds/src/uid.c -I ./ds/include -I ./projects/ -lpthread */

#*/ gd ./projects/watch_dog.c ./projects/wd_process.c ./ds/src/priority_queue.c ./ds/src/sorted_list.c ./ds/src/dllist.c ./ds/src/scheduler.c ./ds/src/task.c ./ds/src/uid.c -I ./ds/include -I ./projects/ -lpthread */




# fix this later:

debug : CFLAGS =
debug : CFLAGS += -ansi -pedantic-errors -Wall -Wextra -ggdb
debug : $(FN)_test.o libdatastructurelib.so
	$(CC) $(CFLAGS) -o debug.out $(FN)_test.o -L $(LIBPATH) -l watchdog

release : CFLAGS =
release : CFLAGS += -ansi -pedantic-errors -Wall -Wextra -DNDEBUG -o3
release : $(FN)_test.o libdatastructurelib.so
	$(CC) $(CFLAGS) -o release.out $(FN)_test.o -L $(LIBPATH) -l watchdog

test :
#####
# Test files compiled to object files:

$(FN)_test.o : $(TEST_DIR)$(FN)_test.c $(HEADER_DIR)$(FN).h
	$(CC) $(CFLAGS) -c -o $@ $(TEST_DIR)$(FN)_test.c -I $(HEADER_DIR)
