#
# $Id: Makefile,v 1.1 2005/12/05 02:07:19 fzb Exp $
#

CXX 		= g++
CXXFLAGS 	= -Wall -g #-DDEBUG
DEPEND 		= makedepend -Y -- $(CFLAGS) --

SRCS 		= Instance.cc Gore.cc ActivityImpl.cc
TEST_SRCS	= test.cc verification.cc experiment.cc

OBJS 		= $(SRCS:%.cc=%.o)
TEST_OBJS	= $(TEST_SRCS:%.cc=%.o)

default: $(OBJS)

all: test verification experiment

test:	test.o $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ test.o $(OBJS)

verification:	verification.o $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ verification.o $(OBJS)

experiment:	experiment.o $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ experiment.o $(OBJS)

clean:
	@rm -f test.o $(OBJS) test verification experiment *~ tags a.out *.o Makefile.bak

depend:	$(SRCS) $(TEST_SRCS)
	$(DEPEND) $(SRCS) $(TEST_SRCS)

tags: $(SRCS) $(TEST_SRCS)
	@ctags *.cc *.h *.in

# DO NOT DELETE

Instance.o: Instance.h PtrInterface.h Ptr.h Ptr.in Gore.h Nominal.h
Instance.o: Notifiee.h Activity.h Numeric.h Log.h
Gore.o: Gore.h PtrInterface.h Ptr.h Nominal.h Notifiee.h Ptr.in Activity.h
Gore.o: Numeric.h Log.h
ActivityImpl.o: Log.h Exception.h Activity.h PtrInterface.h Ptr.h Nominal.h
ActivityImpl.o: Numeric.h Notifiee.h Ptr.in ActivityImpl.h
test.o: Instance.h PtrInterface.h Ptr.h Ptr.in Notifiee.h Activity.h
test.o: Nominal.h Numeric.h
verification.o: Instance.h PtrInterface.h Ptr.h Ptr.in Notifiee.h Activity.h
verification.o: Nominal.h Numeric.h
experiment.o: Instance.h PtrInterface.h Ptr.h Ptr.in Notifiee.h Activity.h
experiment.o: Nominal.h Numeric.h Log.h
