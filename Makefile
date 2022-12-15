all: front back cpu dirs

dirs: dump bin 

dump:
	mkdir -p dump

bin:
	mkdir -p bin

front:
	cd frontend && make

back:
	cd backend && make

cpu:
	cd cpu && make all

clean:
	find . -name "*.cpp.o" -delete && find . -name "*.cpp.d" -delete