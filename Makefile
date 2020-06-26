include Makefile.OSX
#include Makefile.pmdk
#include Makefile.ucx

ifeq ($(VERBOSE),1)
  VC=
  NVC=@$(TRUE) ||
else
  VC=@
  NVC=@
endif

GIT_VERSION := $(shell git describe --abbrev=8 --dirty --always --tags)
CXXFLAGS+=-DVERSION=\"$(GIT_VERSION)\"

SRC_CPP_RED=red.cpp $(SRC_CPP)
SRC_CPP_RED_CLIENT=red-client.cpp $(SRC_CPP)
SRC_CPP_RED_SERVER=red-server.cpp $(SRC_CPP)
SRC_CPP_AVL_CLIENT=avl-client.cpp $(SRC_CPP)
SRC_CPP_AVL_SERVER=avl-server.cpp $(SRC_CPP)

SRC_CPP_AVL=avl.cpp $(SRC_CPP)

OBJS_RED=$(SRC_CPP_RED:.cpp=.o)
OBJS_AVL=$(SRC_CPP_AVL:.cpp=.o)

OBJS_RED_CLIENT=$(SRC_CPP_RED_CLIENT:.cpp=.o)
OBJS_RED_SERVER=$(SRC_CPP_RED_SERVER:.cpp=.o)

OBJS_AVL_CLIENT=$(SRC_CPP_AVL_CLIENT:.cpp=.o)
OBJS_AVL_SERVER=$(SRC_CPP_AVL_SERVER:.cpp=.o)

JSON_OBJS_RED=$(OBJS_RED:.o=.o.json)

all: avl red # red-server red-client avl-server avl-client

red: $(OBJS_RED)
	$(NVC)echo -e "\e[0;33mCreating" $@ "\033[39m"
	$(VC)$(LD) -o $@ $(OBJS_RED) $(LDFLAGS)

avl: $(OBJS_AVL)
	$(NVC)echo -e "\e[0;33mCreating" $@ "\033[39m"
	$(VC)$(LD) -o $@ $(OBJS_AVL) $(LDFLAGS)

red-client: $(OBJS_RED_CLIENT)
	$(NVC)echo -e "\e[0;33mCreating" $@ "\033[39m"
	$(VC)$(LD) -o $@ $(OBJS_RED_CLIENT) $(LDFLAGS)

red-server: $(OBJS_RED_SERVER)
	$(NVC)echo -e "\e[0;33mCreating" $@ "\033[39m"
	$(VC)$(LD) -o $@ $(OBJS_RED_SERVER) $(LDFLAGS)

avl-client: $(OBJS_AVL_CLIENT)
	$(NVC)echo -e "\e[0;33mCreating" $@ "\033[39m"
	$(VC)$(LD) -o $@ $(OBJS_AVL_CLIENT) $(LDFLAGS)

avl-server: $(OBJS_AVL_SERVER)
	$(NVC)echo -e "\e[0;33mCreating" $@ "\033[39m"
	$(VC)$(LD) -o $@ $(OBJS_AVL_SERVER) $(LDFLAGS)

.cpp.o: Makefile Makefile.OSX Makefile.pmdk Makefile.ucx
	$(NVC)echo -e "\e[0;32mCompiling" $< "\033[39m"
	$(VC)$(CXX) -MD $(CXXFLAGS) -MJ $@.json -c $< -o $@

# note, double $ because of escaping in Makefiles
compile_commands.json: $(SRC_CPP_RED) Makefile
	$(NVC)echo -e "\e[0;32mCreating Compilation Database" $< "\033[39m"
	$(VC)sed -e '1s/^/[/' -e '$$ s/,$$/]/' $(JSON_OBJS_RED) > compile_commands.json

clean:
	@-rm -rf $(shell find . -type f -name '*.o')
	@-rm -rf $(shell find . -type f -name '*.d')
	@-rm -rf $(shell find . -type f -name '*.o.json')
	@-rm -f ./red ./red-server ./red-client ./avl

#$(info $$SRC_CPP_RED is [${SRC_CPP_RED}])
#$(info $$OBJS_RED is [${OBJS_RED}])

#$(info $$SRC_CPP_AVL is [${SRC_CPP_AVL}])
$(info $$OBJS_RED_SERVER is [${OBJS_RED_SERVER}])

-include $(OBJS_RED:.o=.d)
-include $(OBJS_AVL:.o=.d)
-include $(OBJS_RED_CLIENT:.o=.d)
-include $(OBJS_RED_SERVER:.o=.d)

beautify:
	@$(CLANGFORMAT) -style=file -i *.cpp *.h

profile:
	instruments -t Time\ Profiler ./red

watch:
	fswatch -o $(SRC_CPP_RED) | xargs -n1 -I{} make -j red

.PHONY: all clean beautify watch profile
