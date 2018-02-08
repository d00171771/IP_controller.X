/***************************************************
Name:- Tommy Gartlan
Date last modified:- Jan 2016
Updated: Update to use xc8 compiler within Mplabx

Filename:- st_mch3_2_debounce2b.c
Program Description:- Using a simple state machine
to loop through a menu system.
 
Rule of thumb: Always read inputs from PORTx and write outputs to LATx. 
  If you need to read what you set an output to, read LATx.
 * 
 * This version combines st_mch3 and debounce2b so that buttons are debounced
 * properly for the menu system
 * 
 * this follow on version uses two buttons'UP and 'DOWN' and and an 'Enter, button to update values 
 * Good example of a menu system where values can be updated.
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
const unsigned char msg_ary[4][16] = {"DESIRED", "ACTUAL", 
                                        "POT"," ENTER"};

const unsigned char * problem = "Problem";

const unsigned char * startup = "Ready to go";
typedef struct {
    unsigned char DESIRED1 ;
    unsigned char ACTUAL1 ;
    
}motor_t;
motor_t motor1 = {10,20};

										  

/************************************************
			Function Prototypes
*************************************************/
void Initial(void);
void Window(unsigned char num);
void delay_s(unsigned char secs);
void HeartBeat (void);


/************************************************
 Interrupt Function 
*************************************************/
unsigned char count_test =0;
void __interrupt myIsr(void)
{
    //Timer overflows every 10mS
    // only process timer-triggered interrupts
    if(INTCONbits.TMR0IE && INTCONbits.TMR0IF) {
        
        Find_Button_Press();       //check the buttons every 10mS
        WriteTimer0(45536); 
        INTCONbits.TMR0IF = 0;  // clear this interrupt condition
        
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

    
    //unsigned char Temp_Value = 0;
    
    typedef  enum {RUN = 0,UPDATE_DESIRE} states;
    states  my_mch_state = UPDATE_DESIRE;
    
    Initial();
    motor1.ACTUAL1 =0;
    motor1.DESIRED1 =30;
    HeartBeat();
    lcd_start ();
    lcd_cursor ( 0, 0 ) ;
    lcd_print ( startup ) ;
    
    delay_s(2);
    //Initial LCD Display
    Window(0);
    lcd_cursor ( 8, 0 ) ;
    lcd_display_value("DESIRED");
    lcd_cursor ( 8, 1 ) ;
    lcd_display_value("ACTUAL");
    
    while(1)
    {
		
		//wait for a button Event
		//while(!MENU_E && !ENTER_E && !UP_E && !DOWN_E);  
		while(!Got_Button_E);
        
		switch(my_mch_state)	
		{
			case RUN: 
				if (MENU_E){
                    my_mch_state = UPDATE_DESIRE; //state transition
                    Window(1);             //OnEntry action
                }
                
				break;
			case UPDATE_DESIRE: 
				if (MENU_E){
                    my_mch_state = RUN;  //state transition
                    Window(0);              //OnEntry action
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
				lcd_cursor ( 7, 0 ) ;    //state actions
                lcd_display_value("DESIRED");
                lcd_cursor ( 7, 1 ) ;
                lcd_display_value("ACTUAL");
                LATC = 0x01;
				
				break;
			case UPDATE_DESIRE: 
                //if (ENTER_E)          //state actions with guard
                  //  A = Temp_Value;
              //  if (UP_E)
                  //  Temp_Value++;
                //if (DOWN_E)
                  //  Temp_Value = (Temp_Value != 0 ? Temp_Value - 1 : 0);
                
				lcd_cursor ( 4, 0 ) ;
                lcd_display_value("POT");
				lcd_cursor ( 0, 1 ) ;
                lcd_display_value("ENTER");
                LATC= 0x02;
				break;
			
			default: 
				lcd_cursor ( 4, 0 ) ;
                lcd_clear();
				lcd_print ( problem );
                LATC = 0x05;
				break;
		}
		
        Button_Press.Full = 0;  //Clear all events since only allowing one button event at a time
                                //which will be dealt with immediately
    
    }
}   


void Initial(void){
    
    ADCON1 = 0x0F;
	TRISB = 0xFF; //Buttons
	TRISC = 0x00;   //LEDS

	LATC = 0xff;
	delay_s(3);
	LATC = 0x00;
    
    //0n, 16bit, internal clock(Fosc/4), prescale by 2)
    OpenTimer0( TIMER_INT_ON & T0_16BIT & T0_SOURCE_INT & T0_PS_1_2);
    WriteTimer0(45536);  //65,536 - 24,576  //overflows every 10mS
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
    OSCCONbits.IRCF1 = 0;
    OSCCONbits.IRCF0 = 1;
}

void HeartBeat (void)
{ 
	LATCbits.LC7 = 1;
    delay_s(1);
    LATCbits.LC7 = 0;
    delay_s(1);
}