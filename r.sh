gcc \
    -I$(brew --prefix libpq)/include \
    -L$(brew --prefix libpq)/lib -lpq \
    main.c 
./a.out
