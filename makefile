all: mkbins module client standalone no-debug clean

all-noclean: module client standalone no-debug	

mkbins:
	mkdir -p bins/

module: 
	gcc -g -DNDEBUG -DNDEBUGFP -c -fPIC -o ailib.o irc_library.c
	gcc -g -shared -fPIC -o bins/ailib.so ailib.o -lc


client:
	gcc -DNDEBUG -DNDEBUGFP -g irc_ssl_client.c -lpthread -lssl -lcrypto -ldl -o bins/irc_main

standalone:
	gcc -DNDEBUG -DNDEBUGFP -g standalone.c -o bins/ai

no-debug:
	gcc standalone.c -o bins/no_dbg_ai
	gcc irc_ssl_client.c -lpthread -lssl -lcrypto -ldl -o bins/no_dbg_irc_main
	gcc -c -fPIC -o no_dbg_ailib.o irc_library.c
	gcc -shared -fPIC -o bins/no_dbg_ailib.so no_dbg_ailib.o -lc
clean:
	rm *.o

clean-all:
	rm -r *.o bins/*
