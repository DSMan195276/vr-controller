
#include <stdio.h>
#include <ctype.h>
#include "avr_emulation.h"
#include "WProgram.h"
#include "usb_desc.h"
#include "usb_dev.h"
#include "usb_serial.h"

#include "vr_con.h"
#include "MPU6050_MotionApps.h"

#if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
    #include "Wire.h"
#endif

static MPU6050 mpu;

// MPU control/status vars
uint8_t mpuIntStatus;   // holds actual interrupt status byte from MPU
uint8_t devStatus;      // return status after each device operation (0 = success, !0 = error)
uint16_t packetSize;    // expected DMP packet size (default is 42 bytes)
uint16_t fifoCount;     // count of all bytes currently in FIFO
uint8_t fifoBuffer[64]; // FIFO storage buffer

// orientation/motion vars
Quaternion q;           // [w, x, y, z]         quaternion container
VectorInt16 aa;         // [x, y, z]            accel sensor measurements
VectorInt16 aaReal;     // [x, y, z]            gravity-free accel sensor measurements
VectorInt16 aaWorld;    // [x, y, z]            world-frame accel sensor measurements
VectorFloat gravity;    // [x, y, z]            gravity vector
float euler[3];         // [psi, theta, phi]    Euler angle container
float ypr[3];           // [yaw, pitch, roll]   yaw/pitch/roll container and gravity vector


volatile bool mpuHadInterrupt = false;

void mpuDMPInterrupt(void) {
    mpuHadInterrupt = true;
}

