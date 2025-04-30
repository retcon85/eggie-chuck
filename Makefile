# Forgive me for I know not what I do.
SHELL = /bin/bash

# OPTIONS YOU MIGHT NEED TO CHANGE:
# change this to true to build in PSGlib (for music & sound in your game)
USEPSGLIB := true
ASSETSOPTS :=
# default behaviour is to automatically generate bank options, modify if you know what you are doing
BANKS = $(foreach bank,$(BANK_NAMES),-Wl-b_$(bank)=0x8000)
# allow for unmanaged banks at beginning of ROM. 2 is usually the starting value here.
FIRSTBANK := 2
# only for unit tests
TEST_BANKS := #-Wl-b_TESTBANK=0x8000
# defaults to the next bank after those already detected automatically
TEST_BANK_NUM = $(words bank0 bank1 $(sort $(BANK_NAMES)))
ifdef LEVEL_TEST
COMPILER_ARGS := -DLEVEL_TEST=1
endif
ifdef DEBUG
COMPILER_ARGS := $(COMPILER_ARGS) -DDEBUG=1
endif

# by default output will take the name of the folder we're in
PROJECTNAME := $(notdir $(CURDIR))
TARGETDIR := ./build/
SOURCEDIR := ./src/
ASSETSDIR := ./assets/
TARGETEXT := sms
SOURCEEXT := c
HEADEREXT := h
# the default entrypoint (where main function is defined), which must come first in the linker
ENTRYPOINT := main
SMSLIB_DIR := /opt/devkitsms/lib
SMSINC_DIR := /opt/devkitsms/include
INCLUDE_OPTIONS := $(patsubst %,-I %,$(SMSINC_DIR) $(SOURCEDIR) $(wildcard $(SOURCEDIR)**/))
ifeq ("$(USEPSGLIB)", "true")
PSGLIB := $(SMSLIB_DIR)/PSGlib.lib
PSGMACRO := -DUSEPSGLIB
endif
VERSION := $(file <VERSION)
VERSION_SUFFIX := -$(VERSION)

TARGET := $(TARGETDIR)$(PROJECTNAME).$(TARGETEXT)
VERSIONED_TARGET = $(TARGETDIR)$(PROJECTNAME)$(VERSION_SUFFIX).$(TARGETEXT)
SOURCE_DIRS := $(SOURCEDIR) $(SOURCEDIR)**/
SOURCES := $(foreach dir,$(SOURCE_DIRS),$(wildcard $(dir)*.$(SOURCEEXT)))
HEADERS := $(foreach dir,$(SOURCE_DIRS),$(wildcard $(dir)*.$(HEADEREXT)))
BANKED_SOURCES := $(foreach dir,$(SOURCE_DIRS),$(wildcard $(dir)*.bank*.$(SOURCEEXT)))
OBJECTS := $(patsubst $(SOURCEDIR)%.$(SOURCEEXT),$(TARGETDIR)%.rel,$(SOURCES))
BANKED_OBJECTS := $(patsubst $(SOURCEDIR)%.$(SOURCEEXT),$(TARGETDIR)%.rel,$(BANKED_SOURCES))
BANK_NAMES := $(sort $(subst .,,$(suffix $(basename $(BANKED_SOURCES)))))
# okay this is kind of diabolical...
SORTED_BANKED_OBJECTS := $(subst :slash:,/,$(notdir $(sort $(join $(suffix $(basename $(BANKED_OBJECTS))),$(addprefix /,$(subst /,:slash:,$(BANKED_OBJECTS)))))))

# automatic asset bundling
ASSETS := $(wildcard $(ASSETSDIR)*)
ASSETSBINTARGET := $(patsubst $(ASSETSDIR)%.bin,$(TARGETDIR)$(ASSETSDIR)%.bin,$(filter %.bin, $(ASSETS)))
ASSETSPSGTARGET := $(patsubst $(ASSETSDIR)%.psg,$(TARGETDIR)$(ASSETSDIR)%.psg,$(filter %.psg, $(ASSETS)))
ASSETSVGMTARGET := $(patsubst $(ASSETSDIR)%.vgm,$(TARGETDIR)$(ASSETSDIR)%.psg,$(filter %.vgm, $(ASSETS)))
ASSETSTILESTARGET := $(patsubst $(ASSETSDIR)%.bmp,$(TARGETDIR)$(ASSETSDIR)%.tiles,$(filter %.bmp, $(ASSETS)))
ASSETSPALETTETARGET := $(patsubst $(ASSETSDIR)%.bmp,$(TARGETDIR)$(ASSETSDIR)%.palette,$(filter %.bmp, $(ASSETS)))
# patterns included in $(BUNDLEDASSETS) will be bundled; others will be ignored
BUNDLEDASSETS = $(ASSETSBINTARGET) $(ASSETSPSGTARGET) $(ASSETSVGMTARGET) $(ASSETSTILESTARGET) $(ASSETSPALETTETARGET)
# the file src/assets.genererated.h will be automatically generated to index all bundled assets in code
ASSETSHEADER := $(SOURCEDIR)assets.generated.$(HEADEREXT)

