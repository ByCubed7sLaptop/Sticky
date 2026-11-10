g++ -std=c++17 -O2 -fPIC sticky.cpp Utils.cpp \
    $(pkg-config --cflags --libs Qt6Widgets) \
    -o sticky
