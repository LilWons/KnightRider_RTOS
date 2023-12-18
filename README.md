# **RTOS Knight Rider Program**

Program developed on MPLABxIDE for Explorer 16 Board.
Microchip: PIC24FJ128GA010

**Features**

- Displays a Knight Rider pattern on LED's D3 - D9.

- Active low push button S3 changes Knight Rider's direction. 

- Toggle LED D10 every 3 seconds. 

- Read potentiometer (R6) and Temperature (U4) analog values and displaying value in terminal through UART interface.

## **Calculations (Current/voltage/resistance/Time/Frequency/ etc):**

**ADC Resolution**

Resolution = Vref / (2^n - 1) = 3.3V / (2^10 - 1) = 3.23mV

**Digital Value to Voltage Value** 

V(R6) = ADC Value * ADC Resolution

**Digital Value to Celsius**

Temp Celsius = ((ADC Value * ADC resolution) - 0.5) / 0.01  

**Resistance**

R34 – R42 = 470 ohms (for each resistor). 
R33 = 10k
Voltage:
RA0 – RA7 = 3.3 Volts 
SW3 = 3.3 Volts 

**Current**

I = (VDD - VLED) / R(35-42) = (3.3V - 1.8V) / 470Ω = 3.19mA
Therefore, the current at RA0 – RA7 = 3.19mA when the LED is on.
When the LED is off, the voltage and current are 0.
 
When S3 is open, there is no current at RD6, and there are 3.3 Volts at RD6.
When S3 is closed, the voltage at RD6 drops to 0V and there is no current. 

The clock frequency is 4MHz.
The clock period is 0.25uS.

**ADC Calculations**

For AD1CON3 = 0x1F3F configuration:
ADC Conversion Clock Period (TAD) = TCY (ADCS + 1) = 0.25uS(63 + 1) = 16uS
ADCS = TAD/TCY - 1 = (16.uS)/0.25uS - 1 = 63
AutoSample Time = (SAMC + 1) * TAD = (31 + 1) * 16uS = 512uS
Therefore, the ADC Clock Period is 16uS and the Auto-Sample Time is 512uS per AD1CON3 configuration where TCY = 0.25uS.

**UART2 Calculations**

FCY = 4MHz
Desired Baud Rate = 9600
UBRGx = FCY / (16 * BaudRate) - 1 = 4MHz / (16*9600) = 25
Calculated Baud Rate = FCY / (16 * (UBRGx + 1)) = 4MHz / (16 * (25 + 1)) = 9615
Error Rate = (Calculated Baud Rate - Desired Baud Rate)/(Desired Baud Rate) * 100% = (9615 - 9600) / 9600 * 100% = 0.156%
Therefore, U2BRG = 0x0019.


**Timer1 / Interrupt Calculations**

Interrupt Period = 1ms
Timer1 = 16bit therefore PR1 < 65536
Timer1 Count Frequency:   FCNT = (FOSC / 2) / Prescaler = ((8MHz / 2)) / 1 = 4MHz / 1 = 4MHz
Timer1 Count Period:  TCNT = 1 / FCNT = 1 / 4MHz = 0.25μS
Period Register = (Interrupt Period - TCNT) / TCNT = (1ms - 0.25μS) / 0.25μS = 3999 = 0x0F9F

**Salvo Tick Rate**

OSTimer() is called every 10th Timer1 Interrupt.
Timer1 Interrupt occurs after 1ms.
10×1ms=10ms
Therefore, Salvo’s tick rate is 10ms.
