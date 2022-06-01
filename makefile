seamcarving : stub.o 
	g++ stub.o -o seamcarving

stub.o : stub.cpp 
	g++ -I/usr/include/eigen3/ -O3 -c stub.cpp 


clean:
	rm *.o *.ppm *.exe output out
	 
