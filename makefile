CXX = clang++
OPTFLAG = -g -O3
STDFLAG = -std=c++17
LDFLAGS = `pkg-config sdl2 --libs`
CPPFLAGS = $(STDFLAG) $(OPTFLAG) -pipe `pkg-config sdl2 --cflags`
FILE_NAMES = pixel

OBJDIR = target
OBJ = $(addprefix $(OBJDIR)/,$(addsuffix .o,$(FILE_NAMES)))

build:
	@if [[ ! -e $(OBJDIR) ]]; then mkdir $(OBJDIR); echo created \'$(OBJDIR)\' directory; fi
	@if [[ ! -e $(OBJDIR)/bin/ ]]; then mkdir $(OBJDIR)/bin; echo created \'$(OBJDIR)/bin\' directory; fi
	@rm -rf $(OBJDIR)/bin/*
	@make main
	@mv main $(OBJDIR)/bin
	@if [[ -e main.dSYM ]];then mv main.dSYM $(OBJDIR)/bin/; fi

run:
	@./$(OBJDIR)/bin/main

main: $(OBJ)

$(OBJ): $(OBJDIR)/%.o: %.cpp
	$(CXX) -c $(CPPFLAGS) $< -o $@


.PHONY: clean

clean:
	rm -rf $(OBJDIR)
