FLAGS=-Wall -pedantic -O3 -march=native
CPPFLAG=-std=c++17

all: bitmatrixshuffle reverse_bitmatrixshuffle


fast_median.o: fast_median.cpp fast_median.h
	g++ $(FLAGS) $(CPPFLAG) -I. -c fast_median.cpp

distance_matrix.o: distance_matrix.cpp distance_matrix.h
	g++ $(FLAGS) $(CPPFLAG) -I. -c distance_matrix.cpp

rng.o: rng.cpp rng.h
	g++ $(FLAGS) $(CPPFLAG) -I. -c rng.cpp

bitmatrixshuffle: reorder.cpp reorder.h bitpermute.o rng.o distance_matrix.o fast_median.o vptree.h vptree_impl.h threadpool.o
	g++ $(FLAGS) $(CPPFLAG) -I. -I./cxxopts/include -I./json/single_include -o bitmatrixshuffle reorder.cpp distance_matrix.o rng.o fast_median.o bitpermute.o threadpool.o

reverse_bitmatrixshuffle: reverse_reorder.cpp reverse_reorder.h bitpermute.o
	g++ $(FLAGS) $(CPPFLAG) -I. -I./cxxopts/include -I./json/single_include -o reverse_bitmatrixshuffle reverse_reorder.cpp bitpermute.o

bitpermute.o: bitpermute.cpp bitpermute.h
	g++ $(FLAGS) $(CPPFLAG) -I. -c bitpermute.cpp

threadpool.o: threadpool.cpp threadpool.h
	g++ $(FLAGS) $(CPPFLAG) -I. -c threadpool.cpp
	
clean:
	rm -rf *.o
