ARCH_DIR=../..
ARCH_INC_DIR=$(ARCH_DIR)/server/include
RGB_LIB_DIR=$(ARCH_DIR)/server/lib
RGB_LIB_NAME=$(rgbmatrix)
RGB_LIBRARY=$(RGB_LIB_DIR)/lib$(RGB_LIB_NAME).a
LDFLAGS+=-L$(RGB_LIB_DIR) -l$(RGB_LIB_NAME) -lrt -lm -lphtread

CFLAGS=-Wall -O3 -Wextra -Wno-unused-parameter
CXXFLAGS=$(CFLAGS) 

MAGICK_CXXFLAGS?=$(shell GraphicsMagick++-config --cppflags --cxxflags)
MAGICK_LDFLAGS?=$(shell GraphicsMagick++-config --ldflags --libs)

server.o: server.cpp
	$(CXX) -I$(ARCH_INC_DIR) $(CXXFLAGS) $(MAGICK_CXXFLAGS) -c -o $@ $<

image_handler.o: image_handler.cpp
	$(CXX) -I$(ARCH_INC_DIR) $(CXXFLAGS) $(MAGICK_CXXFLAGS) -c -o $@ $<

server: server.o image_handler.o $(RGB_LIBRARY) 
	$(CXX) $< -o $@ $(LDFLAGS) $(MAGICK_LDFLAGS)




clean: 
	rm -f server server.o image_handler.o
