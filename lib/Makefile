
all: 
	$ gcc -c -fpic lib.c lib.h ./partitions/*.c ./partitions/*.h ./read_be/*.c ./read_be/*.h ./hfsp/*.c ./hfsp/*.h
	$ mv ./*.o builds/
	
clean: 
	$(RM) main ./builds/*.o
	
create_lib:
	$ gcc -shared -o lib.so ./builds/*.o -lm 
