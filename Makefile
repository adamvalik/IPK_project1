EXEC = ipk24chat-client
SRC = $(wildcard *.cpp)
OBJ = $(patsubst %.cpp,%.o,$(SRC))

CPP = g++
CPPFLAGS = -std=c++20

.PHONY: all clean doc

.DEFAULT_GOAL := all

all: $(EXEC)

$(EXEC): $(OBJ)
	$(CPP) $(CPPFLAGS) -o $@ $^

%.o: %.cpp
	$(CPP) $(CPPFLAGS) -c $< -o $@

clean:
	rm -f *.o $(EXEC)

pack: clean
	zip -v -r xvalik05.zip *.cpp *.hpp Makefile README.md CHANGELOG.md LICENSE IPKClient.jpeg Doxyfile

doc: 
	doxygen Doxyfile



