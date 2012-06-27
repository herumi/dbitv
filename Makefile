CC				= g++
CFLAGS		+= -D__USE_SSE_POPCNT__ -O3 -std=gnu++0x -fomit-frame-pointer -fstrict-aliasing -fno-operator-names -g
CFLAGS		+= -floop-optimize -march=native
WFLAGS		= -Wall
LDFLAGS		= -L/usr/local/lib
INCLUDE		= -I./include -I$(HOME)/local/include/ -I../opti -I../xbyak/
LIBS			= -msse4 -L$(HOME)/local/lib
SRCS			= test/run_query.cpp
OBJS			= $(subst .cpp,.o,$(SRCS))
BENCHMARK	= run_query 

# For gtest
GTEST_DIR			= .utest/gtest-1.6.0
CPPFLAGS			+= -I$(GTEST_DIR)/include -I$(GTEST_DIR)
GTEST_HEADERS	= $(GTEST_DIR)/include/gtest/*.h $(GTEST_DIR)/include/gtest/internal/*.h
GTEST_SRCS		= $(GTEST_DIR)/src/*.cc $(GTEST_DIR)/src/*.h $(GTEST_HEADERS)
SRCS_UTEST		= test/SuccinctBitVector_utest.cpp
OBJS_UTEST		= $(subst .cpp,.o,$(SRCS_UTEST))
SBV_UTEST			= SBVUTest

.PHONY:bench
bench:		$(BENCHMARK)
test/run_query.o: include/SuccinctBitVector.hpp ../opti/rank.hpp
$(BENCHMARK):	$(OBJS)
		$(CC) $(CFLAGS) $(WFLAGS) $(OBJS) $(INCLUDE) $(LDFLAGS) $(LIBS) -o $@

.cpp.o:
		$(CC) $(CPPFLAGS) $(CFLAGS) $(WFLAGS) $(INCLUDE) $(LDFLAGS) -c $< -o $@

.PHONY:utest
utest:		$(OBJS_UTEST) gtest_main.a
		$(CC) $(CPPFLAGS) $(CFLAGS) $(INCLUDE) $(LDFLAGS) $(LIBS) -lpthread $^ -o $(SBV_UTEST)

gtest-all.o:	$(GTEST_SRCS)	
		$(CC) $(CPPFLAGS) -c $(GTEST_DIR)/src/gtest-all.cc

gtest_main.o:	$(GTEST_SRCS)
		$(CC) $(CPPFLAGS) $(CFLAGS) -c $(GTEST_DIR)/src/gtest_main.cc

gtest.a:	gtest-all.o
		$(AR) $(ARFLAGS) $@ $^

gtest_main.a:	gtest-all.o gtest_main.o
		$(AR) $(ARFLAGS) $@ $^
		
.PHONY:clean
clean:
		rm -f *.log *.o *.a $(OBJS) $(OBJS_UTEST) $(BENCHMARK) $(SBV_UTEST)
