/*
    (c) 2020 Microchip Technology Inc. and its subsidiaries. 
    
    Subject to your compliance with these terms, you may use Microchip software and any 
    derivatives exclusively with Microchip products. It is your responsibility to comply with third party 
    license terms applicable to your use of third party software (including open source software) that 
    may accompany Microchip software.
    
    THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER 
    EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY 
    IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS 
    FOR A PARTICULAR PURPOSE.
    
    IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, 
    INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND 
    WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP 
    HAS BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO 
    THE FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL 
    CLAIMS IN ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT 
    OF FEES, IF ANY, THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS 
    SOFTWARE.
*/

/* WDT operating mode->WDT Disabled */
#pragma config WDTE = OFF
/* Low voltage programming enabled, RE3 pin is MCLR */
#pragma config LVP = ON

#include <xc.h>
#include <stdint.h>


#define MIN_DCY                        0
#define MAX_DCY                        1023

/* this must be set to 0 for a common-kathode LED */
#define LED_COMMON_ANODE               1

#if LED_COMMON_ANODE
    #define LED_MIN_BRIGHT             (MAX_DCY)
    #define LED_MAX_BRIGHT             (MIN_DCY)
    #define LED_INC_BRIGHT(x)          (MAX_DCY - (x))
    #define LED_DEC_BRIGHT(x)          (MIN_DCY + (x))
#else /* LED_COMMON_ANODE */
    #define LED_MAX_BRIGHT             (MAX_DCY)
    #define LED_MIN_BRIGHT             (MIN_DCY)
    #define LED_DEC_BRIGHT(x)          (MAX_DCY - (x))
    #define LED_INC_BRIGHT(x)          (MIN_DCY + (x))
#endif /* LED_COMMON_ANODE */


static void RGB_LED_Handler(void);
static void PWM1_Initialize(void);
static void PWM2_Initialize(void);
static void PWM3_Initialize(void);
static void PORT_Initialize(void);
static void PPS_Initialize(void);
static void CLK_Initialize(void);
static void TMR2_Initialize(void);
static void INTERRUPT_Initialize(void);
static void PWM1_LoadDutyValue(uint16_t dutyValue);
static void PWM2_LoadDutyValue(uint16_t dutyValue);
static void PWM3_LoadDutyValue(uint16_t dutyValue);
static void TMR2_ISR(void);

static void RGB_LED_Handler(void)
{
    /* this implements a cyclic color game on the RGB LED:
     * 
     * step 0: red   decreases from max down to 0, 
     *   while green increases from 0 upto max
     *   while blue  is turned off
     * 
     * step 1: red is turned off
     *   while green decreases from max down to 0
     *   while blue  increases from 0 upto max
     * 
     * step 2: red   increases from 0 upto max
     *   while green is turned off
     *   while blue  decreases down to 0
     * 
     * goto step 0
     */

    static uint16_t dcyR    = LED_MIN_BRIGHT;
    static uint16_t dcyG    = LED_MIN_BRIGHT;
    static uint16_t dcyB    = LED_MIN_BRIGHT;
    static uint16_t counter = MIN_DCY;
    static uint8_t  sector  = 0;

    PWM1_LoadDutyValue(dcyR);
    PWM2_LoadDutyValue(dcyG);
    PWM3_LoadDutyValue(dcyB);

    switch(sector)
    {
        case 0: dcyR = LED_DEC_BRIGHT(counter);
                dcyG = LED_INC_BRIGHT(counter);
                dcyB = LED_MIN_BRIGHT;
                break;
        case 1: dcyR = LED_MIN_BRIGHT;
                dcyG = LED_DEC_BRIGHT(counter);
                dcyB = LED_INC_BRIGHT(counter);
                break;
        case 2: dcyR = LED_INC_BRIGHT(counter);
                dcyG = LED_MIN_BRIGHT;
                dcyB = LED_DEC_BRIGHT(counter);
                break;
        default:dcyR = LED_MIN_BRIGHT;
                dcyG = LED_MIN_BRIGHT;
                dcyB = LED_MIN_BRIGHT;
                break;
    }
    
    counter++;
    if(counter > MAX_DCY)
    {
        counter = MIN_DCY;
        sector++;
        if(sector >= 3)
            sector = 0;
    }
}

