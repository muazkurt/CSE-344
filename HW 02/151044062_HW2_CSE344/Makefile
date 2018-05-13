exe: bin/both.o bin/151044062_main.o
	gcc  -o exe bin/*.o -lm

bin/both.o: source/both.c
	gcc  -c source/both.c -o bin/both.o

bin/151044062_main.o: 151044062_main.c
	gcc  -c 151044062_main.c -o bin/151044062_main.o

clear:
	rm bin/* logs/* exe