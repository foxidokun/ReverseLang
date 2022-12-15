all: front back cpu

front:
	cd frontend && make

back:
	cd backend && make

cpu:
	cd cpu && make all

clean:
	find . -name "*.cpp.o" -delete && find . -name "*.cpp.d" -delete