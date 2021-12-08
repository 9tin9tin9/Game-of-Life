CXX = clang++
OPTFLAG = -O3
STDFLAG = -std=c++17
LDFLAGS = `pkg-config sdl2 --libs`
CPPFLAGS = $(STDFLAG) $(OPTFLAG) -pipe `pkg-config sdl2 --cflags`
FILE_NAMES = pixel

OBJDIR = target
OBJ = $(addprefix $(OBJDIR)/,$(addsuffix .o,$(FILE_NAMES)))

build:
	@if [[ ! -e $(OBJDIR) ]]; then mkdir $(OBJDIR); echo created \'$(OBJDIR)\' directory; fi
	@if [[ ! -e $(OBJDIR)/release/ ]]; then mkdir $(OBJDIR)/release; echo created \'$(OBJDIR)/release\' directory; fi
	@rm -rf $(OBJDIR)/release/*
	@make src/main
	@mv src/main $(OBJDIR)/release
	@if [[ ! -e gameOfLife ]]; then ln -s $(OBJDIR)/release/main gameOfLife; echo created symlink gameOfLife -\> $(OBJDIR)/release/main; fi

run:
	@./$(OBJDIR)/release/main

src/main: $(OBJ)

$(OBJ): $(OBJDIR)/%.o: src/%.cpp makefile
	$(CXX) -c $(CPPFLAGS) $< -o $@

.PHONY: clean

clean:
	rm -rf $(OBJDIR)
	rm gameOfLife
