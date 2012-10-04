.PHONY: clean All

All:
	@echo ----------Building project:[ SLADE - Release ]----------
	@"$(MAKE)" -f "SLADE.mk"
clean:
	@echo ----------Cleaning project:[ SLADE - Release ]----------
	@"$(MAKE)" -f "SLADE.mk" clean
