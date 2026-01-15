/*
 * SCSerial.cpp
 * hardware interface layer for waveshare serial bus servo
 * date: 2023.6.28
 */

#include "SCSerial.h"
#include "esp_timer.h"

SCSerial::SCSerial()
{
	IOTimeOut = 200;  // 增加超时时间到200ms，给舵机更多响应时间
	uart_num = UART_NUM_MAX;
}

SCSerial::SCSerial(u8 End):SCS(End)
{
	IOTimeOut = 200;  // 增加超时时间到200ms
	uart_num = UART_NUM_MAX;
}

SCSerial::SCSerial(u8 End, u8 Level):SCS(End, Level)
{
	IOTimeOut = 200;  // 增加超时时间到200ms
	uart_num = UART_NUM_MAX;
}

int SCSerial::init(uart_port_t uart_num, int tx_pin, int rx_pin, int baud_rate)
{
	this->uart_num = uart_num;
	
	uart_config_t uart_config = {
		.baud_rate = baud_rate,
		.data_bits = UART_DATA_8_BITS,
		.parity = UART_PARITY_DISABLE,
		.stop_bits = UART_STOP_BITS_1,
		.flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
		.source_clk = UART_SCLK_DEFAULT,
	};
	
	ESP_ERROR_CHECK(uart_driver_install(uart_num, 1024, 1024, 0, NULL, 0));
	ESP_ERROR_CHECK(uart_param_config(uart_num, &uart_config));
	ESP_ERROR_CHECK(uart_set_pin(uart_num, tx_pin, rx_pin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
	
	return 0;
}

int SCSerial::readSCS(unsigned char *nDat, int nLen)
{
	int Size = 0;
	unsigned char ComData;
	int64_t t_start = esp_timer_get_time() / 1000; // Convert to milliseconds, record start time once
	int64_t t_user;
	
	while(Size < nLen){
		// Use blocking read with small timeout to avoid busy waiting
		int len = uart_read_bytes(uart_num, &ComData, 1, pdMS_TO_TICKS(10));
		if(len > 0){
			if(nDat){
				nDat[Size] = ComData;
			}
			Size++;
		}
		
		// Check total timeout
		t_user = (esp_timer_get_time() / 1000) - t_start;
		if(t_user > IOTimeOut){
			break;
		}
	}
	return Size;
}

int SCSerial::writeSCS(unsigned char *nDat, int nLen)
{
	if(nDat==NULL){
		return 0;
	}
	int len = uart_write_bytes(uart_num, nDat, nLen);
	uart_wait_tx_done(uart_num, portMAX_DELAY);
	return len;
}

int SCSerial::writeSCS(unsigned char bDat)
{
	int len = uart_write_bytes(uart_num, &bDat, 1);
	uart_wait_tx_done(uart_num, portMAX_DELAY);
	return len;
}

void SCSerial::rFlushSCS()
{
	uart_flush_input(uart_num);
}

void SCSerial::wFlushSCS()
{
	uart_wait_tx_done(uart_num, portMAX_DELAY);
}

