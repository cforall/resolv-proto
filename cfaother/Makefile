# Copyright (c) 2015 University of Waterloo
#
# The contents of this file are covered under the licence agreement in 
# the file "LICENCE" distributed with this repository.

.PHONY = jobs
jobs = td-imm-bas td-imm-iti \
co-imm-bas co-imm-iti co-imm-per \
bu-imm-bas bu-imm-iti bu-imm-per \
td-def-bas td-def-iti \
co-def-bas co-def-iti co-def-per \
bu-def-bas bu-def-iti bu-def-per \
td-tec-bas td-tec-iti \
co-tec-bas co-tec-iti co-tec-per \
bu-tec-bas bu-tec-iti bu-tec-per

all: $(jobs)

$(jobs):
	./run.sh -m $@ -f out/$@-tests.txt -r 3 > /dev/null
