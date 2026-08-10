gcc \
    -I$(brew --prefix libpq)/include \
    -L$(brew --prefix libpq)/lib -lpq \
    main.c netio/sock.c netio/listen.c messages.c db/connect.c db/auth.c
./a.out
