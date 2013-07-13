CC = gcc -g
OBJS = token.o ast.o sym.o type.o loc.o scanner.o
all: parser1 parser2 scanner

parser1: parser1.o $(OBJS)
	$(CC) -o $@ parser1.o $(OBJS)

parser2: parser2.o $(OBJS)
	$(CC) -o $@ parser2.o $(OBJS)

parser1.o : parser1.c token.h ast.h sym.h type.h
	$(CC) -DTEST_PARSER -c parser1.c

parser2.o : parser2.c token.h ast.h sym.h type.h
	$(CC) -DTEST_PARSER -c parser2.c

scanner : scanner.c token.o
	$(CC) -DTEST_SCANNER $(CFLAGS) -o $@ scanner.c token.o

ast.o : ast.c ast.h token.h type.h sym.h loc.h type.h
sym.o : sym.c sym.h type.h loc.h type.h
loc.o : loc.c loc.h type.h
type.o : type.c type.h 
token.o : token.c token.h
scanner.o : scanner.c token.h

.PHONY: test
test: parser1 parser2
	@echo "Inside block(start=block)"
	@cat -n ./test/test01.txt
	@echo "------------"
	@./parser1 < test/test01.txt
	@echo "------------"
	@echo "Outside block(start=prog)"
	@cat -n ./test/test02.txt
	@echo "------------"
	@./parser2 < test/test02.txt
	@echo "------------"

clean:
	-rm scanner parser? *.o out? core*
