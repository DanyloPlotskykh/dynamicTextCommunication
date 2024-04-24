CC := g++
CFLAGS := -Wall -Wextra -std=c++17
SRCDIR := server
BUILDDIR := ../build

SRCS := server.cpp main.cpp # Получаем список всех .cpp файлов в папке $(SRCDIR)
OBJS := $(SRCS:$(SRCDIR)/%.cpp=$(BUILDDIR)/%.o)  # Заменяем путь к .cpp файлам на путь к .o файлам в папке $(BUILDDIR)
TARGET := server

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $(BUILDDIR)/$@

$(BUILDDIR)/%.o: $(SRCDIR)/%.cpp
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILDDIR)/*.o $(BUILDDIR)/$(TARGET)
