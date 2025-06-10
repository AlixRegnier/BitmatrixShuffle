FLAGS=-Wall -pedantic -O3 -g
CPPFLAG=-std=c++17
SIMD=-msse2

all: reorder reverse_reorder


fast_median: fast_median.cpp fast_median.h
	g++ $(FLAGS) $(CPPFLAG) -I. -c fast_median.cpp

distance_matrix: distance_matrix.cpp distance_matrix.h
	g++ $(FLAGS) $(CPPFLAG) -I. -c distance_matrix.cpp

rng: rng.cpp rng.h
	g++ $(FLAGS) $(CPPFLAG) -I. -c rng.cpp

reorder: reorder.cpp reorder.h bitpermute gray bytewrapper rng distance_matrix fast_median vptree.h vptree_impl.h
	g++ $(FLAGS) $(CPPFLAG) $(SIMD) -I. -o reorder reorder.cpp distance_matrix.o rng.o fast_median.o bitpermute.o gray.o bytewrapper.o

reverse_reorder: reverse_reorder.cpp bitpermute reverse_reorder.h
	g++ $(FLAGS) $(CPPFLAG) $(SIMD) -I. -o reverse_reorder reverse_reorder.cpp bitpermute.o

bitpermute: bitpermute.cpp bitpermute.h
	g++ $(FLAGS) $(CPPFLAG) -I. -c bitpermute.cpp

gray: bytewrapper gray.cpp gray.h
	g++ $(FLAGS) $(CPPFLAG) -I. -c gray.cpp

bytewrapper: bytewrapper.cpp bytewrapper.h
	g++ $(FLAGS) $(CPPFLAG) -I. -c bytewrapper.cpp
	
clean:
	rm -rf *.o
