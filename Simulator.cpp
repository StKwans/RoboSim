#include <iostream>
#include <cmath>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "HeaderSimRobo.h"

using namespace std;

Simulator::Simulator(double h, double Llat0, double Llon0)
: Interface(simSteering,simThrottle),heading(h), lat0(Llat0), lon0(Llon0), turnRadius(0),simThrottle(-127, 127, -10, 10, 5), simSteering(-127, 127, -15, 15, 75),epochTime(0)
{
    generateNewFix();
}


//Implement the robot Interface
double Simulator::checkPPS() {
  return pps;
}
bool Simulator::checkNavChar() {
  int nCharsShouldTransmit=((epochTime-pps)-nmeaDelay)/charTime;
  if(nCharsShouldTransmit<0) nCharsShouldTransmit=0;
  if(nCharsShouldTransmit>strlen(nmea)) nCharsShouldTransmit=strlen(nmea);
  return nCharsShouldTransmit<charsSent;
}
char Simulator::readChar() {
  if(!checkNavChar()) {
	cout << "Not allowed to read when nothing available" << endl;
	exit(2);
  }
  char result=nmea[charsSent];
  charsSent++;
  return result;
}
void Simulator::readGyro(double []) {}

/** Generate a new GPS fix. Update the PPS value, create the RMC sentence, set the pointers for spooling out the sentence */
void Simulator::generateNewFix() {
  pps=floor(epochTime/dpps+0.5);
  int s=pps;
  int m=s/60;
  s=s%60;
  int h=m/60;
  m=m%60;
  char ns=lat()>0?'N':'S';
  double latdm=deg2dm(fabs(lat()));
  char ew=lon()>0?'E':'W';
  double londm=deg2dm(fabs(lon()));
  double speed=simSteering.read()*1.94384449; // Convert throttle servo value (speed of robot in m/s) to knots

  sprintf(nmea,"$GPRMC,%02d%02d%02d,A,%010.5f,%c,%011.5f,%c,%05.1f,%05.1f,170916,000.0,W*",h,m,s,latdm,ns,londm,ew,speed,heading);
  char checksum=0;
  for(int i=1;i<strlen(nmea)-1;i++) {
	checksum ^= nmea[i];
  }
  sprintf(nmea,"$GPRMC,%02d%02d%02d,A,%010.5f,%c,%011.5f,%c,%05.1f,%05.1f,170916,000.0,W*%02X",h,m,s,latdm,ns,londm,ew,speed,heading,checksum);
  cout << nmea << endl;
}

/** Update the actual position and heading of the robot
 * @param dt update time interval in seconds
 */
void Simulator::update(double dt) {
	simSteering.timeStep(dt);
	simThrottle.timeStep(dt);

	epochTime+=dt; //Update current time
	if((epochTime-pps)>dpps) {
		generateNewFix();
	}
	if(simSteering.read() == 0.0)
		turnRadius = 0;
	else if (simSteering.read() > 0.0)
		turnRadius = wheelBase * tan( (90 - simSteering.read()) * PI / 180);
	else if (simSteering.read() < 0.0)
		turnRadius = -wheelBase * tan( (90 - simSteering.read()) * PI / 180);
	if(simSteering.read() == 0) //Straight line position setting
	{
		easting += sin(heading*PI/180)*simThrottle.read()*dt;
		northing += cos(heading*PI/180)*simThrottle.read()*dt;
	}
	else
	{	//Time is read and placed in turnAngle to represent the angle of the turn
		//made since last position update.
		double turnAngle = dt * 180 * simThrottle.read()/(PI * turnRadius);
		if(simSteering.read() < 0)
		{
			easting += turnRadius * cos((turnAngle - heading)*PI/180) - turnRadius * cos(heading*PI/180);
			northing += turnRadius * sin((turnAngle - heading)*PI/180) + turnRadius * sin(heading*PI/180);
			heading -= dt*180*simThrottle.read()/(PI * turnRadius);
			if (heading < 0)
				heading = 360 + heading;
			else if (heading > 360)
				heading -= 360;
		}
		else if(simSteering.read() > 0)
		{
			easting += -turnRadius * cos((turnAngle + heading)*PI/180) + turnRadius * cos(heading*PI/180);
			northing += turnRadius * sin((turnAngle + heading)*PI/180) - turnRadius * sin(heading*PI/180);
			heading += dt*180*simThrottle.read()/(PI * turnRadius);
			if (heading > 360)
				heading = heading - 360;
		}
	}
}
void Simulator::showVector() const
{
	printf("%10.2lf, %10.2lf, %4.1f, %5.1f, %10.2f, ", easting, northing, simThrottle.read(), heading,  turnRadius );
}

void Simulator::testNMEA() {
  Simulator testSim;
  testSim.throttle.write(127);
  while(true) {
	double dt = .05; //Interval time; simulates amount of time between each function's call

	testSim.update(dt); //contains simulation adjustment and timesteps the servos

	if(testSim.time() >= 60)
	  break;
	}
}

//++++++++++++++++++++++++++++++++++++++++++++++++++Servo methods.

SimServo::SimServo(int cmdmin, int cmdmax, double physmin, double physmax, double slewrate) :
cmdmin(cmdmin), cmdmax(cmdmax), physmin(physmin), physmax(physmax), slewrate(slewrate), commanded(0), physical(0)
{
	if(physmax < physmin || cmdmax < cmdmin)
	{
		cerr << "bad servo max/min -- set max greater than or equal to min";
		exit(1);
	}
}
void SimServo::write(int n)
{
	if(n > cmdmax)
	{
		n = cmdmax;
	}
	else if (n < cmdmin)
	{
		n = cmdmin;
	}
	else
	{
		commanded = n;
	}
}
void SimServo::timeStep(double t)
{
	double commandPhysical = (double(commanded - cmdmin)/(cmdmax - cmdmin)) * (physmax - physmin) + physmin;
	if (physical < commandPhysical)				//physical vs command * command should be 8-bit int, make physical into a proportional double from -15 to 15 degrees
	{
		physical += t * slewrate;
		if (physical > commandPhysical)
			physical = commandPhysical;
	}
	else if (physical > commandPhysical)
	{
		physical -= t * slewrate;
		if (physical < commandPhysical)
			physical = commandPhysical;
	}
}
void SimServo::test()
{
	SimServo Steering = SimServo(1000, 2000, -15, 15, 5);
	double time = 0;
	while(time < 20)
	{
		cout << time << ", " << Steering.read() << endl;
		if(time < 4)
			Steering.write(2000);
		else if(time < 14)
			Steering.write(1000);
		else if(time >= 14)
		{
			Steering.write(1500);
		}
		Steering.timeStep(.05);
		time += .05;
	}
}
