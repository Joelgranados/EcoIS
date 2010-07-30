CFLAGS=`pkg-config --cflags opencv`
LIBS=`pkg-config --libs opencv`

all: ctag
	g++ ${CFLAGS} ${LIBS} -g imageadjust.cpp ia_input.cpp -o imageadjust

ctag:
	ctags -R *

clean:
	rm -rf imageadjust imageadjust.tar.gz
