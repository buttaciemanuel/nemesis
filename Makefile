CC=clang++
FLAGS=-std=c++17 -ggdb3 -Wall -pedantic-errors -D __NEMESIS_COLORIZE__=1
INCLUDE=include
SOURCES=src/driver.cpp src/diagnostic.cpp src/source.cpp src/span.cpp src/token.cpp src/tokenizer.cpp src/ast.cpp src/parser.cpp src/type.cpp src/checker.cpp src/evaluator.cpp src/pattern_matcher.cpp src/type_matcher.cpp src/code_generator.cpp src/pm.cpp
OBJECTS=build/driver.o build/diagnostic.o build/source.o build/span.o build/token.o build/tokenizer.o build/ast.o build/parser.o build/type.o build/checker.o build/evaluator.o build/pattern_matcher.o build/type_matcher.o build/code_generator.o build/pm.o
LIBS=-lzip -lcurl

build/driver: $(OBJECTS)
	$(CC) $(FLAGS) $(OBJECTS) -o build/driver $(LIBS)

build/driver.o: src/driver.cpp $(INCLUDE)/nemesis/driver/*.hpp
	$(CC) $(FLAGS) -I $(INCLUDE) -c src/driver.cpp -o build/driver.o

build/diagnostic.o: src/diagnostic.cpp $(INCLUDE)/nemesis/diagnostics/*.hpp
	$(CC) $(FLAGS) -I $(INCLUDE) -c src/diagnostic.cpp -o build/diagnostic.o

build/source.o: src/source.cpp $(INCLUDE)/nemesis/source/*.hpp
	$(CC) $(FLAGS) -I $(INCLUDE) -c src/source.cpp -o build/source.o

build/span.o: src/span.cpp $(INCLUDE)/nemesis/utf8/*.hpp
	$(CC) $(FLAGS) -I $(INCLUDE) -c src/span.cpp -o build/span.o

build/token.o: src/token.cpp $(INCLUDE)/nemesis/tokenizer/*.hpp
	$(CC) $(FLAGS) -I $(INCLUDE) -c src/token.cpp -o build/token.o

build/tokenizer.o: src/tokenizer.cpp $(INCLUDE)/nemesis/tokenizer/*.hpp
	$(CC) $(FLAGS) -I $(INCLUDE) -c src/tokenizer.cpp -o build/tokenizer.o

build/ast.o: src/ast.cpp $(INCLUDE)/nemesis/parser/*.hpp
	$(CC) $(FLAGS) -I $(INCLUDE) -c src/ast.cpp -o build/ast.o

build/parser.o: src/parser.cpp $(INCLUDE)/nemesis/parser/*.hpp
	$(CC) $(FLAGS) -I $(INCLUDE) -c src/parser.cpp -o build/parser.o

build/type.o: src/type.cpp $(INCLUDE)/nemesis/analysis/*.hpp
	$(CC) $(FLAGS) -I $(INCLUDE) -c src/type.cpp -o build/type.o

build/checker.o: src/checker.cpp $(INCLUDE)/nemesis/analysis/*.hpp
	$(CC) $(FLAGS) -I $(INCLUDE) -c src/checker.cpp -o build/checker.o

build/evaluator.o: src/evaluator.cpp $(INCLUDE)/nemesis/analysis/*.hpp $(INCLUDE)/utils/*.hpp
	$(CC) $(FLAGS) -I $(INCLUDE) -c src/evaluator.cpp -o build/evaluator.o

build/pattern_matcher.o: src/pattern_matcher.cpp $(INCLUDE)/nemesis/analysis/*.hpp
	$(CC) $(FLAGS) -I $(INCLUDE) -c src/pattern_matcher.cpp -o build/pattern_matcher.o

build/type_matcher.o: src/type_matcher.cpp $(INCLUDE)/nemesis/analysis/*.hpp
	$(CC) $(FLAGS) -I $(INCLUDE) -c src/type_matcher.cpp -o build/type_matcher.o

build/code_generator.o: src/code_generator.cpp $(INCLUDE)/nemesis/codegen/*.hpp
	$(CC) $(FLAGS) -I $(INCLUDE) -c src/code_generator.cpp -o build/code_generator.o

build/pm.o: src/pm.cpp $(INCLUDE)/nemesis/pm/*.hpp
	$(CC) $(FLAGS) -I $(INCLUDE) -c src/pm.cpp -o build/pm.o

clean:
	rm -rf build/*
