DOL :=	$(basename $(BIN)).dol

$(DOL)	: 	$(BIN)
	powerpc-gekko-objcopy -O binary $< $@


all-after: $(DOL)
