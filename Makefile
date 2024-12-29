#	 __         __
#	/  \.-"""-./  \		UrSheet - main file.
#	\    -   -    /		abc19
#	 |   o   o   |		Dec 27 2024
#	 \  .-'''-.  /
#	  '-\__Y__/-'
#	     `---`

objs = ursheet.o solver.o
flags = -Wall -Wextra -Wpedantic -std=c11 -Wno-switch
exec = UrSh

all: $(exec)

$(exec): $(objs)
	gcc	-o $(exec) $(objs)

%.o: %.c
	gcc	-c $< $(flags)

clean:
	rm	-rf $(objs) $(exec)
