all: module client standalone no-debug clean

all-noclean: module client standalone no-debug	

module: 
	gcc -g -DNDEBUG -DNDEBUGFP -c -fPIC -o ailib.o irc_library.c
	gcc -shared -fPIC -o ailib.so ailib.o -lc

client:
	gcc -DNDEBUG -DNDEBUGFP -g irc_ssl_client.c -lpthread -lssl -lcrypto -ldl -o irc_main

standalone:
	gcc -DNDEBUG -DNDEBUGFP -g standalone.c -o ai

no-debug:
	gcc standalone.c -o no_dbg_ai
	gcc irc_ssl_client.c -lpthread -lssl -lcrypto -ldl -o no_dbg_irc_main
	gcc -c -fPIC -o no_dbg_ailib.o irc_library.c
	gcc -shared -fPIC -o no_dbg_ailib.so no_dbg_ailib.o -lc
clean:
	rm *.o

clean-all:
	rm *.o ai irc_main no_dbg_irc_main no_dbg_ai no_dbg_ailib.so ailib.so
