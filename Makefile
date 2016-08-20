CC = gcc
CPP = g++

EXES = RoboSim.exe RoboPi.exe buttonTest.exe recordOdometer.exe i2c_echo.exe recordGyro.exe

all: $(EXES)

MainSimRobo.o: MainSimRobo.cpp robot.h Simulator.h roboBrain.h
	${CPP} -g -c -std=c++14 -o $@ $<

Simulator.o: Simulator.cpp robot.h Simulator.h
	${CPP} -g -c -std=c++14 -o $@ $<

roboBrain.o: roboBrain.cpp roboBrain.h robot.h Simulator.h
	${CPP} -g -c -std=c++14 -o $@ $<

#Main executable rule. Dependencies are each .o file, one for each .cpp file, use wildcard rule above to make them.
#Is called .exe by convention, even though it isn't necessary in Unix. Keep the .exe extension to make it easy to identify executables.
RoboSim.exe: MainSimRobo.o Simulator.o roboBrain.o
	${CPP} -g -o $@ $^ # Link in debug mode to an executable, output name from $@, input is all named .o files ($^)

HardwarePi.o: HardwarePi.cpp HardwarePi.h robot.h
	${CPP} -g -I /usr/local/include -c -std=c++14 -o $@ $<

MPU.o: MPU.cpp HardwarePi.h robot.h
	${CPP} -g -c -std=c++14 -o $@ $<

RoboPiMain.o: RoboPiMain.cpp HardwarePi.h robot.h
	${CPP} -g -c -std=c++14 -o $@ $<

OpenLoopGuidance.o: OpenLoopGuidance.cpp OpenLoopGuidance.h HardwarePi.h robot.h
	${CPP} -g -c -std=c++14 -o $@ $<

RoboPi.exe: RoboPiMain.o HardwarePi.o OpenLoopGuidance.o Simulator.o MPU.o
	${CPP} -g -o $@ $^ -L /usr/local/lib -lwiringPi

buttonTest.o: buttonTest.cpp HardwarePi.h robot.h
	${CPP} -g -c -std=c++14 -o $@ $<

recordOdometer.o: recordOdometer.cpp HardwarePi.h robot.h
	${CPP} -g -c -std=c++14 -o $@ $<

recordGyro.o: recordGyro.cpp HardwarePi.h robot.h
	${CPP} -g -c -std=c++14 -o $@ $<

buttonTest.exe: buttonTest.o HardwarePi.o MPU.o
	${CPP} -g -o $@ $^ -L /usr/local/lib -lwiringPi
	
recordOdometer.exe: recordOdometer.o HardwarePi.o MPU.o Simulator.o OpenLoopGuidance.o
	${CPP} -g -o $@ $^ -L /usr/local/lib -lwiringPi

recordGyro.exe: recordGyro.o HardwarePi.o MPU.o Simulator.o OpenLoopGuidance.o
	${CPP} -g -o $@ $^ -L /usr/local/lib -lwiringPi

i2c_echo.exe: i2c_echo.c
	${CC} -g -o $@ $^ -std=c99
	

html: Doxyfile
	doxygen

#Remove all compiled files
clean:
	$(RM) -r $(EXES) *.o html latex
