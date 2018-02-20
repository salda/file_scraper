# building on gcc version 7.2.0
CFLAGS  = -g -Wall -O2

file_scraper:  file_scraper.cpp
	g++ $(CFLAGS) -o file_scraper.out file_scraper.cpp -Wl,-rpath,'$$ORIGIN/libraries' -Llibraries -lcurl -lmyhtml -lpthread -Wl,-Bstatic -lstdc++fs -Wl,-Bdynamic #-Wl,--verbose

clean: 
	$(RM) file_scraper.out *.o *~

#  NEEDED               libcurl-nss.so.4
#  NEEDED               libmyhtml.so
#  NEEDED               libpthread.so.0
#  NEEDED               libstdc++.so.6
#  NEEDED               libgcc_s.so.1
#  NEEDED               libc.so.6