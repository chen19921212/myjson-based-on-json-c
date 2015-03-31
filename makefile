all: myjson.cpp
	g++ myjson.cpp myjson.h -ojson -ljson -I/usr/local/include/json/ -L/usr/local/lib/

