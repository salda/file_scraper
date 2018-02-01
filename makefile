CFLAGS  = -g -Wall -O2

file_scraper:  file_scraper.cpp
	g++ $(CFLAGS) -o file_scraper.out file_scraper.cpp -lcurl -lmyhtml -lstdc++fs -lpthread

clean: 
	$(RM) file_scraper.out *.o *~