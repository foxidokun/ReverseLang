all: front back

front:
	cd frontend && make

back:
	cd backend && make

clean:
	find . -name "*.cpp.o" -delete && find . -name "*.cpp.d" -delete