# Insertions and deletions work:  "crossovers" and duplications don't.

all: sampler

# -fomit-frame-pointer
# -fprefetch-loop-arrays
# -ffast-math 

# -fexpensive-optimizations
# -fno-strength-reduce?
# -fno-exceptions -fno-rtti

# try -ffast-math, -march=pentium4, -malign-double, -mfpmath=sse,387
#     -msse2


# try -fforce-addr

#----------------- Definitions
LANGO = fast-math unroll-loops prefetch-loop-arrays 
DEBUG = pipe g  #pg
DEFS = # NDEBUG 
WARN = all no-sign-compare
OPT =  malign-double mfpmath=sse msse2 march=pentium4 O1
LDFLAGS = # -pg # -static 
LI=${CXX}

#------------------- Main 
PROGNAME = sampler
NAME = sampler
SOURCES = sequence.C tree.C alignment.C substitution.C moves.C \
          rng.C node-sample.C branch-sample.C exponential.C \
          eigenvalue.C parameters.C likelihood.C mcmc.C topology-sample.C \
	  choose.C map.C sequencetree.C 
LIBS = gsl gslcblas m
PROGNAMES = ${NAME} 
ALLSOURCES = ${SOURCES} 

${NAME} : ${SOURCES:%.C=%.o} ${LIBS:%=-l%} # libgsl.a libgslcblas.a libm.a


treecount: treecount.o tree.o sequencetree.o

#-----------------Other Files
OTHERFILES += 

#------------------- End
DEVEL = ../../..
# includes += /usr/local/include/
src      += 
include $(DEVEL)/GNUmakefile

