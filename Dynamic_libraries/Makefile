
all:
	gcc -c -g -O0 -Wall -Werror mn.c -o libmn.o -fPIC
	gcc -shared -o libmn.so libmn.o

	gcc -c -g -O0 -Wall -Werror m16.c -o libm16.o -fPIC
	gcc -shared -o libm16.so libm16.o

	gcc -c -g -O0 -Wall -Werror flib.c -o libsflib.o -fPIC
	gcc -shared -o libflib.so libflib.o

	-sudo unlink /lib/x86_64-linux-gnu/libflib.so
	-sudo ln -s $(shell pwd)/libflib.so /lib/x86_64-linux-gnu/libflib.so

	gcc -g -O0 main.c -o a.out -ldl -L $(shell pwd) -lflib