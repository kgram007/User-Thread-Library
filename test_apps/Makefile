
all:
	gcc -o test_create_join test_create_join.c -L. -luthread -lrt
	gcc -o test_periodic test_periodic.c -L. -luthread -lrt
	gcc -o test_lock test_lock.c -L. -luthread -lrt
	gcc -o test_srp test_srp.c -L. -luthread -lrt
	gcc -o test_deadlock test_deadlock.c -L. -luthread -lrt

clean:
	rm -f test_create_join test_periodic test_lock test_srp test_deadlock
