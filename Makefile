secondary: secondary.c
	gcc -c secondary.c
	gcc -o receiver receiver.o secondary.o ccitt16.o
	gcc -c primary.c
	gcc -o sender sender.o primary.o ccitt16.o
