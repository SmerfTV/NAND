
#Flagi kompilacji 
KOMPILATOR=gcc
FLAGI_KOMPILACJI_BIBLIOTEKI=-Wall -Wextra -Wno-implicit-fallthrough -std=gnu17 -fPIC -O2
FLAGI_LINKOWANIA_BIBLIOTEKI=-shared -Wl,--wrap=malloc -Wl,--wrap=calloc -Wl,--wrap=realloc -Wl,--wrap=reallocarray -Wl,--wrap=free -Wl,--wrap=strdup -Wl,--wrap=strndup

#Pliki zrodlowe biblioteki
PLIKI_ZRODLOWE=nand.c memory_tests.c

#Pliki biblioteki po skompilowaniu
SKOMPILOWANE=$(PLIKI_ZRODLOWE:.c=.o)

#Pliki naglowkowe biblioteki
NAGLOWKI=nand.h memory_tests.h

#Plik, do ktorego bedzie sie kompilowac biblioteka
BIBLIOTEKA=libnand.so

.PHONY: all clean

all: $(BIBLIOTEKA)

$(BIBLIOTEKA): $(SKOMPILOWANE)
	$(KOMPILATOR) $(FLAGI_LINKOWANIA_BIBLIOTEKI) -o $@ $^

%.o: %.c $(NAGLOWKI)
	$(KOMPILATOR) $(FLAGI_KOMPILACJI_BIBLIOTEKI) -c -o $@ $<

clean:
	rm -f $(SKOMPILOWANE) $(BIBLIOTEKA)