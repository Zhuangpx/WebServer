CXX = g++
CFLAGS = -std=c++14 -O2 -Wall -g
CXXFLAGS = -std=c++14 -O2 -Wall -g

TARGET:=server
# SOURCE:=
OBJS = ../Include/Utils/*.cpp \
		   ../Include/Pool/*.cpp \
			 ../Include/Log/*.cpp \
			 ../Include/Http/*.cpp \
			 ../Source/*.cpp

all: $(OBJS)
		$(CXX) $(CFLAGS) $(OBJS) -o ../Bin/$(TARGET) -pthread -lmysqlclient

clean:
		rm -rf ../Bin/$(OBJS) $(TARGET)