# customize this list to explicitly specify the order of linking
MAINS := $(TARGETDIR)$(ENTRYPOINT).rel

# main build target
build: $(TARGETDIR) assets $(TARGET) $(VERSIONED_TARGET)

# create the build folder if it doesn't exist
$(TARGETDIR):
	mkdir -p $(TARGETDIR)

$(VERSIONED_TARGET): $(TARGET) VERSION
	cp $< $@

# link stage - generally runs once to create a single output
$(TARGETDIR)%.ihx: $(OBJECTS)
	sdcc -L$(SMSLIB_DIR) -o$@ -mz80 --no-std-crt0 --data-loc 0xC000 $(BANKS) $(SMSLIB_DIR)/crt0_sms.rel $(MAINS) SMSlib.lib $(PSGLIB) $(filter-out $(MAINS) $(BANKED_OBJECTS),$(OBJECTS)) $(SORTED_BANKED_OBJECTS) || (rm -f $@ && echo "** LINK FAILED **")

# compile unbanked sources
$(filter-out $(BANKED_OBJECTS), $(OBJECTS)): $(TARGETDIR)%.rel: $(SOURCEDIR)%.$(SOURCEEXT) $(ASSETSHEADER) $(HEADERS) Makefile
	mkdir -p $(TARGETDIR)$(dir $*)
	sdcc $(INCLUDE_OPTIONS) $(COMPILER_ARGS) --opt-code-speed -c -mz80 -o$(TARGETDIR)$(dir $*) --peep-file $(SMSLIB_DIR)/peep-rules.txt $(PSGMACRO) $<

# compile banked sources
$(BANKED_OBJECTS): $(TARGETDIR)%.rel: $(SOURCEDIR)%.$(SOURCEEXT) $(ASSETSHEADER) $(HEADERS) Makefile
	mkdir -p $(TARGETDIR)$(dir $*)
	sdcc $(INCLUDE_OPTIONS) $(COMPILER_ARGS) -DTHIS_BANK=$(subst .bank,,$(suffix $(basename $<))) --constseg $(subst .,,$(suffix $(basename $<))) --opt-code-speed -c -mz80 -o$(TARGETDIR)$(dir $*) --peep-file $(SMSLIB_DIR)/peep-rules.txt $(PSGMACRO) $<

# packing stage - generally runs once to create a single output
# pads to minimum 64km with -pm to ensure SRAM is used
$(TARGET): $(TARGETDIR)$(ENTRYPOINT).ihx
	ihx2sms -pm  $< $(TARGET)

.PHONY: assets
assets: $(TARGETDIR) $(TARGETDIR)$(ASSETSDIR) $(BUNDLEDASSETS) $(ASSETSHEADER) Makefile

$(SOURCEDIR)assets.generated.bank%.c: $(TARGETDIR)bank%.c $(ASSETSHEADER)
	mv $< $@

# automatically bundle selected assets using assets2banks
$(ASSETSHEADER): $(TARGETDIR)$(ASSETSDIR) $(BUNDLEDASSETS) $(ASSETSDIR)assets2banks.cfg
	echo '// This file is automatically generated - do not modify!' > $(ASSETSHEADER)
	-cp  $(ASSETSDIR)assets2banks.cfg $(TARGETDIR)$(ASSETSDIR)
	cd $(TARGETDIR) && assets2banks $(ASSETSDIR) $(ASSETSOPTS) --firstbank=$(FIRSTBANK) --allowsplitting --singleheader=../$(ASSETSHEADER)
	for f in $(TARGETDIR)bank*.c; do \
		b=`basename $$f`; \
		t="$(SOURCEDIR)assets.generated.$${b}"; \
		echo '// This file is automatically generated - do not modify!' > $$t; \
		# echo "#pragma constseg $${b/.c/}" >> $$t; \
		echo "" >> $$t; \
		cat $$f >> $$t; \
	done

