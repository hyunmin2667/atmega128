// ����ũ�����μ���
// UV��� ���� ���� �ð� �����

#include <mega128.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <delay.h>

#define FUNCSET 0x28 //Function Set 
#define ENTMODE 0x06 // Entry Mode Set 
#define ALLCLR 0x01 // All Clear 
#define DISPON 0x0c // Display On 
#define DISPOFF 0x08 // Display Off 
#define BLINKON 0x0F // Display On & Cursor On & Cursor Blink on
#define BLINKOFF 0x0C // Display On & Cursor Off & Cursor Blink off
#define LINE2 0xC0 // 2nd Line Move 1110 0000
#define HOME 0x02 // Cursor Home 
#define RSHIFT 0x1C // Display Right Shift
        
#define nop2 {#asm("nop"); #asm("nop");}
#define nop8 {nop2; nop2; nop2; nop2;}
#define ws2812b PORTB.7
       
char EDITCURSOR = 0x8E; // Editting Cursor
char mood = 0; // Mood Light On/Off & Color change
int duty1 = 0, duty2 = 0, duty3 = 0; // Mood Light RGB Value
char hour=0, _min=0, sec=0, mSec=0, loc=0;
char buf[20];

void byte_out(char d);
void LCD_init(void); //LCD �ʱ�ȭ
void LCD_String(char *); // ���ڿ� ��� �Լ� 
void Busy(void); 
void Command(unsigned char); //�ν�Ʈ���� ���� �Լ� 
void Data(unsigned char); //������ ���� �Լ� 
void time_display(void);
void edit_cursor(void);

void main(void) 
{             
    char i = 0;
    
    DDRB.7 = 1;  
    DDRC.7 = 1;   
    
    PORTC.7 = 0;
    
    //���ͷ�Ʈ     
    DDRD = 0x00;
    EIMSK = 0b00001111;        
    EICRA = 0b10101010;
    TCCR1B=0x0C; OCR1A=6249; TIMSK=0x10; //16000000/256/(1+6249)=10Hz=0.1sec // A��ġ
    SREG = 0b10000000;  
                  
    LCD_init(); 
    
    while(1)
    {
        for(i=0;i<8;i++){  //8���� LED�� ���ʴ�� ���� ��
            byte_out(duty1); //G 
            byte_out(duty2); //R
            byte_out(duty3); //B
        } 
        delay_ms(5);  
    }
} 

interrupt [TIM1_COMPA] void timer1_compa_isr(void)
{
    mSec++;
    if(mSec> 9)
    {
        mSec=0; 
        if(++sec >59)
        {
            sec=0;
            if(++_min >59)
            {
                _min=0;
                if(++hour>23) hour=0;
            }
        }
    }     
    time_display();
    Command(LINE2); 
    LCD_String(" Microprocessor"); 
}

//�ð� �ð� ����1
interrupt [EXT_INT0] void external_int0(void)
{
    if(loc == 0)
    {    
        Command(LINE2);
        LCD_String("Editing...       ");    
    }      
    TIMSK = 0b00000000; //�ð� �����߿��� �ð踦 ����
    loc++;     
    edit_cursor();
    if(loc == 7)
    {
        loc=0;
        Command(ALLCLR);
        Command(BLINKOFF);
        time_display(); 
        TIMSK=0x10;
    }  
}

//�ð� �ð� ����2
interrupt [EXT_INT1] void external_int1(void)
{
    if(TIMSK == 0b00000000)
    {
        if(loc == 1) sec++;
        if(loc == 2) sec += 10;
        if(loc == 3) _min++;
        if(loc == 4) _min += 10;
        if(loc == 5) hour++;
        if(loc == 6) hour += 10;
        if(sec > 59) sec = 0;
        if(_min > 59) _min = 0;
        if(hour > 24) hour = 0;
        time_display();
        edit_cursor();
    }
}

