gcc \
    -I/opt/homebrew/opt/libpq/include \
    -L/opt/homebrew/opt/libpq/lib -lpq \
    main.c -g -O0
./a.out
