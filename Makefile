.PHONY : all clean

PATCH	:=	patches
BINFILS	:=	$(wildcard $(PATCH)/*.s) $(filter %/, $(wildcard $(PATCH)/*/))
BINFILS :=	$(filter %.bin, $(BINFILS:.s=.bin) $(BINFILS:/=.bin))

all: 
	@$(foreach sfile,$(wildcard $(PATCH)/*.s), $(PATCH)/armips $(sfile);)
	@$(foreach sfile,$(filter %/, $(wildcard $(PATCH)/*/)), $(MAKE) -C $(sfile);)
	@$(PATCH)/bin2c -o payload/source/patches.h $(BINFILS)
	@cd payload && make TARGET=../arm9loaderhax
	
clean :
	@cd payload && make clean TARGET=../arm9loaderhax
	@rm -f $(PATCH)/*.bin
	@$(foreach sfile,$(filter %/, $(wildcard $(PATCH)/*/)), $(MAKE) -C $(sfile) clean;)
	