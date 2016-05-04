.PHONY : all clean

PATCH	:=	patches
BINFILS	:=	$(wildcard $(PATCH)/*.s) $(filter %/, $(wildcard $(PATCH)/*/))
BINFILS :=	$(filter %.bin, $(BINFILS:.s=.bin) $(BINFILS:/=.bin))
LINKSCR	:=	$(CURDIR)/tools/bootstrap.ld

all: 
	@$(foreach sfile,$(wildcard $(PATCH)/*.s), tools/armips $(sfile);)
	@$(foreach sfile,$(filter %/, $(wildcard $(PATCH)/*/)), $(MAKE) -C $(sfile) LDSCRIPT=$(LINKSCR);)
	@tools/bin2c -o payload/source/patches.h $(BINFILS)
	@make -C payload TARGET=../arm9loaderhax LDSCRIPT=$(LINKSCR)
	
clean :
	@make -C payload clean TARGET=../arm9loaderhax
	@rm -f $(PATCH)/*.bin
	@$(foreach sfile,$(filter %/, $(wildcard $(PATCH)/*/)), $(MAKE) -C $(sfile) clean;)
	