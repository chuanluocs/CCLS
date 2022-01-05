CFLAGS = -O3 -static -m64
#CFLAGS = -m64 -O3 -static -fno-strict-aliasing -funroll-all-loops -fexpensive-optimizations
TARGET = CCLS

all: ${TARGET} clean

${TARGET}: basic.h main.cpp
	g++ ${CFLAGS} main.cpp -o ${TARGET}

clean:
	rm -f *~

cleanup:
	rm -f ${TARGET}
	rm -f *~
