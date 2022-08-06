.PHONY: all

all: main.exe cestimate.exe caida.exe

main.exe: main.cpp rainbow.hpp debug-table.hpp rainbow-four-one.hpp four-one.hpp Makefile
	$(CXX) main.cpp -O3 -ggdb3 -W -Wall -Wextra -o main.exe

cestimate.exe: cestimate.cpp four-one.hpp Makefile
	$(CXX) cestimate.cpp -O3 -ggdb3 -W -Wall -Wextra -o cestimate.exe

caida.exe: caida.cpp four-one.hpp Makefile
	$(CXX) caida.cpp -O3 -fno-strict-aliasing -ggdb3 -W -Wall -Wextra -o caida.exe