void vr_run(void)
{
    Wire.begin();
    TWBR = 24;

    mpu.initialize();

    devStatus = mpu.dmpInitialize();

    serial_printf("DMP status: %d\n", devStatus);

    if (devStatus != 0) {
        serial_printf("ERROR: DMP not enabled\n");
        return ;
    }

    mpu.setDMPEnabled(true);
    attachInterrupt(MPU6050_INT_PIN, mpuDMPInterrupt, RISING);

    mpuIntStatus = mpu.getIntStatus();
    packetSize = mpu.dmpGetFIFOPacketSize();

    serial_printf("DMP Packet size: %d\n", packetSize);

    while (1) {
        // wait for MPU interrupt or extra packet(s) available
        while (!mpuHadInterrupt && fifoCount < packetSize) {
            // other program behavior stuff here
            // .
            // .
            // .
            // if you are really paranoid you can frequently test in between other
            // stuff to see if mpuInterrupt is true, and if so, "break;" from the
            // while() loop to immediately process the MPU data
            // .
            // .
            // .
        }

        // reset interrupt flag and get INT_STATUS byte
        mpuHadInterrupt = false;
        mpuIntStatus = mpu.getIntStatus();

        // get current FIFO count
        fifoCount = mpu.getFIFOCount();

        // check for overflow (this should never happen unless our code is too inefficient)
        if ((mpuIntStatus & 0x10) || fifoCount == 1024) {
            // reset so we can continue cleanly
            mpu.resetFIFO();
            Serial.println(F("FIFO overflow!"));

        // otherwise, check for DMP data ready interrupt (this should happen frequently)
        } else if (mpuIntStatus & 0x02) {
            // wait for correct available data length, should be a VERY short wait
            while (fifoCount < packetSize) fifoCount = mpu.getFIFOCount();

            // read a packet from FIFO
            mpu.getFIFOBytes(fifoBuffer, packetSize);
            
            // track FIFO count here in case there is > 1 packet available
            // (this lets us immediately read more without waiting for an interrupt)
            fifoCount -= packetSize;

            // display quaternion values in easy matrix form: w x y z
            mpu.dmpGetQuaternion(&q, fifoBuffer);


            mpu.dmpGetAccel(&aa, fifoBuffer);

            float l = sqrt(aa.x * aa.x + aa.y * aa.y + aa.z * aa.z);
            Serial.print(l);

            mpu.dmpGetGravity(&gravity, &q);
            mpu.dmpGetLinearAccel(&aaReal, &aa, &gravity);
            mpu.dmpGetLinearAccelInWorld(&aaWorld, &aaReal, &q);

            Serial.print(q.w);
            Serial.print(" ");
            Serial.print(q.x);
            Serial.print(" ");
            Serial.print(q.y);
            Serial.print(" ");
            Serial.print(q.z);
            Serial.print(" ");

            Serial.print(gravity.x);
            Serial.print(" ");
            Serial.print(gravity.y);
            Serial.print(" ");
            Serial.print(gravity.z);
            Serial.print(" ");

            Serial.print(aa.x);
            Serial.print(" ");
            Serial.print(aa.y);
            Serial.print(" ");
            Serial.print(aa.z);
            Serial.print(" ");

            Serial.print(aaReal.x);
            Serial.print(" ");
            Serial.print(aaReal.y);
            Serial.print(" ");
            Serial.print(aaReal.z);
            Serial.print(" ");

            Serial.println(millis());

            #ifdef OUTPUT_READABLE_EULER
                // display Euler angles in degrees
                mpu.dmpGetQuaternion(&q, fifoBuffer);
                mpu.dmpGetEuler(euler, &q);
                Serial.print("euler\t");
                Serial.print(euler[0] * 180/M_PI);
                Serial.print("\t");
                Serial.print(euler[1] * 180/M_PI);
                Serial.print("\t");
                Serial.println(euler[2] * 180/M_PI);
            #endif

            #ifdef OUTPUT_READABLE_YAWPITCHROLL
                // display Euler angles in degrees
                mpu.dmpGetQuaternion(&q, fifoBuffer);
                mpu.dmpGetGravity(&gravity, &q);
                mpu.dmpGetYawPitchRoll(ypr, &q, &gravity);
                Serial.print("ypr\t");
                Serial.print(ypr[0] * 180/M_PI);
                Serial.print("\t");
                Serial.print(ypr[1] * 180/M_PI);
                Serial.print("\t");
                Serial.println(ypr[2] * 180/M_PI);
            #endif

            #ifdef OUTPUT_READABLE_REALACCEL
                // display real acceleration, adjusted to remove gravity
                mpu.dmpGetQuaternion(&q, fifoBuffer);
                mpu.dmpGetAccel(&aa, fifoBuffer);
                mpu.dmpGetGravity(&gravity, &q);
                mpu.dmpGetLinearAccel(&aaReal, &aa, &gravity);
                Serial.print("areal\t");
                Serial.print(aaReal.x);
                Serial.print("\t");
                Serial.print(aaReal.y);
                Serial.print("\t");
                Serial.println(aaReal.z);
            #endif

            #ifdef OUTPUT_READABLE_WORLDACCEL
                // display initial world-frame acceleration, adjusted to remove gravity
                // and rotated based on known orientation from quaternion
                mpu.dmpGetQuaternion(&q, fifoBuffer);
                mpu.dmpGetAccel(&aa, fifoBuffer);
                mpu.dmpGetGravity(&gravity, &q);
                mpu.dmpGetLinearAccel(&aaReal, &aa, &gravity);
                mpu.dmpGetLinearAccelInWorld(&aaWorld, &aaReal, &q);
                Serial.print("aworld\t");
                Serial.print(aaWorld.x);
                Serial.print("\t");
                Serial.print(aaWorld.y);
                Serial.print("\t");
                Serial.println(aaWorld.z);
            #endif
        
            #ifdef OUTPUT_TEAPOT
                // display quaternion values in InvenSense Teapot demo format:
                teapotPacket[2] = fifoBuffer[0];
                teapotPacket[3] = fifoBuffer[1];
                teapotPacket[4] = fifoBuffer[4];
                teapotPacket[5] = fifoBuffer[5];
                teapotPacket[6] = fifoBuffer[8];
                teapotPacket[7] = fifoBuffer[9];
                teapotPacket[8] = fifoBuffer[12];
                teapotPacket[9] = fifoBuffer[13];
                Serial.write(teapotPacket, 14);
                teapotPacket[11]++; // packetCount, loops at 0xFF on purpose
            #endif

            // blink LED to indicate activity
            //blinkState = !blinkState;
            //digitalWrite(LEDPIN, blinkState);
        }

    }
}

