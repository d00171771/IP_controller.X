/***************************************************
Name:- Abdalrahman Hussain & Amro Alotaibi 
Date last modified:- March 2018

Filename:- Motor_1
Program Description:- Control motor speed with setting a desired value.
 
*****************************************************************************************************/

/****************************************************
		Libraries included
*****************************************************/
#include <xc.h>
#include <stdio.h>
#include <stdlib.h>
#include "../../Repositories/Buttons_Debounce_State_Mch/Buttons_Debounce.X/Buttons_Debounce.h"
#include "../../Repositories/LCD_library/lcdlib_2016.h"

#include <plib/timers.h>
#include <plib/adc.h>
//#include <plib/usart.h>
//#include <plib/can2510.h>

/***********************************************
		Configuration Selection bits
************************************************/

/*************************************************
					Clock
*************************************************/
#define _XTAL_FREQ 8000000

/************************************************
			Global Variables
*************************************************/
const unsigned char msg_ary[4][16] = {"DESIRED_VAL", "ACTUAL_VAL", 
                                        "POT","ENTER_TO_ACCEPT"};
const unsigned char * problem = "Problem";
const unsigned char * startup = "Ready to go";
typedef struct {
    unsigned char DESIRED_1 ;
    unsigned int ACTUAL_1 ;
}motor_t;
motor_t motor1 ;
/************************************************
			Function Prototypes
*************************************************/
void Initial(void);
void Window(unsigned char num);
void delay_s(unsigned char secs);
void setup_capture(void);
void setup_PWM(void);
void setup_clock (void);
void controller_func (void);
/************************************************
 Interrupt Function 
*************************************************/
unsigned int capture;
unsigned char count_test =0;
unsigned char sample_time = 0;
bit TICK_E = 0;//tick_e is only 1 bit
unsigned char i=0;
typedef struct {
        float Kp;
        float Ki;
        float KpKi; // Kp + Ki
        signed char ek; // e[n]
        signed char ek_1; // e[n-1]
        unsigned char Uk;// u[n]
        unsigned char Uk_1;// u[n-1]
    }controller;
    controller control1;
    
void __interrupt myIsr(void)
{
     if(PIE1bits.CCP1IE && PIR1bits.CCP1IF)
    {
        PIR1bits.CCP1IF = 0;  //A TMR1 register capture occurred (flag capture bit must be cleared in software) 
        TMR1H = 0x00;         //TMR1 low and high bytes cleared
        TMR1L = 0x00;
        capture = CCPR1L;     
        capture = capture + (CCPR1H*256);
        //TRISCbits.RC2=1;
    }
    //Timer overflows every 10mS
    // only process timer-triggered interrupts
     //100ms
     if(INTCONbits.TMR0IE && INTCONbits.TMR0IF) {  
        Find_Button_Press();         //check the buttons every 10mS
        WriteTimer0(45536); 
        INTCONbits.TMR0IF = 0;       // clear this interrupt condition
        
           sample_time ++;
        if(sample_time == 10){
            controller_func();
         CCPR2L = control1.Uk;
            TICK_E = 1;              //check the timer overflow rate
            sample_time = 0;         //Toggle every 1 second (heartbeat)
        }
        //Heartbeat signal
        count_test++;
        if(count_test == 100){
            PORTCbits.RC7 = ~PORTCbits.RC7;   //check the timer overflow rate
            count_test = 0;                   //Toggle every 1 second (heartbeat))
        }
    }
}
//declare Button
Bit_Mask Button_Press;	
/************************************************
			Macros
*************************************************/
#define MENU_E Button_Press.B0
#define ENTER_E Button_Press.B1
/*****************************************
 			Main Function
******************************************/
void main ( void ) 
{
    unsigned short long tclk = 250000;
    unsigned int rev_s;
    unsigned char POT_VALUE = 0;
    typedef  enum {RUN = 0,UPDATE_DESIRED_VALUE} states;
    states  my_mch_state = UPDATE_DESIRED_VALUE;
    Initial();
    setup_capture();
    setup_PWM();  
    setup_clock();
    lcd_start ();
    
    
    lcd_cursor ( 0, 0 ) ;
    lcd_print ( startup ) ;
    delay_s(2);
    Window(0);
    lcd_cursor ( 12, 0 ) ;
    lcd_display_value(motor1.DESIRED_1);
    lcd_cursor ( 12, 1 ) ;
    lcd_display_value(motor1.ACTUAL_1);
    while(1)
    {

        while(!Got_Button_E && !TICK_E);
		switch(my_mch_state)	
		{
			case RUN: 
				if (MENU_E){
                    my_mch_state = UPDATE_DESIRED_VALUE; //state transition
                    Window(1);             //OnEntry action
                  ADCON0bits.ADON =1;   
                  
                }
				break;
			case UPDATE_DESIRED_VALUE: 
				if (MENU_E){
                    my_mch_state = RUN;  //state transition
                    Window(0);              //OnEntry action
                    ADCON0bits.ADON =0;   
                }
				break;
			default: 
				if (MENU_E){
                    my_mch_state = RUN;
                    Window(0);
                }
				break;
		}
		switch(my_mch_state)	
		{
			case RUN: 
                motor1.ACTUAL_1 = rev_s; 

	    lcd_cursor ( 12, 0 ) ;    //state actions
               lcd_display_value(motor1.DESIRED_1);
               lcd_cursor ( 12, 1 ) ;
               (capture==0)?(rev_s=0):( rev_s= tclk/capture); 
               lcd_display_value(motor1.ACTUAL_1);
                //LATC = 0x01;
				
				break;
			case UPDATE_DESIRED_VALUE: 
               POT_VALUE = ADRESH>>2;
               if(POT_VALUE>50)
               {
                       POT_VALUE=50;
               }
               if (ENTER_E)          //state actions with guard
               {
                  motor1.DESIRED_1 = POT_VALUE;
               }
                  
                
				lcd_cursor ( 12, 0 ) ;
                lcd_display_value(POT_VALUE);

                SelChanConvADC(ADC_CH0);
				break;
			
			default: 
				lcd_cursor ( 4, 0 ) ;
                lcd_clear();
				lcd_print ( problem );
				break;
		}
		
        Button_Press.Full = 0;  //Clear all events since only allowing one button event at a time
                                //which will be dealt with immediately
        TICK_E = 0;
    }
}   

