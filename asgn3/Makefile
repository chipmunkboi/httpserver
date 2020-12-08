OBJECTS				= httpserver.o
CXX					= g++
REMOVE				= rm -f
CXXFLAGS			= -std=gnu++11 -Wall -Wextra -Wpedantic -Wshadow -pthread
_submit_CXXFLAGS	= -std=gnu++11 -Wall -Wextra -Wpedantic -Wshadow -pthread

httpserver : $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^

clean :
	$(REMOVE) $(OBJECTS)

spotless :
	$(REMOVE) $(OBJECTS) httpserver

memcheck :
	valgrind --tool=memcheck --leak-check=yes ./httpserver