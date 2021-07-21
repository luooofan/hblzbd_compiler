compiler:./src/parser.cpp ./src/scanner.c ./src/ast.cpp ./src/main.cpp ./src/generate_ir.cpp ./src/ir.cpp ./src/evaluate.cpp ./src/ir_struct.cpp ./src/construct_ir_module.cpp ./src/arm.cpp ./src/arm_struct.cpp ./src/Pass/generate_arm.cpp ./src/Pass/arm_liveness_analysis.cpp ./src/Pass/allocate_register.cpp
	g++ -std=c++17 -o $@ $^

./src/parser.cpp: ./src/parser.y
	bison -o $@ -d $<

./src/scanner.c: ./src/scanner.l
	flex -o $@ $<

clean:
	rm compiler ./src/scanner.c ./src/parser.hpp ./src/parser.cpp