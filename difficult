#include <reg51.h>
#include <intrins.h>
#include <stdio.h>

// ================= 引脚定义 =================
// ADC0832
sbit ADC_CS  = P1^0;
sbit ADC_CLK = P1^1;
sbit ADC_DI  = P1^2;
sbit ADC_DO  = P1^3;

// LCD1602 (直插引脚)
sbit LCD_RS = P2^6;
sbit LCD_E  = P2^7;
#define LCD_DataPort P0

// 报警器件
sbit LED  = P2^0;
sbit BEEP = P2^5;

// ================= 基础函数 =================
void Delay_ms(unsigned int ms) {
    unsigned int i, j;
    for(i=0; i<ms; i++)
        for(j=0; j<110; j++);
}

void Beep_Sound(unsigned int duration) {
    unsigned int i;
    for(i=0; i<duration*2; i++) {
        BEEP = ~BEEP;
        Delay_ms(1);
    }
    BEEP = 1;
}

// ================= LCD1602 驱动 =================
void LCD_WriteCmd(unsigned char cmd) {
    LCD_RS = 0; LCD_DataPort = cmd; LCD_E = 1; _nop_(); _nop_(); LCD_E = 0; Delay_ms(2);
}
void LCD_WriteData(unsigned char dat) {
    LCD_RS = 1; LCD_DataPort = dat; LCD_E = 1; _nop_(); _nop_(); LCD_E = 0; Delay_ms(2);
}
void LCD_Init(void) {
    Delay_ms(15); LCD_WriteCmd(0x38); LCD_WriteCmd(0x0C); LCD_WriteCmd(0x06); LCD_WriteCmd(0x01); Delay_ms(5);
}
void LCD_ShowString(unsigned char row, unsigned char col, unsigned char *str) {
    unsigned char addr = (row == 1) ? (0x00 + col - 1) : (0x40 + col - 1);
    LCD_WriteCmd(addr | 0x80); 
    while(*str != '\0') LCD_WriteData(*str++);
}

// ================= ADC0832 驱动 =================
unsigned char ADC0832_Read(void) {
    unsigned char i, dat = 0;
    ADC_CS = 0; ADC_CLK = 0;
    // 发送起始位和通道选择
    ADC_DI = 1; _nop_(); ADC_CLK = 1; _nop_(); ADC_CLK = 0;
    ADC_DI = 1; _nop_(); ADC_CLK = 1; _nop_(); ADC_CLK = 0;
    ADC_DI = 0; _nop_(); ADC_CLK = 1; _nop_(); ADC_CLK = 0;
    
    ADC_DI = 1; // 释放数据线
    for(i=0; i<20; i++) _nop_(); 
    
    for(i=0; i<8; i++) {
        ADC_CLK = 1; _nop_();
        dat <<= 1;
        if(ADC_DO) dat |= 0x01;
        ADC_CLK = 0; _nop_();
    }
    ADC_CS = 1;
    return dat;
}

// ================= 主函数 =================
void main() {
    unsigned int sum = 0;
    unsigned char adc_val, i;
    float db;
    char display_buf[16];

    LCD_Init();
    LCD_ShowString(1, 1, "  Noise Meter  "); 
    LED = 1; BEEP = 1;

    while(1) {
        // 1. 连续读5次ADC求平均（滤波）
        sum = 0;
        for(i=0; i<5; i++) {
            sum += ADC0832_Read();
            Delay_ms(5);
        }
        adc_val = sum / 5;

        // 2. 将 0~255 映射为 30~100 的模拟分贝值
        db = 30.0 + (adc_val / 255.0) * 70.0;

        // 3. 屏幕显示
        sprintf(display_buf, "Val: %2.1f dB   ", db);
        LCD_ShowString(2, 1, display_buf);

        // 4. 报警逻辑 (超过 75.0 报警)
        if(db > 75.0) {
            LED = 0;
            Beep_Sound(50);
            Delay_ms(50);
        } else {
            LED = 1;
            BEEP = 1;
        }
        Delay_ms(100); 
    }
}
