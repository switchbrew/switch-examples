MAKEFILES	:=	$(shell find . -mindepth 2 -name Makefile)

DATESTRING	:=	$(shell date +%Y)$(shell date +%m)$(shell date +%d)

all:
	@for i in $(MAKEFILES); do $(MAKE) -C `dirname $$i` || exit 1; done;

clean:
	@rm -f *.bz2
	@for i in $(MAKEFILES); do $(MAKE) -C `dirname $$i` clean || exit 1; done;

dist: clean
	@tar -cvjf switch-examples-$(DATESTRING).tar.bz2 *
