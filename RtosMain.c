
//Project: RTOS Salvo Project 
//Submitted by: LilWons
//Date of Submission: 2023-12-5

//RTOS Project Template
#include <xc.h>
#include <salvo.h>
#include <stdint.h>
#include <stdbool.h>

#define _XTAL_FREQ  8000000UL

// CONFIG2 WORD
#pragma config POSCMOD = XT         // Primary Oscillator Select->XT Oscillator mode selected
#pragma config OSCIOFNC = OFF       // Primary Oscillator Output Function->OSC2/CLKO/RC15 functions as CLKO (FOSC/2)
#pragma config FCKSM = CSDCMD       // Clock Switching and Monitor->Clock switching and Fail-Safe Clock Monitor are disabled
#pragma config FNOSC = PRI          // Oscillator Select->Primary Oscillator (XT, HS, EC)
#pragma config IESO = ON            // Internal External Switch Over Mode->IESO mode (Two-Speed Start-up) enabled

// CONFIG1 WORD
#pragma config WDTPS = PS32768      // Watchdog Timer Postscaler->1:32768
#pragma config FWPSA = PR128        // WDT Prescaler->Prescaler ratio of 1:128
#pragma config WINDIS = ON          // Watchdog Timer Window->Standard Watchdog Timer enabled,(Windowed-mode is disabled)
#pragma config FWDTEN = OFF         // Watchdog Timer Enable->Watchdog Timer is disabled
#pragma config ICS = PGx2           // Comm Channel Select->Emulator/debugger uses EMUC2/EMUD2
#pragma config BKBUG = OFF          // Background Debug->Device resets into Operational mode
#pragma config GWRP = OFF           // General Code Segment Write Protect->Writes to program memory are allowed
#pragma config GCP = OFF            // General Code Segment Code Protect->Code protection is disabled
#pragma config JTAGEN = OFF         // JTAG Port Enable->JTAG port is disabled

