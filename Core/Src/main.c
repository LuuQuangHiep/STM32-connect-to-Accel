#include "main.h"
#include <stdint.h>

#define I2C1_BASE_ADDR 0x40005400
#define GPIOB_BASE_ADDR 0x40020400

void I2C_Init()
{
	__HAL_RCC_GPIOB_CLK_ENABLE();
	uint32_t* GPIOB_MODER = (uint32_t*)(GPIOB_BASE_ADDR + 0x00);
	*GPIOB_MODER &= ~(0b11 << 12)| (0b11 << 18);
	*GPIOB_MODER |= (0b10 << 12) |(0b10 << 18);

	uint32_t* GPIOB_AFRL = (uint32_t*)(GPIOB_BASE_ADDR + 0x20);
	uint32_t* GPIOB_AFHL = (uint32_t*)(GPIOB_BASE_ADDR + 0x24);
	*GPIOB_AFRL &= ~(0b1111 << 24);
	*GPIOB_AFRL |= (4 << 24);

	*GPIOB_AFHL &= ~(0b1111 << 4);
	*GPIOB_AFHL |= (4 << 4);

	__HAL_RCC_I2C1_CLK_ENABLE();
	uint32_t* I2C1_CR1 = (uint32_t*)(I2C1_BASE_ADDR + 0x00);
	uint32_t* I2C1_CR2 = (uint32_t*)(I2C1_BASE_ADDR + 0x04);
	uint32_t* I2C1_CCR = (uint32_t*)(I2C1_BASE_ADDR + 0x1C);

	*I2C1_CR1 &= ~(0b1 << 0);
	*I2C1_CR2 |= 16;
	*I2C1_CCR |= 80;
	*I2C1_CR1 |= (0b1 << 0);

}

uint8_t I2C_WriteData(uint8_t reg, uint8_t val)
{
	uint16_t* CR1 = (uint16_t*)(I2C1_BASE_ADDR + 0x00);
	uint16_t* DR  = (uint16_t*)(I2C1_BASE_ADDR + 0x10);
	uint16_t* SR1 = (uint16_t*)(I2C1_BASE_ADDR + 0x14);
	uint16_t* SR2 = (uint16_t*)(I2C1_BASE_ADDR + 0x18);

	uint16_t temp;

	*CR1 |= 1 << 8;							//gửi start bit
	while(((*SR1 >> 0) &1) != 1);			// chờ kiểm tra start bit gửi thành công

	uint8_t accel_addr_w = (0b0011001 << 1)|0;	// gửi địa chỉ SLAVE và bit write
	*DR = accel_addr_w;						// ghi dữ liệu vào thanh ghi data

	while(((*SR1 >> 1) &1) != 1);			// kiểm tra địa chỉ và bit write gửi thành công

	temp = *SR2;							// đọc thanh ghi SR2 để hệ thống reset Bit ADDR

	if((*SR1 >> 10) == 1)
	{											// kiểm tra Slave có gửi ACK về hay không
		*SR1 &= ~(1 << 10);					// CLEAR bit AF
		*CR1 |= 1 << 9;						// gửi Stop bit
		return 0;
	}

	*DR = reg;								// gửi địa chỉ thanh ghi
	while(((*SR1 >> 2)&1) != 1);			// kiểm tra data địa chỉ thanh ghi được gửi thành công

	*DR = val;								// gửi dữ liệu chính
	while(((*SR1 >> 2)&1) != 1);			// kiểm tra dữ liệu được gửi hoàn tất

	*CR1 |= 1 << 9;							// gửi stop bit

	return temp;
}