//RGB LED (Mood Light)
interrupt [EXT_INT2] void external_int2(void)
{
    mood++;            
    if(mood == 1) { // Default Light
        duty1 = 100; //G
        duty2 = 255; //R
        duty3 = 0; //B 
    }
    else if(mood == 2) { // RED
        duty1 = 0; //G
        duty2 = 255; //R
        duty3 = 0; //B
    } 
    else if(mood == 3) { // GREEN
        duty1 = 255; //G
        duty2 = 0; //R
        duty3 = 0; //B 
    }   
    else if(mood == 4) { // BLUE
        duty1 = 0; //G
        duty2 = 0; //R
        duty3 = 255; //B 
    }   
    else if(mood == 5) { // PURPLE
        duty1 = 0; //G
        duty2 = 95; //R
        duty3 = 255; //B 
    } 
    else if(mood == 6) { // PINK
        duty1 = 0; //G
        duty2 = 255; //R
        duty3 = 221; //B 
    }   
    else if(mood == 7) {
        duty1 = 0; //G
        duty2 = 0; //R
        duty3 = 0; //B   
        mood = 0;
    }
}

//UV LED ���� ON OFF
interrupt [EXT_INT3] void external_int3(void)
{
    PORTC.7 = !PORTC.7;
}

//RGB LED
void byte_out(char d)
{
    char i;
    for(i=0;i<8;i++){
        if(d&0x80){ ws2812b=1; nop8; ws2812b=0; }
        else      { ws2812b=1; nop2; ws2812b=0; }
        d<<=1;  // ��Ʈ������
    }
}

// �ð� ���
void time_display(void)
{       
    Command(HOME);           
    sprintf(buf,"[TIME][%2d:%2d:%2d]", hour, _min, sec);               
    LCD_String(buf);                      
}

// Editting Ŀ�� ǥ�� 
void edit_cursor(void)
{
    Command(BLINKON);
    if(loc == 1) Command(EDITCURSOR);
    if(loc == 2) Command(EDITCURSOR-1);
    if(loc == 3) Command(EDITCURSOR-3);
    if(loc == 4) Command(EDITCURSOR-4);
    if(loc == 5) Command(EDITCURSOR-6);
    if(loc == 6) Command(EDITCURSOR-7);
}

// LCD �ʱ�ȭ 
void LCD_init(void) 
{ 
    DDRA = 0xFF; // ��Ʈ A ��� ����  
    PORTA &= 0xFB; // E = 0; 
    delay_ms(15); 
    Command(0x20); 
    delay_ms(5); 
    Command(0x20); 
    delay_us(100); 
    Command(0x20); 
    Command(FUNCSET); 
    Command(DISPON);
    Command(ALLCLR); 
    Command(ENTMODE); 
} 

// ���ڿ� ��� �Լ� 
void LCD_String(char *str) 
{ 
    char *pStr=0; 
    pStr = str; 
    while(*pStr) Data(*pStr++); 
} 

// �ν�Ʈ���� ���� �Լ� 
void Command(unsigned char byte) 
{ 
    Busy();     
    
    // �ν�Ʈ���� ���� ����Ʈ 
    PORTA = (byte & 0xF0); // ������ 
    PORTA &= 0xFE; //RS = 0; 
    PORTA &= 0xFD; //RW = 0; 
    delay_us(1); 
    PORTA |= 0x04; //E = 1; 
    delay_us(1); 
    PORTA &= 0xFB; //E = 0; 
    
    // �ν�Ʈ���� ���� ����Ʈ 
    PORTA = ((byte<<4) & 0xF0); // ������ 
    PORTA &= 0xFE; //RS = 0; 
    PORTA &= 0xFD; //RW = 0; 
    delay_us(1); 
    PORTA |= 0x04; //E = 1; 
    delay_us(1); 
    PORTA &= 0xFB; //E = 0; 
} 

// ������ ���� �Լ� 
void Data(unsigned char byte) 
{ 
    Busy();  
    
    // ������ ���� ����Ʈ 
    PORTA = (byte & 0xF0); // ������ 
    PORTA |= 0x01; //RS = 1; 
    PORTA &= 0xFD; //RW = 0; 
    delay_us(1); 
    PORTA |= 0x04; //E = 1; 
    delay_us(1); 
    PORTA &= 0xFB; //E = 0; 
    // ������ ���� ����Ʈ 
    PORTA = ((byte<<4) & 0xF0); // ������ 
    PORTA |= 0x01; //RS = 1; 
    PORTA &= 0xFD; //RW = 0; 
    delay_us(1); 
    PORTA |= 0x04; //E = 1;
    delay_us(1); 
    PORTA &= 0xFB; //E = 0; 
} 

// Busy Flag Check -> �Ϲ����� BF�� üũ�ϴ� ���� �ƴ϶� 
// ������ �ð� ������ �̿��Ѵ�. 
void Busy(void) 
{ 
    delay_ms(2); 
}