////////////////////////////////////////////////////////////////////////////////
//Function Proto type
void SYSTEM_Initialize(void);
void OSCILLATOR_Initialize(void);
void PIN_MANAGER_Initialize(void);
void TMR1_Initialize (void);
void UART2_Initialize (void);
void ADC_Initialize (void);
void buttonInit (void);
////////////////////////////////////////////////////////////////////////////////
//Global variables
uint16_t mask = 0x01; //mask variable is 16 bit unsigned integer 
uint16_t maskReset = 0x80; //maskReset variable is 16 bit unsigned integer with hex value 0x80
unsigned int counter = 0; //counter is 16-bit unsigned integer set to 0
volatile int direction = 0; //direction is integer and set to 0
int rcvByte; //rcvByte variable is set as integer type
int ADCValue; //ADCValue as integer
int tempC; //tempC as unsigned int
float resolution = 0.0032258; //resolution variable is set to float type and given value 0.0032258 (resolution of the 10-bit ADC)
float voltage; // voltage as float
////////////////////////////////////////////////////////////////////////////////
//Defining task's control block pointers (TCBP)
#define TASK_LED_TOGGLE_P             OSTCBP(1)     //Task #1
#define TASK_SEND_TEMP_P              OSTCBP(2)     //Task #2
#define TASK_SEND_VOLTAGE_P           OSTCBP(3)     //Task #3
#define TASK_READ_AN_P                OSTCBP(4)     //Task #4
#define TASK_KNIGHT_RIDER_P           OSTCBP(5)     //Task #5
#define TASK_DIRECTION_SWAP_P         OSTCBP(6)     //Task #5
////////////////////////////////////////////////////////////////////////////////
//Defining task priority
#define PRIO_LED_TOGGLE             4           //Task1 -> Priority 4 
#define PRIO_SEND_TEMP              10          //Task2 -> Priority 10
#define PRIO_SEND_VOLTAGE           10          //Task3 -> Priority 10
#define PRIO_READ_AN                10          //Task4 -> Priority 10
#define PRIO_KNIGHT_RIDER           10          //Task5 -> Priority 10
#define PRIO_DIRECTION_SWAP         10          //Task6 -> Priority 10
////////////////////////////////////////////////////////////////////////////////
//Defining event control block pointers (ECBP)
#define BINSEM_READ_TEMP_ADC        OSECBP(1)   //Semaphore
#define BINSEM_READ_VOLTAGE_ADC     OSECBP(2)   //Semaphore
#define BINSEM_DIR_SWAP             OSECBP(3)   //Semaphore
////////////////////////////////////////////////////////////////////////////////
//Task #4 ReadAN:, reads from channels AN4 and AN5 than signals corresponding semaphore when sample is completed.  
void ReadAN(void) {
    for (;;){
        AD1CHS = 0x84; //Channel AN4 is set for ADC sampling.
        AD1CON1bits.SAMP = 1; //Start sampling.
        while(!AD1CON1bits.DONE){}; //Wait until conversion is done.
        OSSignalBinSem(BINSEM_READ_TEMP_ADC); //signal semaphore BINSEM_READ_TEMP_ADC
        OS_Delay(100); //Force context switch and delay task by 1 second
        
        AD1CHS = 0x85; //Channel AN5 is set for ADC sampling
        AD1CON1bits.SAMP = 1; //start sampling 
        while(!AD1CON1bits.DONE){}; //wait until conversion is done
        OSSignalBinSem(BINSEM_READ_VOLTAGE_ADC); //signal semaphore BINSEM_READ_VOLTAGE_ADC
      
        OS_Delay(100); //Force context switch and delay task by 1 second  
    }
}
////////////////////////////////////////////////////////////////////////////////
//Task #2 SendTempData: When 'BINSEM_READ_TEMP_ADC' is signaled, reads ADC value, converts to Celsius, than passes Celsius value to U2_TxString().
void SendTempData(void){
    for (;;){     
            OS_WaitBinSem(BINSEM_READ_TEMP_ADC, OSNO_TIMEOUT); //Waiting on 'BINSEM_READ_TEMP_ADC' semaphore, task will continue to wait until 'BINSEM_READ_TEMP_ADC' is signaled.
            ADCValue = ADC1BUF0; //Get value from ADC1BUF0 register.
            tempC = ((ADCValue * resolution)-0.5)/0.01; //Convert digital value to represent temperature in Celsius.
            char value[20]; //Value as a char array with size 20.
            sprintf(value, "%d Celsius", tempC); //Store value with "Celsius."
            U2_TxString(value); //Call U2_TxString method and pass 'value.' 
    }
}
////////////////////////////////////////////////////////////////////////////////
//Task #3 SendResData: When 'BINSEM_READ_VOLTAGE_ADC' is signaled, reads ADC value, converts to millivolts, than passes millivolts value to U2_TxString().
void SendResData(void){
    for (;;){
        OS_WaitBinSem(BINSEM_READ_VOLTAGE_ADC, OSNO_TIMEOUT); //Waiting on 'BINSEM_READ_VOLTAGE_ADC' semaphore, task will continue to wait until 'BINSEM_READ_VOLTAGE_ADC' is signaled.
        ADCValue = ADC1BUF0; //Gets value from ADC1BUF0 register 
        voltage = (ADCValue * resolution); //Convert digital value to represent voltage in milivolts.
        char value[20]; //Value as a char array.
        sprintf(value, "%.2f Voltage (R6)", voltage); //Store formatted value with "Voltage (R6)." 
        U2_TxString(value); //Call U2_TxString method and pass value.
    }
}
////////////////////////////////////////////////////////////////////////////////
//Method U2_TxString: Iterates through char array and passes each iteration to U2_TxByte method.
void U2_TxString(char *s)
{
    char nextChar; //Sets 'nextChar' as character type.
    while(*s != '\0') //Verifies the current iteration of character array is not null.
    {
        nextChar = *s++; //Sets 'nextChar' to current iteration of character array than increases character array's index by one.
        Nop(); //Small delay.
        Nop(); //Small delay.
        U2_TxByte(nextChar); //Calls U2_TxByte method and passes 'nextChar' value.
    }
        U2_TxByte(0x0d); //CR
        U2_TxByte(0x0a); //LF
}
////////////////////////////////////////////////////////////////////////////////
//Method U2_TxByte: Writes the character to the transmit buffer once the transmit buffer is empty.
void U2_TxByte(char value)
{
    while (U2STAbits.UTXBF); // Wait for the transmit buffer to be empty.
    U2TXREG = value; // Write the character to the transmit buffer.
}
////////////////////////////////////////////////////////////////////////////////
//Task KnightRider: Runs the 7 LED (D9-D3) KnightRider pattern.
void KnightRider(void){
    for(;;){
        if(!direction) //If 'direction' is 0...
        { 
            if(mask < 0x80) //If 'mask' is less than 0x80...
            {
            PORTA |= mask; //Perform bitwise OR operator on 'PORTA' with 'mask' 
            mask = mask << 1; //Bit shift 'mask' left once.
            }
            else //If 'mask' is anything else...
            {
                OSSignalBinSem(BINSEM_DIR_SWAP); //Signal 'BINSEM_DIR_SWAP' semaphore.
            }
        }
        else //If 'direction' is anything else...
        {
            if(mask > 0x01) //If 'mask' is greater than 0x01...
            {
                mask = mask >> 1; //Bit shift 'mask' right once.
                PORTA |= mask; //Perform bitwise OR operator on 'PORTA' with 'mask'.   
            }
            else //If 'mask' is anything else...
            {
                OSSignalBinSem(BINSEM_DIR_SWAP); //Signal 'BINSEM_DIR_SWAP' semaphore.
            } 
        }
        OS_Delay(40); //Force context switch and delay task by 400 milliseconds.
        }
    }
