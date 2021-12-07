CC = clang++
CXX = $(CC)
FSANFLAG = # -fsanitize=address -fsanitize=alignment 
OPTFLAG = -g -O3
STDFLAG = -std=c++20
LDFLAGS = `pkg-config sdl2 --libs`
CFLAGS = $(STDFLAG) $(FSANFLAG) $(OPTFLAG) -pipe `pkg-config sdl2 --cflags`
CPPFLAGS = $(CFLAGS)
FILE_NAMES = pixel
FILE_TYPE = cpp

SRCDIR = src
OBJDIR = target
OBJ = $(addprefix $(OBJDIR)/,$(addsuffix .o,$(FILE_NAMES)))

build:
	@if [[ ! -e $(OBJDIR) ]]; then mkdir $(OBJDIR); echo created \'$(OBJDIR)\' directory; fi
	@if [[ ! -e $(OBJDIR)/release/ ]]; then mkdir $(OBJDIR)/release; echo created \'$(OBJDIR)/release\' directory; fi
	@rm -rf $(OBJDIR)/release/*
	@make src/main
	@mv src/main $(OBJDIR)/release
	@mv src/main.dSYM $(OBJDIR)/release
	@if [[ ! -e gameOfLife ]]; then ln -s ./target/release/main gameOfLife; echo created \'gameOfLife\' symlink; fi

run:
	@./target/release/main

src/main: $(OBJ)

$(OBJ): $(OBJDIR)/%.o: $(SRCDIR)/%.$(FILE_TYPE) makefile
	$(CC) -c $(CFLAGS) $< -o $@

.PHONY: clean

clean:
	rm -rf $(OBJDIR)
	rm gameOfLife
