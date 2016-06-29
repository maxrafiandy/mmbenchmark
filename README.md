# Matrix-multiplication Benchmark

Depencies:
	Intel Performance Monitor

	Compile the Intel Performance Monitor. 
	create and link the common used Intel-pcm libray : 
		# ar cr libpcm.a utils.o pci.o client_bw.o msr.o cpucounters.o
		# ln -s libpcm.a /usr/local/lib/libpcm.so

# How To Install
	Open the Makefile on the root of source directory. Making some costumizing for your need.
	If you want to profiling some the CPU resource, uncomment line MACRO += -D WITH_COUNTER.
	If you want to run the benchmark on sequence algoritm, uncomment line MACRO += -D SERIAL_ONLY,
	But if you want to run the benchmark on parallel algoritm, uncomment line MACRO += -D PARALLEL_ONLY.
	Comment it both if you need speedup in result
	
	Just run make on main directory of the source.