void Initial(void){
    control1.Kp = 0.4;
    control1.Ki = 0.3;
    control1.KpKi = control1.Kp + control1.Ki;
    control1.ek = 0;
    control1.ek_1 = 0;
    control1.Uk = 30;
    control1.Uk_1 = 30;
    ADCON1 = 0x0F;
    TRISB = 0xFF; //Buttons
    TRISC = 0x04;   //LEDS
    LATC = 0xff;
    delay_s(3);
    LATC = 0x00;
    //0n, 16bit, internal clock(Fosc/4), prescale by 2)
    OpenTimer0( TIMER_INT_ON & T0_16BIT & T0_SOURCE_INT & T0_PS_1_1);
    WriteTimer0(45536);  //65,536 - 24,576  //overflows every 10mS
    
    OpenADC(ADC_FOSC_RC & ADC_LEFT_JUST
             & ADC_4_TAD, ADC_CH0
             & ADC_INT_OFF & ADC_REF_VDD_VSS,
              ADC_1ANA); 
    ei();
    
}

void Window(unsigned char num)
{
    lcd_clear();
    lcd_cursor ( 0, 0 ) ;	
	lcd_print ( msg_ary[num*2]);
    lcd_cursor ( 0, 1 ) ;
    lcd_print ( msg_ary[(num*2)+1]);
}

void delay_s(unsigned char secs)
{
    unsigned char i,j;
    for(j=0;j<secs;j++)
    {
        for (i=0;i<25;i++)
                __delay_ms(40);  //max value is 40 since this depends on the _delay() function which has a max number of cycles     
    }
}
void setup_clock (void) 
{
    OSCCONbits.SCS1 = 1; // switch the oscilator 
    OSCCONbits.IRCF2 = 1;
    OSCCONbits.IRCF1 = 1;
    OSCCONbits.IRCF0 = 1;
}
void setup_capture(void)
{
T1CONbits.T1CKPS = 0b11;
T1CONbits.TMR1ON = 1;
T1CONbits.RD16 = 1;
INTCONbits.GIEL = 1;
INTCONbits.GIE = 1;
CCP1CONbits.CCP1M = 0b0101;
PIE1bits.CCP1IE = 1;
}
void setup_PWM (void)
{
    CCP2CON = 0b00001110;
    PR2 = 50;
    T2CON = 0b00000100; // Timer 2 is turned on but not pre-scaled
   TRISCbits.RC1=0;
}
void controller_func (void)
{
 float temp = 0;
 control1.ek_1 = control1.ek;
 control1.ek = motor1.DESIRED_1 - motor1.ACTUAL_1;
control1.Uk_1 = control1.Uk;
temp = ((control1.KpKi)*control1.ek) - (control1.Kp*control1.ek_1);
control1.Uk = control1.Uk_1+temp;

}
