SRC = SqlParser.tab.c lex.sql.c SqlEngine.cc BTreeIndex.cc BTreeNode.cc RecordFile.cc PageFile.cc 
MAINSRC = main.cc
TESTSRC = test.cc
HDR = Bruinbase.h PageFile.h SqlEngine.h BTreeIndex.h BTreeNode.h RecordFile.h SqlParser.tab.h

bruinbase: $(MAINSRC) $(SRC) $(HDR)
	g++ -ggdb -o $@ $(MAINSRC) $(SRC)

test: $(TESTSRC) $(SRC) $(HDR)
	g++ -std=c++11 -ggdb -o $@ $(TESTSRC) $(SRC)

lex.sql.c: SqlParser.l
	flex -Psql $<

SqlParser.tab.c: SqlParser.y
	bison -d -psql $<

clean:
	rm -f bruinbase bruinbase.exe test *.o *~ lex.sql.c SqlParser.tab.c SqlParser.tab.h 