# include any existing .psg file in assets folder in bundled assets
$(TARGETDIR)$(ASSETSDIR)%.psg: $(ASSETSDIR)%.psg
	cp $< $@
# include any existing .tiles file in assets folder in bundled assets
$(TARGETDIR)$(ASSETSDIR)%.tiles: $(ASSETSDIR)%.tiles
	cp $< $@
# include any existing .palette file in assets folder in bundled assets
$(TARGETDIR)$(ASSETSDIR)%.palette: $(ASSETSDIR)%.palette
	cp $< $@
# include any existing .bin file in assets folder in bundled assets
$(TARGETDIR)$(ASSETSDIR)%.bin: $(ASSETSDIR)%.bin
	cp $< $@

# convert a .bmp file in assets folder to .tiles and .palette files in bundled assets
$(TARGETDIR)$(ASSETSDIR)%.tiles: $(ASSETSDIR)%.bmp
	img2tiles $< -obin --no-palette > $@
$(TARGETDIR)$(ASSETSDIR)%.palette: $(ASSETSDIR)%.bmp
	img2tiles $< -obin --no-tiles > $@

# convert a .vgm file in assets folder to .psg file in bundled assets
$(TARGETDIR)$(ASSETSDIR)%.psg: $(ASSETSDIR)%.vgm
	vgm2psg $< $@
	retcon-audio psg $@ $@

# create bundled assets folder under build as staging area
$(TARGETDIR)$(ASSETSDIR):
	mkdir -p $(TARGETDIR)$(ASSETSDIR)

# make clean to remove the build folder and all generated files
.PHONY: clean
clean:
	rm -rf $(TARGETDIR)
	-rm -r */*.generated*.*

# unit testing
TEST_SOURCEDIR := ./test/
TEST_TARGETDIR := $(TARGETDIR)test/
TEST_TARGET := $(TEST_TARGETDIR)$(PROJECTNAME)_test.$(TARGETEXT)
TEST_SOURCES := $(wildcard $(TEST_SOURCEDIR)*.$(SOURCEEXT))
TEST_OBJECTS := $(patsubst $(TEST_SOURCEDIR)%.$(SOURCEEXT),$(TEST_TARGETDIR)%.rel,$(TEST_SOURCES))
# explicitly excluding these large and untested objects to cut down on test executable size
TEST_EXCLUDE_OBJECTS := 
TEST_MAIN := $(TARGETDIR)test/test_main.rel

.PHONY: test
test: $(TEST_TARGET)

$(TEST_TARGETDIR):
	mkdir -p $(TEST_TARGETDIR)

$(TEST_TARGETDIR)%.rel: $(TEST_SOURCEDIR)%.$(SOURCEEXT) $(TEST_SOURCEDIR)*.$(HEADEREXT) $(TEST_TARGETDIR)
	sdcc $(INCLUDE_OPTIONS) -DTESTLIB_FONT_BANK=$(TEST_BANK_NUM) --opt-code-speed -c -mz80 -o$(TEST_TARGETDIR) --peep-file $(SMSLIB_DIR)/peep-rules.txt $(PSGMACRO) $<

$(TEST_TARGETDIR)test.ihx: $(OBJECTS) $(TEST_OBJECTS)
	sdcc -L$(SMSLIB_DIR) -o$@ -mz80 --no-std-crt0 --data-loc 0xC000 $(BANKS) $(TEST_BANKS) $(SMSLIB_DIR)/crt0_sms.rel $(TEST_MAIN) SMSlib.lib $(PSGLIB) $(filter-out $(MAINS) $(BANKED_OBJECTS) $(TEST_EXCLUDE_OBJECTS),$(OBJECTS)) $(SORTED_BANKED_OBJECTS) $(filter-out $(TEST_MAIN),$(TEST_OBJECTS)) || (rm -f $@ && echo "** LINK FAILED **")

# pads to minimum 64km with -pm to ensure SRAM is used
$(TEST_TARGET): $(TEST_TARGETDIR)test.ihx
	ihx2sms -pm $< $(TEST_TARGET)


# intermediates never get deleted
.SECONDARY:
