CC := g++
CFLAGS := -Wall -Wextra -std=c++17
SRCDIR := client
BUILDDIR := ../build

SRCS := client.cpp
OBJS := $(SRCS:$(SRCDIR)/%.cpp=$(BUILDDIR)/%.o)
TARGET := client

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $(BUILDDIR)/$@

$(BUILDDIR)/%.o: $(SRCDIR)/%.cpp
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILDDIR)/*.o $(BUILDDIR)/$(TARGET)
