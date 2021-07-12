mycompiler:./src/parser.cpp ./src/scanner.c ./src/ast.cpp ./src/main.cpp ./src/generate_ir.cpp ./src/ir.cpp ./src/evaluate.cpp ./src/ir_struct.cpp ./src/arm_struct.cpp
	g++ -std=c++17 -o $@ $^

./src/parser.cpp: ./src/parser.y
	bison -o $@ -d $<

./src/scanner.c: ./src/scanner.l
	flex -o $@ $<

clean:
	rm mycompiler ./src/scanner.c ./src/parser.hpp ./src/parser.cpp