////////////////////////////////////////////////////////////////////////////////
//Task DirectionSwap: When 'BINSEM_DIR_SWAP' is signaled, turns LEDs (D9-D3) off, changes KnightRider Direction, and assigns corresponding directional mask.
void DirectionSwap(void)
{
    for(;;){
        OS_WaitBinSem(BINSEM_DIR_SWAP, OSNO_TIMEOUT); //Waiting on 'BINSEM_DIR_SWAP' semaphore, task will continue to wait until 'BINSEM_DIR_SWAP' is signaled. 
        direction = !direction; //Inverts the value of 'direction'.
        PORTA = PORTA & maskReset; //Preform bitwise operator AND on PORTA with 'maskReset.'
        if(direction) //If 'direction' is 1...
        {  
          mask = 0x80;  //'mask' is set to 0x80.
        }
        else //If 'direction' is anything else...
        {
          mask = 0x01; //'mask' is set to 0x80.
        } 
    }
}
////////////////////////////////////////////////////////////////////////////////
//Task LED_Toggle: Toggles LED (D10) on and off every 3.5 seconds.
void LED_Toggle (void)
{
 for (;;) 
 {
     PORTAbits.RA7 = !PORTAbits.RA7; //Inverts the value of 'PORTAbits.RA7'.
     OS_Delay(175); //Force context switch and delay task by 175 milliseconds.
     OS_Delay(175); //Force context switch and delay task by 175 milliseconds.
 }
}
////////////////////////////////////////////////////////////////////////////////
//Main Method: Initialize Servo, Tasks, Events, calls SYSTEM_Initialize method, than starts Servos Scheduler.  
int main (void)
{
    OSInit(); //Initializes all of Servo's data structures, pointers, and counters.
    OSCreateTask(DirectionSwap, TASK_DIRECTION_SWAP_P, PRIO_DIRECTION_SWAP); //Creates task 'DirectionSwap' with TCBP 'TASK_DIRECTION_SWAP_P' and task priority 'PRIO_DIRECTION_SWAP'.
    OSCreateTask(LED_Toggle, TASK_LED_TOGGLE_P, PRIO_LED_TOGGLE); //Creates task 'LED_Toggle' with TCBP 'TASK_LED_TOGGLE_P' and task priority 'PRIO_LED_TOGGLE'.
    OSCreateTask(ReadAN, TASK_READ_AN_P, PRIO_READ_AN); //Creates task 'ReadAN' with TCBP 'TASK_READ_AN_P' and task priority 'PRIO_READ_AN'.
    OSCreateTask(KnightRider, TASK_KNIGHT_RIDER_P, PRIO_KNIGHT_RIDER); //Creates task 'KnightRider' with TCBP 'TASK_KNIGHT_RIDER_P' and task priority 'PRIO_KNIGHT_RIDER'.
    OSCreateTask(SendTempData, TASK_SEND_TEMP_P, PRIO_SEND_TEMP); //Creates task 'SendTempData' with TCBP 'TASK_SEND_TEMP_P' and task priority 'PRIO_SEND_TEMP'.
    OSCreateTask(SendResData, TASK_SEND_VOLTAGE_P, PRIO_SEND_VOLTAGE); //Creates task 'SendResData' with TCBP 'TASK_SEND_VOLTAGE_P' and task priority 'PRIO_SEND_VOLTAGE'.
  
    OSCreateBinSem(BINSEM_READ_TEMP_ADC, 0); //Creates binary semaphore with ECBP 'BINSEM_READ_TEMP_ADC' and sets current value to 0.
    OSCreateBinSem(BINSEM_READ_VOLTAGE_ADC, 0); //Creates binary semaphore with ECBP 'BINSEM_READ_VOLTAGE_ADC' and sets current value to 0.
    OSCreateBinSem(BINSEM_DIR_SWAP, 0); //Creates binary semaphore with ECBP 'BINSEM_DIR_SWAP' and sets current value to 0.
 
    SYSTEM_Initialize(); //Calls SYSTEM_Initialize method.
    
// start multitasking/scheduler/despatcher.
for (;;)
    OSSched(); //pass control to the scheduler(kernel). OSSched() is Salvo's multitasking scheduler. 
}

