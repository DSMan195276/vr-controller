#include <stdio.h>
#include <ctype.h>
#include <stdint.h>
#include "avr_emulation.h"
#include "WProgram.h"
#include "usb_desc.h"
#include "usb_dev.h"
#include "usb_serial.h"
#include <Wire.h>

#include "vr_con.h"
#include "mpu6050_2.h"

typedef union accel_t_gyro_union
{
    struct
    {
        uint8_t x_accel_h;
        uint8_t x_accel_l;
        uint8_t y_accel_h;
        uint8_t y_accel_l;
        uint8_t z_accel_h;
        uint8_t z_accel_l;
        uint8_t t_h;
        uint8_t t_l;
        uint8_t x_gyro_h;
        uint8_t x_gyro_l;
        uint8_t y_gyro_h;
        uint8_t y_gyro_l;
        uint8_t z_gyro_h;
        uint8_t z_gyro_l;
    } reg;
    struct MPU6050_accelgyro value;
} accel_t_gyro_union;


void MPU6050_init(void)
{
    int error;
    uint8_t c;
    Wire.begin();

    // default at power-up:
    //    Gyro at 250 degrees second
    //    Acceleration at 2g
    //    Clock source at internal 8MHz
    //    The device is in sleep mode.
    //
    error = MPU6050_read (MPU6050_WHO_AM_I, &c, 1);

    // According to the datasheet, the 'sleep' bit
    // should read a '1'.
    // That bit has to be cleared, since the sensor
    // is in sleep mode at power-up.
    error = MPU6050_read (MPU6050_PWR_MGMT_1, &c, 1);

    // Clear the 'sleep' bit to start the sensor.
    MPU6050_write_reg (MPU6050_PWR_MGMT_1, 0);
}

// --------------------------------------------------------
// MPU6050_read
//
// This is a common function to read multiple bytes
// from an I2C device.
//
// It uses the boolean parameter for Wire.endTransMission()
// to be able to hold or release the I2C-bus.
// This is implemented in Arduino 1.0.1.
//
// Only this function is used to read.
// There is no function for a single byte.
//
int MPU6050_read(int start, uint8_t *buffer, int size)
{
    int i, n, error;

    Wire.beginTransmission(MPU6050_I2C_ADDRESS);
    n = Wire.write(start);
    if (n != 1)
        return (-10);

    n = Wire.endTransmission(false);    // hold the I2C-bus
    if (n != 0)
        return (n);

    // Third parameter is true: relase I2C-bus after data is read.
    Wire.requestFrom(MPU6050_I2C_ADDRESS, size, true);
    i = 0;
    while(Wire.available() && i<size)
        buffer[i++]=Wire.read();

    if (i != size)
        return (-11);

    return (0);  // return : no error
}


// --------------------------------------------------------
// MPU6050_write
//
// This is a common function to write multiple bytes to an I2C device.
//
// If only a single register is written,
// use the function MPU_6050_write_reg().
//
// Parameters:
//   start : Start address, use a define for the register
//   pData : A pointer to the data to write.
//   size  : The number of bytes to write.
//
// If only a single register is written, a pointer
// to the data has to be used, and the size is
// a single byte:
//   int data = 0;        // the data to write
//   MPU6050_write (MPU6050_PWR_MGMT_1, &c, 1);
//
int MPU6050_write(int start, const uint8_t *pData, int size)
{
    int n, error;

    Wire.beginTransmission(MPU6050_I2C_ADDRESS);
    n = Wire.write(start);        // write the start address
    if (n != 1)
        return (-20);

    n = Wire.write(pData, size);  // write data bytes
    if (n != size)
        return (-21);

    error = Wire.endTransmission(true); // release the I2C-bus
    if (error != 0)
        return (error);

    return (0);         // return : no error
}

// --------------------------------------------------------
// MPU6050_write_reg
//
// An extra function to write a single register.
// It is just a wrapper around the MPU_6050_write()
// function, and it is only a convenient function
// to make it easier to write a single register.
//
int MPU6050_write_reg(int reg, uint8_t data)
{
    int error;

    error = MPU6050_write(reg, &data, 1);

    return (error);
}

int MPU6050_read_accelgyro(struct MPU6050_accelgyro *accelgyro)
{
    int error;
    uint8_t swap;
    union accel_t_gyro_union accel_t_gyro;

    error = MPU6050_read (MPU6050_ACCEL_XOUT_H, (uint8_t *) &accel_t_gyro, sizeof(accel_t_gyro));

#define SWAP(x,y) swap = x; x = y; y = swap

    SWAP (accel_t_gyro.reg.x_accel_h, accel_t_gyro.reg.x_accel_l);
    SWAP (accel_t_gyro.reg.y_accel_h, accel_t_gyro.reg.y_accel_l);
    SWAP (accel_t_gyro.reg.z_accel_h, accel_t_gyro.reg.z_accel_l);
    SWAP (accel_t_gyro.reg.t_h, accel_t_gyro.reg.t_l);
    SWAP (accel_t_gyro.reg.x_gyro_h, accel_t_gyro.reg.x_gyro_l);
    SWAP (accel_t_gyro.reg.y_gyro_h, accel_t_gyro.reg.y_gyro_l);
    SWAP (accel_t_gyro.reg.z_gyro_h, accel_t_gyro.reg.z_gyro_l);

    memcpy(accelgyro, &accel_t_gyro.value, sizeof(*accelgyro));

    return error;
}

