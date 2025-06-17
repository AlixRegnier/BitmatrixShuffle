FLAGS=-Wall -pedantic -O3
CPPFLAG=-std=c++17
SIMD=-msse2

all: reorder reverse_reorder


fast_median.o: fast_median.cpp fast_median.h
	g++ $(FLAGS) $(CPPFLAG) -I. -c fast_median.cpp

distance_matrix.o: distance_matrix.cpp distance_matrix.h
	g++ $(FLAGS) $(CPPFLAG) -I. -c distance_matrix.cpp

rng.o: rng.cpp rng.h
	g++ $(FLAGS) $(CPPFLAG) -I. -c rng.cpp

reorder: reorder.cpp reorder.h bitpermute.o rng.o distance_matrix.o fast_median.o vptree.h vptree_impl.h
	g++ $(FLAGS) $(CPPFLAG) $(SIMD) -I. -o reorder reorder.cpp distance_matrix.o rng.o fast_median.o bitpermute.o

reverse_reorder: reverse_reorder.cpp reverse_reorder.h bitpermute.o
	g++ $(FLAGS) $(CPPFLAG) $(SIMD) -I. -o reverse_reorder reverse_reorder.cpp bitpermute.o

bitpermute.o: bitpermute.cpp bitpermute.h
	g++ $(FLAGS) $(CPPFLAG) -I. -c bitpermute.cpp
	
clean:
	rm -rf *.o
