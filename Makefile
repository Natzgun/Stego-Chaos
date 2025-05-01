PROGRAMS = hide reveal
SRC_DIR = src
INC_DIR = includes

# Archivos fuente y sus respectivas dependencias
SRCS_hide = $(SRC_DIR)/hide.cpp
SRCS_reveal = $(SRC_DIR)/reveal.cpp

# Configuración del compilador
CXX = g++
CXXFLAGS = -I$(INC_DIR) `pkg-config --cflags opencv4`
LDFLAGS = `pkg-config --libs opencv4`

# Regla para compilar todos los programas
all: $(PROGRAMS)

# Reglas para compilar cada programa
hide: $(SRCS_hide)
	$(CXX) $(CXXFLAGS) $^ -g -o $@ $(LDFLAGS)

reveal: $(SRCS_reveal)
	$(CXX) $(CXXFLAGS) $^ -g -o $@ $(LDFLAGS)

# Limpieza
clean:
	rm -f $(PROGRAMS)

.PHONY: all clean
