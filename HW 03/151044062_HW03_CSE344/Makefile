exe: main ls pwd

main: main.o
	gcc -o main main.o

main.o: main.c
	gcc -c main.c

ls: ls.o
	gcc -o qls ls.o

ls.o: ls.c
	gcc -c ls.c

pwd: pwd.o
	gcc -o qpwd pwd.o

pwd.o: pwd.c
	gcc -c pwd.c

clear:
	rm ./qls ./main ./qpwd *.o