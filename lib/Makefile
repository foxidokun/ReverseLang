lib: loglib filelib listlib
	ld -relocatable obj/* -o library.so
	
loglib:
	g++ -c -o obj/liblog.o log.cpp

filelib:
	g++ -c -o obj/libfile.o file.cpp

listlib:
	cd list && make as_lib
