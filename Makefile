CXXFLAGS= -std=c++11 -Wall -Wextra -O3 -ffast-math

.PHONY: test

test: exp_test
	./exp_test | tee results
	@echo -----------------------
	sort -rn results

exp_256.h: make_table.awk
	awk -f make_table.awk > exp_256.h

exp_test: exp_test.cc exp_256.h 
	$(CXX) -o $@ $< $(CXXFLAGS)

clean:
	rm -f *.o exp_256.h exp_test




	