////////////////////////////////////////////////////////////////////////////////
//Timer 1 ISR
//Interrupt comes at 1 ms Interval.
void __attribute__ ( ( interrupt, no_auto_psv ) ) _T1Interrupt (  )
{
    static uint8_t i = 10;       //Counter = 1 ms x 10 = 10ms.
    
    if(IEC0bits.T1IE && IFS0bits.T1IF )
    {
        IFS0bits.T1IF = false;
        if ( !(--i) ) //Decreases the value of 'i' by one. If 'i' equals zero...
        {
            i = 10; //Sets 'i' back to 10.
            OSTimer();          //call salvo service at 10 ms interval, Tick Rate = 10 mS
        } 
    }
}
//Button S3 ISR.
//Interrupt comes when S3 is pressed (active low).
void __attribute__((__interrupt__, __auto_psv)) _CNInterrupt(void) {
    if(!PORTDbits.RD6) //If S3 is pressed (active low)...
    OSSignalBinSem(BINSEM_DIR_SWAP);

    IFS1bits.CNIF = 0; //Set change notification interrupt flag to 0.
}
////////////////////////////////////////////////////////////////////////////////
//UART2 Receiver ISR.
//Reads data received and resets UART2 Receiver Interrupt Flag Status bit.
void __attribute__ ( ( interrupt, no_auto_psv ) ) _U2RXInterrupt( void )
{
    if(IEC1bits.U2RXIE == 1) //If UART2 Receiver Interrupt is enabled...
    {
        if(IFS1bits.U2RXIF == true) //Check the status flag.
        {
            rcvByte = U2RXREG; //Read the received byte.
        }
    }
    IFS1bits.U2RXIF = false; //Reset the status flag.
}

