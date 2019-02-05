.PHONY = jobs
jobs = bu-tec-per bu-tec-iti bu-tec-bas \
bu-def-per bu-def-iti bu-def-bas \
bu-imm-per bu-imm-iti bu-imm-bas \
co-tec-per co-tec-iti co-tec-bas \
co-def-per co-def-iti co-def-bas \
co-imm-per co-imm-iti co-imm-bas \
td-tec-iti td-tec-bas \
td-def-iti td-def-bas \
td-imm-iti td-imm-bas 

all: $(jobs)

$(jobs):
	./run.sh -m $@ -f out/$@-tests.txt -r 3 > /dev/null
