#include <reg51.h>
#include <intrins.h>

sbit LCD_RS = P2^6;
sbit LCD_E  = P2^7;
#define LCD_DataPort P0

void Delay_ms(unsigned int ms) {
    unsigned int i, j;
    for(i=0; i<ms; i++) for(j=0; j<110; j++);
}

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

void main() {
    LCD_Init();
    LCD_ShowString(1, 1, " Noise Meter  ");
    LCD_ShowString(2, 1, "   TEST OK!   ");
    
    while(1) {
        // 程序停在这里，啥也不干，绝对不叫！
    }
}
