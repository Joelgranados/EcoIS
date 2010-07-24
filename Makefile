CFLAGS=`pkg-config --cflags opencv`
LIBS=`pkg-config --libs opencv`


all:
	g++ ${CFLAGS} ${LIBS} -g imageadjust.cpp ia_input.cpp -o imageadjust

debug:
	g++ ${CFLAGS} ${LIBS} -g camcal.cpp -o camcal

cal:
	g++ ${CFLAGS} ${LIBS} calibration.cpp -o calibration

clean:
	rm -rf fcount campos calibration camcal
