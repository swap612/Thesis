###############################################################################
# Makefile script for PQoS library and sample application
#
# @par
# BSD LICENSE
# 
# Copyright(c) 2014-2015 Intel Corporation. All rights reserved.
# All rights reserved.
# 
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 
#   * Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
#   * Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in
#     the documentation and/or other materials provided with the
#     distribution.
#   * Neither the name of Intel Corporation nor the names of its
#     contributors may be used to endorse or promote products derived
#     from this software without specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.O
# 
#  version: CMT_CAT_Refcode.L.0.1.2-10
###############################################################################


LIBSONAME = libpqos.so.1
LIBANAME = libpqos.a
ifeq ($(SHARED),y)
LIBNAME = $(LIBSONAME)
else
LIBNAME = $(LIBANAME)
endif 

CC = gcc
LDFLAGS = -L. -lpthread
CFLAGS = -pthread -I./ \
	-W -Wall -Wextra -Wstrict-prototypes -Wmissing-prototypes \
	-Wmissing-declarations -Wold-style-definition -Wpointer-arith \
	-Wcast-qual -Wundef -Wwrite-strings 
ifneq ($(EXTRA_CFLAGS),)
CFLAGS += $(EXTRA_CFLAGS)
endif
DOXY_DIRS = doc_api doc_lib

# ICC and GCC options
ifeq ($(CC),icc)
else
CFLAGS += -Wcast-align -Wnested-externs
endif

# so or static build
ifeq ($(SHARED),y)
CFLAGS += -fPIC -g
else
CFLAGS += 
endif 

# DEBUG build
ifeq ($(DEBUG),y)
CFLAGS += -g -ggdb -O0 -DDEBUG
else
CFLAGS += -g -O3
endif 

# Build targets and dependencies
OBJS = cpuinfo.o machine.o host_cap.o host_allocation.o host_monitoring.o utils.o log.o
DEPFILE = $(LIBANAME).dep

all: $(LIBNAME)

$(LIBNAME): $(OBJS)
ifeq ($(SHARED),y)
	$(CC) -shared -Wl,-soname,$(LIBNAME) -o $(LIBNAME).0.1 $^ -l c
	ln -s $(LIBNAME).0.1 $(LIBNAME)
else
	$(AR) crvs $@ $^
endif

.PHONY: dep
dep: $(DEPFILE)

$(DEPFILE): $(subst .o,.c,$(OBJS))
	$(CC) -MM $(CFLAGS) $^ > $@

.PHONY: clean clobber doxy help

help:
	@echo "PQoS library make targets:"
	@echo "    make all          - build static library"
	@echo "    make all SHARED=y - build shared library"
	@echo "    make all DEBUG=y  - build static library for debugging"
	@echo "    make clean        - clean build files"
	@echo "    make clobber      - clear even more files than clean"
	@echo "    make doxy         - make doxygen documentation" 

doxy:
	doxygen api_doxygen.cfg
	doxygen lib_doxygen.cfg

clean:
	-rm -f $(OBJS) $(LIBNAME)* $(DEPFILE)

clobber:
	-rm -f $(OBJS) $(LIBANAME) $(LIBSONAME)* $(DEPFILE) ./*~
	-rm -rf $(DOXY_DIRS)

# dependecies file
-include $(DEPFILE)  
