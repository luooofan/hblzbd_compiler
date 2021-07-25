vpath %.cpp src
vpath %.cpp src/Pass

CC := clang++
CPPFLAGS := -std=c++17 -O2
SOURCES := $(wildcard src/*.cpp src/Pass/*.cpp)
OBJECTS := $(patsubst %.cpp,build/%.o,$(notdir $(SOURCES)))
DEPENDS := $(patsubst %.o,%.d,$(OBJECTS))
TARGETS := compiler
$(shell mkdir -p build)

.PHONY:all
all: $(TARGETS)
	@echo Build Successfully.

$(TARGETS):$(OBJECTS)
	$(CC) $(CPPFLAGS) -lm  $^ -o $@

src/parser.cpp: src/parser.y
	bison -o $@ -d $< 

src/scanner.cpp: src/scanner.l
	flex -o $@ $< 

build/%.o: %.cpp
	$(CC) $(CPPFLAGS) -c $< -o $@

-include $(DEPENDS)

build/%.d: %.cpp
	@set -e; rm -f $@; \
	$(CC) -MM $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,build/\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

.PHONY:clean
clean:
	rm -f build/*