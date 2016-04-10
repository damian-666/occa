#---[ OCCA_DIR ]----------------------------------
rmSlash = $(patsubst %/,%,$1)

OCCA_DIR := $(call rmSlash,$(dir $(abspath $(lastword $(MAKEFILE_LIST)))))
PROJ_DIR := $(OCCA_DIR)

include $(OCCA_DIR)/scripts/makefile

#---[ WORKING PATHS ]-----------------------------
ifeq ($(usingWinux),0)
  compilerFlags  += $(picFlag) -DOCCA_COMPILED_DIR="$(OCCA_DIR)"
  fCompilerFlags += $(picFlag)
else
  sharedFlag += $(picFlag)
endif

# [-L$OCCA_DIR/lib -locca] are kept for applications
#   using $OCCA_DIR/scripts/makefile
paths := $(filter-out -L$(OCCA_DIR)/lib,$(paths))
links := $(filter-out -locca,$(links))

incPath := $(iPath)
iPath   := $(iPath)/occa
#=================================================

#---[ VARIABLES ]---------------------------------
srcToObject  = $(subst $(PROJ_DIR)/src,$(PROJ_DIR)/obj,$(patsubst %.f90,%.o,$(1:.cpp=.o)))
fsrcToObject = $(subst $(PROJ_DIR)/src,$(PROJ_DIR)/obj,$(1:.f90=.o))
srcToHeader  = $(subst $(PROJ_DIR)/src,$(PROJ_DIR)/inc,$(wildcard $(1:.cpp=.hpp)))

sources  = $(realpath $(shell find $(PROJ_DIR)/src     -type f -name '*.cpp'))
headers  = $(realpath $(shell find $(PROJ_DIR)/include -type f -name '*.hpp'))
fsources = $(realpath $(shell find $(PROJ_DIR)/src     -type f -name '*.f90'))

#  ---[ Languages ]-----------
ifndef OCCA_COMPILE_PYTHON
	sources := $(filter-out $(OCCA_DIR)/src/lang/python=/%,$(sources))
endif

ifndef OCCA_COMPILE_JAVA
	sources := $(filter-out $(OCCA_DIR)/src/lang/java/%,$(sources))
endif

ifndef OCCA_COMPILE_OBJC
	sources := $(filter-out $(OCCA_DIR)/src/lang/objc/%,$(sources))
endif

ifndef OCCA_COMPILE_FORTRAN
	sources := $(filter-out $(OCCA_DIR)/src/lang/fortran/%,$(sources))
endif
#  ===========================

objects = $(call srcToObject,$(sources))

outputs = $(lPath)/libocca.so $(bPath)/occa
#=================================================


#---[ COMPILE LIBRARY ]---------------------------
all: $(objects) $(outputs)
#=================================================


#---[ PYTHON ]------------------------------------
python: $(lPath)/_C_occa.so
	python $(OCCA_DIR)/scripts/make.py
#=================================================


#---[ BUILDS ]------------------------------------
#  ---[ libocca ]-------------
$(lPath)/libocca.so:$(objects) $(headers)
	$(compiler) $(compilerFlags) $(sharedFlag) -o $(lPath)/libocca.so $(flags) $(objects) $(paths) $(filter-out -locca, $(links))

$(bPath)/occa:$(OCCA_DIR)/scripts/occa.cpp $(lPath)/libocca.so
	$(compiler) $(compilerFlags) -o $(bPath)/occa $(flags) $(OCCA_DIR)/scripts/occa.cpp $(paths) $(links) -L$(OCCA_DIR)/lib -locca

#  ---[ C++ ]-----------------
define compileCpp
$(call srcToObject,$1):$1 $(call srcToHeader,$1)
	@mkdir -p $(dir $(call srcToObject,$1))
	$(compiler) $(compilerFlags) -o $(call srcToObject,$1) $(flags) -c $(paths) $1
endef

$(foreach s,$(sources),$(eval $(call compileCpp,$s)))
#  ===========================

#  ---[ Fortran ]-------------
define compileF90
$(call srcToObject,$1):$1 $(call srcToHeader,$1)
	@mkdir -p $(dir $(call srcToObject,$1))
	$(fCompiler) $(fCompilerFlags) $(fModDirFlag) $(lPath) -o $@ $(fFlags) -c $<
endef

$(foreach s,$(fsources),$(eval $(call compileF90,$s)))
#  ===========================

#  ---[ Python ]-------------
pyflags = -I${OCCA_PYTHON_DIR}/ -I${OCCA_NUMPY_DIR} -L${OCCA_LIBPYTHON_DIR} -l${OCCA_LIBPYTHON}

$(lPath)/_C_occa.so: $(lPath)/libocca.so $(iPath)/lang/python/_C_occa.h $(iPath)/lang/python/_C_occa.h
	gcc $(compilerFlags) $(sharedFlag) $(sPath)/python/_C_occa.c -o $@ -I$(incPath) -I$(iPath)/python -L$(lPath) $(pyFlags) -locca
#  ===========================
#=================================================


#---[ TEST ]--------------------------------------
test:
	echo '---[ Testing ]--------------------------'
	cd $(OCCA_DIR); \
	make -j 4 CXXFLAGS='-g' FCFLAGS='-g'

	cd $(OCCA_DIR)/examples/addVectors/cpp; \
	make -j 4 CXXFLAGS='-g' FCFLAGS='-g'; \
	./main

	cd $(OCCA_DIR)/examples/addVectors/c; \
	make -j 4 CXXFLAGS='-g' FCFLAGS='-g'; \
	./main

	cd $(OCCA_DIR)/examples/addVectors/f90; \
	make -j 4 CXXFLAGS='-g' FCFLAGS='-g'; \
	./main

	cd $(OCCA_DIR)/examples/reduction/; \
	make -j 4 CXXFLAGS='-g' FCFLAGS='-g'; \
	./main

	cd $(OCCA_DIR)/examples/usingArrays/; \
	make -j 4 CXXFLAGS='-g' FCFLAGS='-g'; \
	./main
#=================================================


#---[ CLEAN ]-------------------------------------
clean:
	rm -rf $(oPath)/*
	rm -rf $(bPath)/*
	rm  -f $(lPath)/libocca.so
	rm  -f $(lPath)/*.mod
	rm  -f $(lPath)/_C_occa.so
	rm  -f $(OCCA_DIR)/scripts/main
#=================================================