uint8_t I2C_Read(uint8_t reg)
{
	uint16_t* CR1 = (uint16_t*)(I2C1_BASE_ADDR + 0x00);
	uint16_t* SR1 = (uint16_t*)(I2C1_BASE_ADDR + 0x14);
	uint16_t* DR  = (uint16_t*)(I2C1_BASE_ADDR + 0x10);
	uint16_t* SR2 = (uint16_t*)(I2C1_BASE_ADDR + 0x18);
	uint16_t temp;

	*CR1 |= 1 << 8;									// gửi start bit
	while(((*SR1 >> 0) &1) != 1);					// chờ start bit được gửi thành công

	uint8_t accelwrite = (0b0011001 << 1) |0;		// địa chỉ I2C + bit write
	*DR = accelwrite;								// gửi địa chỉ I2C + bit write

	while(((*SR1 >> 1) &1) != 1);					// kiểm tra xem địa chỉ I2C đã được gửi
	temp = *SR2;									// đọc địa chỉ I2C vừa nhận để ngắt việc truyền data

	if((*SR1 >> 10) == 1)								// kiểm tra Slave có gửi ACK về hay không
	{
		*SR1 &= ~(1 << 10);							// clear bit AF
		*CR1 |= 1 << 9;								// gửi stop bit
		return 0;
	}

	*DR = reg;
	while(((*SR1 >> 2 )&1) != 1);

	*CR1 |= 1 << 8;
	while(((*SR1 >> 0 )&1) != 1);

	uint8_t accelread = (0b0011001 << 1)| 1;
	*DR = accelread;

	while(((*SR1 >> 1) &1) != 1);
	temp = *SR2;

	while(((*SR1 >> 6) &1) != 1);
	temp = *DR;

	*CR1 |= 1 << 9;
	return temp;

}

#define WHO_AM_I  	0x0F
#define CTRL_REG1  	0x20
#define OUT_X_L		0x28
#define OUT_X_H		0x29
#define OUT_Y_L		0x2A
#define OUT_Y_H		0x2B

int16_t x_axis;
int16_t y_axis;

#define ADC_BASE_ADDR 0x40012000

void ADC_Init()
{
	__HAL_RCC_ADC1_CLK_ENABLE();
	uint32_t* ADC1_CCR = (uint32_t*)(ADC_BASE_ADDR + 0x300 + 0x04);
	*ADC1_CCR |= 1 << 23;					//set enable cho temp sensor

	uint32_t* ADC1_SMPR1 =(uint32_t*)(ADC_BASE_ADDR + 0x0C);		//SET sample time cho ADC
	*ADC1_SMPR1 |= 0b111 << 18;

	uint32_t* ADC1_JSQR = (uint32_t*)(ADC_BASE_ADDR + 0x38);
	*ADC1_JSQR &= ~(0b11 << 20);		// SET conversion

	*ADC1_JSQR |= 16 << 15;     		// SET JSQR4 thành temp seson

	uint32_t* ADC1_CR2 = (uint32_t*)(ADC_BASE_ADDR +0x08);
	*ADC1_CR2 |= 1 << 0;


}

uint16_t ADC_Read()
{
	uint32_t* CR2 = (uint32_t*)(ADC_BASE_ADDR + 0x08);
	*CR2 |= 1 << 22;

	uint32_t* SR = (uint32_t*)(ADC_BASE_ADDR + 0x00);
	while(((*SR >> 2) &1) == 0);
	*SR &= ~ (1 << 2);

	uint32_t* JDR1 =(uint32_t*)(ADC_BASE_ADDR + 0x3C );
	return JDR1;
}
int main()
{
	HAL_Init();
	I2C_Init();
	uint8_t accel_id = I2C_Read(WHO_AM_I);
	uint8_t temp = I2C_Read(CTRL_REG1);
	I2C_WriteData(CTRL_REG1, 0x53);
	temp = I2C_Read(CTRL_REG1);
	(void) accel_id;
	(void) temp;

	while(1)
	{
		x_axis = (I2C_Read(OUT_X_H) << 8)|I2C_Read(OUT_X_L);
		y_axis = (I2C_Read(OUT_Y_H) << 8)|I2C_Read(OUT_Y_L);
		HAL_Delay(100);

	}
	return 0;
}
