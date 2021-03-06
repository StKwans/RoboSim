#include <time.h>
#include "HardwarePi.h"
#include <wiringPi.h>
#include <fcntl.h>


/**
 * @copydoc Servo::write(int)
 * \internal
 * Implemented by writing to the correct registers in the Arduino serving as the odometer. The value written is a 16-bit unsigned integer
 * written little-endian to address 0x2C/0x2D or 0x2E/0x2F. This number is interpreted as the pulse width to use in 10us units. There is a software stop at 100 units and 200 units.
 */
void HardwarePiServoArduino::write(int n) {
  if(n<100) n=100;
  if(n>200) n=200;
  writeI2Creg_le<uint16_t>(bus,ADDRESS,0x2C+2*channel,n);
}

/**
 * @copydoc Interface::checkPPS()
 * \internal
 * Implemented using the LinuxPPS driver, which in turn implements RFC2783, Pulse-Per-Second API. If the interface epoch isn't valid
 * at the first PPS, then the epoch is set to the time of the PPS.
 *
 */
double HardwarePiInterface::checkPPS() {
//  pps_info_t info;
//  static const struct timespec timeout={0,0};
//  time_pps_fetch(pps,PPS_TSFMT_TSPEC,&info,&timeout);
//  double t=ts2t(info.assert_timestamp);
//  if(t0<t) t0=t;
//  return t-t0;
	return 0; //Comment out PPS stuff because no PPS source is plugged in
}

/** Makes sure that GPS buffer has data to read. If there is still data in the buffer that hasn't been spooled out, return immediately. If
 * we have read to the end of the buffer, then try to read a full buffer, but set the length of data left to the amount we actually read.
 * The file has been opened with O_NONBLOCK so it should return immediately even if there isn't a full buffer worth of data to read.
 */
void HardwarePiInterface::fillGpsBuf() {
//  if(gpsPtr<gpsLen) return;
//  gpsLen=fread(gpsBuf,1,sizeof(gpsBuf),gpsf);
//  gpsPtr=0;
}

/**
 * @copydoc Interface::checkNavChar()
 * \internal
 * Implemented with an internal buffer. First, the buffer is filled if necessary with fillGpsBuf. Then, return true if there is more than 1 byte in the buffer.
 */
bool HardwarePiInterface::checkNavChar() {
  fillGpsBuf(); //Make sure the buffer has data in it
  return gpsLen!=0;
}

/**
 * @copydoc Interface::readChar()
 * \internal
 * Implemented with an internal buffer. First, the buffer is filled if necessary with
 * fillGpsBuf. Then, return the character pointed to by the buffer read pointer, and
 * increment that pointer. If checkNavChar would return false, this will read the
 * character at index 0 in the buffer, which is old data.
 */
char HardwarePiInterface::readChar() {
  fillGpsBuf();
  char result=gpsBuf[gpsPtr];
  gpsPtr++;
  return result;
}

/**
 * @copydoc Interface::time()
 * \internal
 * Implemented by reading the system clock, then subtracting off the epoch of the first time the clock was read. If this *is* the first time
 *  the clock was read, records this time as the epoch in t0.
 */
double HardwarePiInterface::time() {
  struct timespec ts;
  clock_gettime(CLOCK_REALTIME,&ts);
  double t=ts2t(ts);
  if(t0<0) t0=t;
  return t-t0;
}

/** @copydoc Interface::button(int)
 *  \internal
 *  Since we use WiringPiSetupGpio(), we use the Broadcom numbers. This happens to match the numbers printed
 *  on our Pi header.
 */
bool HardwarePiInterface::button(int pin) {
  return 0==digitalRead(pin);
}

void HardwarePiInterface::readOdometer(uint32_t &timeStamp, int32_t &wheelCount, uint32_t &dt) {
  ioctl(bus,I2C_SLAVE,ODOMETER_ADDRESS);
  char buf[0x0C];
  buf[0]=0x00;
  write(bus,buf,1);
  read(bus,buf,12);
  wheelCount=readBuf_le<int32_t>(buf,0);
  dt        =readBuf_le<uint32_t>(buf,4);
  timeStamp =readBuf_le<uint32_t>(buf,8);
}

bool HardwarePiInterface::readGyro(int16_t g[], int16_t& t) {
  return mpu.readGyro(g[0],g[1],g[2],t);
}

bool HardwarePiInterface::readGyro(int16_t g[]) {
  return mpu.readGyro(g[0],g[1],g[2]);
}

HardwarePiInterface::HardwarePiInterface(Servo& Lsteering, Servo& Lthrottle):Interface(Lsteering,Lthrottle),t0(-1.0) {
  //Setup for GPIO (for buttons)
  wiringPiSetupGpio();
//  ppsf=fopen("/dev/pps0","r");
//  time_pps_create(fileno(ppsf), &pps);
  //Open the I2C bus
  bus=open("/dev/i2c-1",O_RDWR);
  if(bus<0) printf("Couldn't open bus: errno %d",errno);

  //Initialize the MPU9250
  mpu.begin(bus);
//  int gps=open("/dev/ttyAMA0",O_RDONLY | O_NONBLOCK);
//  gpsf=fdopen(gps,"rb");
}

HardwarePiInterface::~HardwarePiInterface() {
//  time_pps_destroy(pps);
//  fclose(ppsf);
//  fclose(bus);
}

HardwarePiInterfaceArduino::HardwarePiInterfaceArduino():hardSteering(0),hardThrottle(1),HardwarePiInterface(hardSteering,hardThrottle) {
  hardSteering.begin(bus);
  hardThrottle.begin(bus);
};

HardwarePiInterfaceArduino::~HardwarePiInterfaceArduino() {

}


