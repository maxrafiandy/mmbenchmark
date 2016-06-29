PCM_DIR = /usr/local/src/pcm
LINKER += -I/usr/local/src/pcm 
LINKER += -L/usr/local/src/pcm -L/usr/local/lib 
LINKER += -lpcm -lpthread

CXXFLAGS = -std=c++11 -Wall -O3 -msse4.2 -mavx2 -mfma -fopenmp

MACRO += -D WITH_COUNTER
MACRO += -D PARALLEL_ONLY
#MACRO += -D SERIAL_ONLY

mmbench.x: main.cpp mm.h counter.h
	g++ $(CXXFLAGS) -ommbench.x main.cpp $(MACRO) $(LINKER)
		
install:
	cp mmbench.x /usr/local/bin

uninstall:
	rm /usr/local/bin/mmbench.x

clean:
	rm *.x