////////////////////////////////////////////////////////////////////////////////
//Calls methods to initialize system.
void SYSTEM_Initialize(void)
{
    PIN_MANAGER_Initialize(); //Calls PIN_MANAGER_Initialize() method.
    OSCILLATOR_Initialize(); //Calls OSCILLATOR_Initialize() method.
    TMR1_Initialize(); //Calls TMR1_Initialize() method.
    UART2_Initialize(); //Calls UART2_Initialize() method.
    ADC_Initialize(); //Calls ADC_Initialize() method.
    buttonInit(); //Calls buttonInit() method.
}
////////////////////////////////////////////////////////////////////////////////\
//Initialize Oscillator.
void OSCILLATOR_Initialize(void)
{
    // NOSC PRI; SOSCEN disabled; OSWEN Switch is Complete; 
    __builtin_write_OSCCONL((uint8_t) (0x0200 & 0x00FF));
    // RCDIV FRC/2; DOZE 1:8; DOZEN disabled; ROI disabled; 
    CLKDIV = 0x3100;
    // TUN Center frequency; 
    OSCTUN = 0x0000;    
    // WDTO disabled; TRAPR disabled; SWDTEN disabled; EXTR disabled; POR disabled; SLEEP disabled; BOR disabled; IDLE disabled; IOPUWR disabled; VREGS disabled; CM disabled; SWR disabled; 
    RCON = 0x0000;
}
////////////////////////////////////////////////////////////////////////////////
//Initialize Pins.
void PIN_MANAGER_Initialize(void)
{
    /****************************************************************************
     * Setting the Output Latch SFR(s)
     * Through this register, MPU writes
     ***************************************************************************/
    LATA = 0x0000;
    LATB = 0x0000;
    LATC = 0x0000;
    LATD = 0x0000;
    LATE = 0x0000;
    LATF = 0x0000;
    LATG = 0x0000;

    /****************************************************************************
     * Setting the GPIO Direction SFR(s)
     * Through this register, MPU writes and reads
     ***************************************************************************/
    TRISA = 0xC600; //Configure RA0 - RA7 as output.  
    TRISB = 0xFFFF;
    TRISC = 0xF01E;
    TRISD = 0xFFFF; //Configure RD6 as input.
    TRISE = 0x03FF;
    TRISF = 0x31FF;
    TRISG = 0xF3CF;

    /****************************************************************************
     * Setting the Weak Pull Up and Weak Pull Down SFR(s)
     ***************************************************************************/
    CNPU1 = 0x0000;
    CNPU2 = 0x0000;

    /****************************************************************************
     * Setting the Open Drain SFR(s)
     ***************************************************************************/
    ODCA = 0x0000;
    ODCB = 0x0000;
    ODCC = 0x0000;
    ODCD = 0x0000;
    ODCE = 0x0000;
    ODCF = 0x0000;
    ODCG = 0x0000;

    /****************************************************************************
     * Setting the Analog/Digital Configuration SFR(s)
     ***************************************************************************/
}
////////////////////////////////////////////////////////////////////////////////
//Configuring button (S3) CN interrupt.
void buttonInit(void) {
    CNEN1bits.CN15IE = 1; // Enable change notification.
    CNPU1bits.CN15PUE = 1; // Enable internal pull-up resistor.
    IFS1bits.CNIF = 0; // Clear the change notification interrupt flag.
    IEC1bits.CNIE = 1; // Enable the change notification interrupt.
}
////////////////////////////////////////////////////////////////////////////////
// Initialize 10-bit ADC.
void ADC_Initialize ( void )
{
//Setting the Analog/Digital Configuration SFR(s).
    AD1PCFG = 0xFFCF; //Set AN4 (PIN 21) and AN5 (PIN) 20 to Analog Inputs.
    AD1CON1 = 0xE0; // Set SSRC<2:0> to 111 (Internal counter ends sampling and starts conversion (auto-convert)).
    AD1CON2 = 0x0000; //Sets AD1CON2 to 0x0000.
    AD1CON3 = 0x1F3F; //Set ADCS<7:0> to TAD(16uS), Set SAMC<4:0> to 31*TAD (496uS).
    AD1CSSL = 0x30; //Select AN4 (PIN 21) and AN5 (PIN 20) for input scan.  
    AD1CON1bits.ADON = 1; //Enable A/D.
}
////////////////////////////////////////////////////////////////////////////////
// Initialize Timer1.
void TMR1_Initialize (void)
{
    TMR1 = 0x0000; //Timer1 = 0.
    PR1 = 0x0FA0; //Interrupt at = 1 ms; Frequency = 4000000/8 = 5000000 Hz; T = 2 us; PR1= 1000.
    T1CON = 0x0000; //TCKPS 1:64; TON disabled; TSIDL disabled; TCS FOSC/2; TSYNC disabled; TGATE disabled.
    IFS0bits.T1IF = false; //Sets Timer1's Interrupt Flag Status to false.
    IEC0bits.T1IE = true; //Enables Interrupt's for Timer1.
    IPC0bits.T1IP2 = 1; //Priority bits (IP) : (1 1 1 = 7)
    IPC0bits.T1IP1 = 1; //Priority bits (IP) : (1 1 1 = 7)
    IPC0bits.T1IP0 = 1; //Priority bits (IP) : (1 1 1 = 7)
    T1CONbits.TON = 1; //Start Timer1.
}
////////////////////////////////////////////////////////////////////////////////
//Initialize UART2
void UART2_Initialize (void)
{
    //Operation related
    U2MODE = 0x8802; //Enables UART2; Simplex mode; 16x Baud Clock, Standard mode; 8-bit even parity data; One Stop Bit.
    U2STA = 0x0000; //UART2 Status Register is set to 0x0000.
    U2TXREG = 0x0000; //UART2 Transmitter Register is set to 0x0000.
    U2RXREG = 0x0000; //UART2 Receiver Register is set to 0x0000.
    U2BRG = 0x0019; // BaudRate = 9600; Frequency = 4000000 Hz.
    ////////////////////////////////////////////////////////////////////////////////
    //Interrupt related
    IEC1bits.U2RXIE = 1; //Rx interrupt Enabled.
    U2STAbits.UTXEN = 1; //Tx Enabled.
    IEC1bits.U2TXIE = 0; //Tx interrupt disabled.
    IFS1bits.U2RXIF = 0; //Sets UART2 Receiver Interrupt Flag Status bit to 0.
    IFS1bits.U2TXIF = 0; //Sets UART2 Transmitter Interrupt Flag Status bit to 0.
    IPC7bits.U2RXIP2 = 1; // Priority bits (IP) : (1 0 0 = 4).
    IPC7bits.U2RXIP1 = 0; // Priority bits (IP) : (1 0 0 = 4).
    IPC7bits.U2RXIP0 = 0; // Priority bits (IP) : (1 0 0 = 4).
}
////////////////////////////////////////////////////////////////////////////////