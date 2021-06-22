mycompiler:./src/parser.cpp ./src/scanner.c ./src/ast.cpp ./src/main.cpp
	g++ -std=c++17 -o $@ $^

./src/parser.cpp: ./src/parser.y
	bison -o $@ -d $<
	mv ./src/parser.hpp ./include/

./src/scanner.c: ./src/scanner.l ./include/parser.hpp
	flex -o $@ $<

clean:
	rm mycompiler ./src/scanner.c ./include/parser.hpp ./src/parser.cpp