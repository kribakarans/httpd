# GNU Makefile to build HTTPd:

PACKAGE  :=  HTTPd
VERSION  :=  0.1
ELFNAME  :=  httpd
TARGET   :=  $(ELFNAME).out

# Build options
CC       ?=  gcc
CFLAGS   := -g -MMD -Wall -Wno-unused-function #-Werror #-Wextra
CPPFLAGS :=
INCLUDE  :=
LDLIBS   += -pthread

SRCDIR   :=  src
OBJDIR   :=  obj
DISTDIR  :=  dist
PREFIX   ?=  /usr/local

RM       :=  rm -f
BOLD     :=  $(shell tput bold)
NC       :=  $(shell tput sgr0)

# Source files
SRCS :=  $(SRCDIR)/main.c $(SRCDIR)/httpd.c $(SRCDIR)/utils.c

# Build object files
OBJS  = $(SRCS:%.c=$(OBJDIR)/%.o)

# Build dependencies
DEPS  = $(OBJS:%.o=%.d)

# Makefile execution starts here
all: info $(TARGET)

info:
	@echo "\n$(BOLD)Building $(PACKAGE) v$(VERSION) $(DISTNAME) release:$(NC)\n"
	@echo "TARGET   =  $(TARGET)"
	@echo "COMPILER =  $(CC)"
	@echo "CFLAGS   = $(CFLAGS)"
	@echo "CPPFLAGS = $(CPPFLAGS)"
	@echo "LDLIBS   = $(LDLIBS)"

# Link object files
$(TARGET): $(OBJS)
	@echo  "\n\nLinking   ... object-files"
	@$(CC) -o $(TARGET) $(OBJS) $(LDFLAGS) $(LDLIBS)
	@echo "$(BOLD)$(PACKAGE)$(NC) build completed [$(BOLD)$(TARGET)$(NC)]"

# Compile source files
$(OBJDIR)/%.o : %.c
	@mkdir -p $(@D)
	@printf "\nCompiling ... $<"
	@$(CC) $(CFLAGS) $(CPPFLAGS) $(INCLUDE) -c $< -o $@
	@printf "\33[2K\rCompiled  ... $<"

clean:
	$(RM) -r $(OBJDIR) $(TARGET)

# Include dependencies
-include $(DEPS)

# EOF