static void PWM1_Initialize(void)
{
	/* MODE PWM; EN enabled; FMT left_aligned */
    CCP1CONbits.MODE = 0x0C;
    CCP1CONbits.FMT = 1;
    CCP1CONbits.EN = 1;

	/* Selecting Timer 2 */
    CCPTMRSbits.C1TSEL = 1;
    
    /* Configure initial duty cycle */
    CCPR1 = (uint16_t)LED_MIN_BRIGHT << 6;
}

static void PWM2_Initialize(void)
{
	/* MODE PWM; EN enabled; FMT left_aligned */
    CCP2CONbits.MODE = 0x0C;
    CCP2CONbits.FMT = 1;
    CCP2CONbits.EN = 1;

	/* Selecting Timer 2 */
    CCPTMRSbits.C2TSEL = 1;

    /* Configure initial duty cycle */
    CCPR2 = (uint16_t)LED_MIN_BRIGHT << 6;
}

static void PWM3_Initialize(void)
{
    /* PWM3 enabled module, polarity normal */
    PWM3CONbits.EN = 1;
    PWM3CONbits.POL = 0;

    /* Select timer 2 */
    CCPTMRSbits.P3TSEL = 1;

    /* Configure initial duty cycle */
    PWM3DC = (uint16_t)LED_MIN_BRIGHT << 6;
}

static void PORT_Initialize(void)
{
    /* RB0 is output for PWM1 */
    TRISBbits.TRISB0 = 0;
    /* RB3 is output for PWM2 */
    TRISBbits.TRISB3 = 0;
    /* RD0 is output for PWM3 */
    TRISDbits.TRISD0 = 0;
}   

static void PPS_Initialize(void)
{
    /* Configure RB0 for PWM1 output */
    RB0PPS = 0x05;
    /* Configure RB3 for PWM2 output */
    RB3PPS = 0x06;
    /* Configure RD0 for PWM3 output */
    RD0PPS = 0x07;
}

static void CLK_Initialize(void)
{
    /* Configure NOSC HFINTOSC; NDIV 1; FOSC = 64MHz */
    OSCCON1bits.NOSC = 6;
    OSCCON1bits.NDIV = 0;

    /* HFFRQ 64_MHz */
    OSCFRQbits.HFFRQ = 8;
}

static void TMR2_Initialize(void)
{
    /* TIMER2 clock source is FOSC/4 */
    T2CLKCONbits.CS = 1;

    /* TIMER2 counter reset */
    T2TMR = 0x00;

    /* TIMER2 prescaler 1:8, postscaler 1:16 */
    T2CONbits.CKPS = 3;
    T2CONbits.OUTPS = 15;

    /* TIMER2 period register setting, divided by 4 because FOSC/4 is used for PWM mode */
    T2PR = MAX_DCY >> 2;

    /* Clearing IF flag before enabling the interrupt */
    PIR4bits.TMR2IF = 0;

    /* Enabling TIMER2 interrupt */
    PIE4bits.TMR2IE = 1;

    /* TIMER2 ON */
    T2CONbits.ON = 1;
}

static void  INTERRUPT_Initialize(void)
{
    /* Enable the Global Interrupts */
    INTCONbits.GIE = 1;

    /* Enable the Peripheral Interrupts */
    INTCONbits.PEIE = 1;
}

static void PWM1_LoadDutyValue(uint16_t dutyValue)
{
    /* duty cycle is 10-bit left justified in 16-bit register */
    CCPR1 = dutyValue << 6;
}

static void PWM2_LoadDutyValue(uint16_t dutyValue)
{
    /* duty cycle is 10-bit left justified in 16-bit register */
    CCPR2 = dutyValue << 6;
}

static void PWM3_LoadDutyValue(uint16_t dutyValue)
{
    /* duty cycle is 10-bit left justified in 16-bit register */
    PWM3DC = dutyValue << 6;
}

static void TMR2_ISR(void)
{
    /* clear the TMR2 interrupt flag */
    PIR4bits.TMR2IF = 0;
    
    /* user ISR function call; */
    RGB_LED_Handler();
}

static void __interrupt() INTERRUPT_InterruptManager (void)
{
    /* interrupt handler */
    if(INTCONbits.PEIE)
    {
        if( (PIE4bits.TMR2IE) && (PIR4bits.TMR2IF) )
        {
            TMR2_ISR();
        } 
    }      
}

void main(void)
{
    /* Initialize the device */
    PORT_Initialize();
    PPS_Initialize();
    CLK_Initialize();
    PWM1_Initialize();
    PWM2_Initialize();
    PWM3_Initialize();
    TMR2_Initialize();
	INTERRUPT_Initialize();

    while (1)
    {
        ;
    }
}

