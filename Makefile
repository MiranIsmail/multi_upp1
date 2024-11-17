all: qsort gauss
		./gaus && ./qsmm

qsort:
			gcc -O2 -o qsmm qsm.c -lpthread
			./qsmm

gauss:
			gcc -O2 -o gaus gaussian_multi.c -lpthread
			./gaus 
clean:
			rm qsmm && rm gaus
