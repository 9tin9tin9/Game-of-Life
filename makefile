CC = clang++
CXX = $(CC)
FSANFLAG = -fsanitize=address -fsanitize=alignment 
OPTFLAG = -g -O0
STDFLAG = -std=c++20
LDFLAGS = ~/program/pixel/target/*.o `pkg-config sdl2 --libs`
CFLAGS = $(STDFLAG) $(FSANFLAG) $(OPTFLAG) -pipe -I/Users/austinsiu/Desktop/program/pixel/src/include `pkg-config sdl2 --cflags`
CPPFLAGS = $(CFLAGS)
FILE_NAMES =
FILE_TYPE = cpp
HEADER_TYPE = hpp

SRCDIR = src
OBJDIR = target
OBJ = $(addprefix $(OBJDIR)/,$(addsuffix .o,$(FILE_NAMES)))
HEADER_DIR = $(SRCDIR)/include
HEADER = $(addsuffix .$(HEADER_TYPE),$(addprefix $(HEADER_DIR)/,$(FILE_NAMES)))

build:
	@if [[ ! -e $(OBJDIR) ]]; then mkdir $(OBJDIR); echo created \'$(OBJDIR)\' directory; fi
	@if [[ ! -e $(OBJDIR)/release/ ]]; then mkdir $(OBJDIR)/release; echo created \'$(OBJDIR)/release\' directory; fi
	@rm -rf $(OBJDIR)/release/*
	@make src/main
	@mv src/main $(OBJDIR)/release
	@mv src/main.dSYM $(OBJDIR)/release

run:
	@./target/release/main

src/main: $(OBJ)

$(OBJ): $(OBJDIR)/%.o: $(SRCDIR)/%.$(FILE_TYPE) $(HEADER) makefile
	$(CC) -c $(CFLAGS) $< -o $@

.PHONY: clean test

init:
	if [[ ! -e $(SRCDIR) ]]; then \
	mkdir $(SRCDIR); echo created \`$(SRCDIR)\` directory; \
	printf "#include <iostream>\n\nint main(){\n    std::cout << \"Hello World\";\n    return 0;\n}" > src/main.cpp; \
	fi

test:
	@cd test && ./run_test.sh

clean:
	rm -rf $(OBJDIR)
