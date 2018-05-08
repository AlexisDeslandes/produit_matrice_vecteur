deslandes.o: deslandes.c
	mpicc -fopenmp -Wall deslandes.c -lm -o deslandes.o

clean:
	rm *.o
	rm resultat
