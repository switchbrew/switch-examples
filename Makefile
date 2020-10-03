MAKEFILES	:=	$(subst ./,,$(shell find . -mindepth 2 -name Makefile))

TARGETS	:= $(dir $(MAKEFILES))

DATESTRING	:=	$(shell date +%Y)$(shell date +%m)$(shell date +%d)

all: $(TARGETS)

.PHONY: $(TARGETS)

$(TARGETS):
	@$(MAKE) -C $@

clean:
	@rm -f *.bz2
	@for i in $(TARGETS); do $(MAKE) -C $$i clean || exit 1; done;

dist: clean
	@tar -cvjf switch-examples-$(DATESTRING).tar.bz2 *
