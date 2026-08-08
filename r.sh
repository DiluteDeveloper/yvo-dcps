gcc \
    -I$(brew --prefix libpq)/include \
    -L$(brew --prefix libpq)/lib -lpq \
    main.c netio/sock.c netio/listen.c
./a.out
