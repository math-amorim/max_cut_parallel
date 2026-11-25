CXX = mpic++

CXXFLAGS = -O3 -fopenmp -Wall

TARGET = main
SOURCES = main_paralelo.cpp

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CXX) $(CXXFLAGS) $(SOURCES) -o $(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: all clean
