SUBDIRS:= `ls | egrep -v '^(CVS)$$'`

all:
	@for i in $(SUBDIRS); do if test -e $$i/Makefile ; then make  -C $$i || { exit 1;} fi; done;

clean:
	@rm -f *.bz2
	@for i in $(SUBDIRS); do if test -e $$i/Makefile ; then make  -C $$i clean || { exit 1;} fi; done;

