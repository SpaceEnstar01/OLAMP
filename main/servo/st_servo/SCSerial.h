/*
 * SCSerial.h
 * hardware interface layer for waveshare serial bus servo
 * date: 2023.6.28 
 */

#ifndef _SCSERIAL_H
#define _SCSERIAL_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include <stdint.h>
#include "SCS.h"

class SCSerial : public SCS
{
public:
	SCSerial();
	SCSerial(u8 End);
	SCSerial(u8 End, u8 Level);
	
	// Initialize UART for ESP-IDF
	int init(uart_port_t uart_num, int tx_pin, int rx_pin, int baud_rate);

protected:
	virtual int writeSCS(unsigned char *nDat, int nLen);//output nLen byte
	virtual int readSCS(unsigned char *nDat, int nLen);//input nLen byte
	virtual int writeSCS(unsigned char bDat);//output 1 byte
	virtual void rFlushSCS();//
	virtual void wFlushSCS();//
public:
	uint32_t IOTimeOut;//I/O timeout
	uart_port_t uart_num;//UART port number
	int Err;
public:
	virtual int getErr(){  return Err;  }
};

#endif

