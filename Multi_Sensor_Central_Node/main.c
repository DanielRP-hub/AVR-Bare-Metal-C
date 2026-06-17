/**
 * @file main.c
 * @brief Multi-Sensor Master Controller and Actuator Node for ATmega2560.
 * @details Integrates multiple hardware protocols (I2C, SPI, USART, ADC) to interface with
 * environmental sensors (LM35, MPU6050, BH1750, MAX6675, HC-SR04) and actuate outputs 
 * (DC Motor via Fast PWM, Servo Motor via Phase Correct PWM, MCP4725 DAC).
 * @author Daniel Ruiz Perez, Diego Mondragon Gutierrez, Karla Gissel Jimenez Aguilar
 * @date 2024-06-22
 */ 

#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>		
#include <stdio.h>			
#include <stdlib.h>			
#include <string.h>			
#include <ctype.h>			
#include <avr/interrupt.h>	

// ---------------- Hardware Definitions ----------------

// DC Motor Output (Timer1)
#define PWM_MOTOR_PORT PORTB
#define PWM_MOTOR_DDR  DDRB
#define PWM_MOTOR_PIN  PB5 // OC1A

// I2C Status Codes
#define START        0x08 
#define RESTART      0x10 
#define MT_SLA_ACK   0x18 
#define MR_SLA_ACK   0x40 
#define MT_DATA_ACK  0x28 
#define MR_DATA_ACK  0x50 
#define MR_DATA_NACK 0x58 

// I2C Device Addresses
#define MPU6050_ADDR 0x68
#define MCP4725_ADDR 0x62 

// HC-SR04 Ultrasonic Sensor
#define US_PORT PORTE
#define US_PIN  PINE
#define US_DDR  DDRE
#define US_TRIG_POS  PE4
#define US_ECHO_POS  PE5
#define US_ERROR        -1
#define US_NO_OBSTACLE  -2

// ---------------- Global Variables ----------------

char print[32], stra[50], sata[32], Temperature[10];
int valid;
char GPS, seleccion;
uint8_t k = 0, j = 0, recibir_gps = 0, pwm_duty_cycle = 0, grados = 0;
float ancho_pulso = 0;
char string[100] = {'\0'}, buffer[100] = {'\0'};
char string_gps[400] = {'\0'};

// MPU6050 Datapoints
int16_t accelX0, accelY0, accelZ0, gyroX0, gyroY0, gyroZ0;
int16_t accelX, accelY, accelZ, gyroX, gyroY, gyroZ;
int16_t AX, AY, AZ, GX, GY, GZ;

// ---------------- USART Initialization & Methods ----------------

/**
 * @brief Initializes USART0 for Command Input (9600 Baud).
 */
void init_usart0(void) {
    UBRR0 = 103;                                
    UCSR0C |= (1 << UCSZ00) | (1 << UCSZ01);    
    UCSR0B |= (1 << RXEN0) | (1 << TXEN0) | (1 << RXCIE0); 
}

/**
 * @brief Initializes USART3 for GPS Parsing (9600 Baud).
 */
void init_USART3(){
    UBRR3 = 103;		
    UCSR3C |= (1 << UCSZ11) | (1 << UCSZ10);	
    UCSR3B |= (1 << RXEN3) | (1 << TXEN3) | (1 << RXCIE3);		
    DDRJ &= ~(1 << 0); // PJ0 (RX3) as input
}

void usart_putc(char x) {
    while (!(UCSR0A & (1 << UDRE0)));
    UDR0 = x;    
}

void usart_puts(const char *s) {
    uint8_t i = 0;
    while (s[i] != '\0') {
        usart_putc(s[i]);
        i++;
    }
}

void usart_puts3(){			
    uint8_t z = 0;
    while(string_gps[z] != '\0'){
        usart_putc(string_gps[z]);
        z++;
    }
    usart_putc('\n');
}

void UART_sendFloat(float value) {
    snprintf(buffer, sizeof(buffer), "%d.%02d", (int)value, (int)((value - (int)value) * 100));
    usart_puts(buffer);
}

// ---------------- SPI Initialization & Methods ----------------

/**
 * @brief Initializes SPI bus as Master.
 */
void init_SPI(void){
    DDRB |= (1<<DDB1) | (1<<DDB2) | (1<<DDB0); // CS (PB0), CLK, MOSI Outputs
    PORTB |= (1<<0); // Deselect CS
    SPCR |= (1<<SPE) | (1<<MSTR); // Mode 0, Master, Enable SPI, f/4
    DDRB &= ~(1<<DDB3); // MISO Input
}

/**
 * @brief Unified SPI transfer function to prevent hardware locking.
 * @param data Byte to transmit.
 * @return Byte received.
 */
uint8_t spi_transfer(uint8_t data) {
    SPDR = data;
    while (!(SPSR & (1<<SPIF)));
    return SPDR;
}

// ---------------- I2C Initialization & Methods ----------------

/**
 * @brief Initializes Two-Wire Interface (I2C) at 400kHz.
 */
void I2C_init(){
    TWSR = 0x00; 
    TWBR = 0x03; 
    TWCR = (1<<TWEN); 
}

uint8_t I2C_start(){
    TWCR = (1<<TWINT) | (1<<TWSTA) | (1<<TWEN);
    while (!(TWCR & (1<<TWINT)));          
    if (((TWSR & 0xF8) != START) && ((TWSR & 0xF8) != RESTART)) return 0;
    return 1;
}

uint8_t I2C_slave(uint8_t SLA_WR){
    TWDR = SLA_WR;                 
    TWCR = (1<<TWINT) | (1<<TWEN);   
    while (!(TWCR & (1<<TWINT))); 
    if (((TWSR & 0xF8) != MT_SLA_ACK) && ((TWSR & 0xF8) != MR_SLA_ACK)) return 0;
    return 1;
}

uint8_t I2C_write(uint8_t DATA){
    TWDR = DATA;                 
    TWCR = (1<<TWINT) | (1<<TWEN);  
    while (!(TWCR & (1<<TWINT))); 
    if ((TWSR & 0xF8) != MT_DATA_ACK) return 0;
    return 1;
}

uint8_t I2C_read_ack(void){
    TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWEA); 
    while (!(TWCR & (1<<TWINT)));
    if ((TWSR & 0xF8) != MR_DATA_ACK) return 0;
    return TWDR;
}

uint8_t I2C_read_nack(){
    TWCR = (1<<TWINT) | (1<<TWEN); 
    while (!(TWCR & (1<<TWINT)));
    if ((TWSR & 0xF8) != MR_DATA_NACK) return 0;
    return TWDR;
}

void I2C_stop(void){
    TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWSTO); 
}

// ---------------- Timer Initialization ----------------

/**
 * @brief Initializes Timer1 in Fast PWM (Mode 14) for DC Motor Control.
 */
void timer1_init(void) {
    TCCR1A = (1 << COM1A1) | (1 << WGM11);						
    TCCR1B = (1 << WGM13) | (1 << WGM12) | (1 << CS10); // Fast PWM, ICR1 TOP, Prescaler 1			
    ICR1 = 320; // 50 kHz PWM frequency
    OCR1A = ICR1 / 2; // 50% initial duty
    
    PWM_MOTOR_DDR |= (1 << PWM_MOTOR_PIN);
    
    // Direction Control Ports
    DDRA |= (1 << 0) | (1 << 1) | (1 << 2);
    PORTA &= ~((1 << 0) | (1 << 1) | (1 << 2));
}

/**
 * @brief Initializes Timer0 in Phase Correct PWM (Mode 5) for Servo Motor Control.
 */
void init_timer0(){
    TCCR0A |= (1 << WGM00) | (1 << COM0B1);	
    TCCR0B |= (1 << WGM02) | (1 << CS02) | (1 << CS00); // Prescaler 1024
    OCR0A = 156; // TOP value for 50Hz
    
    ancho_pulso = 8 + (grados * 0.07);
    OCR0B = ancho_pulso;
    TCNT0 = 0;
    
    DDRG |= (1 << 5); // Servo output pin
}

// ---------------- Sensor Implementations ----------------

/**
 * @brief HC-SR04 Ultrasonic driver.
 */
void HCSR04Init() {
    US_DDR |= (1 << US_TRIG_POS);
}

void HCSR04Trigger() {
    US_PORT |= (1 << US_TRIG_POS);
    _delay_us(10);  
    US_PORT &= ~(1 << US_TRIG_POS);
}

uint16_t GetPulseWidth() {
    uint32_t i, result;

    for (i = 0; i < 600000; i++) {
        if (!(US_PIN & (1 << US_ECHO_POS))) continue;
        else break;
    }

    if (i == 600000) return US_ERROR;

    TCCR4A = 0X00;
    TCCR4B = (1 << CS41); // Prescaler 8
    TCNT4 = 0x00;

    for (i = 0; i < 600000; i++) {
        if (US_PIN & (1 << US_ECHO_POS)) {
            if (TCNT4 > 60000) break;
            else continue;
        } else break;
    }

    if (i == 600000) return US_NO_OBSTACLE;

    result = TCNT4;
    TCCR4B = 0x00;

    if (result > 60000) return US_NO_OBSTACLE;
    else return (result >> 1);
}

void ultrasonico(void){
    HCSR04Trigger();
    uint16_t r = GetPulseWidth();
    char buffer2[20];

    if (r == US_ERROR) {
        sprintf(buffer2, "E\r\n");
    } else {
        int distance = (r * 0.0345 / 2.0);
        sprintf(buffer2, "%d\n\r", distance);
    }
    usart_puts(buffer2);
    _delay_ms(100);
}

/**
 * @brief LM35 Temperature Sensor (Internal ADC).
 */
void ADC_Init() {
    ADMUX = (1 << REFS0);												
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); 
}

int ADC_Read(char channel) {
    ADMUX = (1 << REFS0) | (channel & 0x07);  
    ADCSRA |= (1 << ADSC);                    
    while (ADCSRA & (1 << ADSC));             
    return ADC;                               
}

void LM35(void) {
    int adc_value = ADC_Read(0);              
    float cels = (adc_value * 5.0) / 1024.0 * 100.0;  
    sprintf(Temperature, "%.2fC ", cels);  
    usart_puts(Temperature);
    _delay_ms(100);
}

/**
 * @brief MPU6050 Accelerometer & Gyroscope (I2C).
 */
void Coords0(int16_t* ax0, int16_t* ay0, int16_t* az0, int16_t* gx0, int16_t* gy0, int16_t* gz0) {
    I2C_start();
    I2C_write(MPU6050_ADDR << 1 | 0);
    I2C_write(0x3B); 
    I2C_start();
    I2C_write(MPU6050_ADDR << 1 | 1);
    *ax0 = ((int16_t)I2C_read_ack() << 8) | I2C_read_ack();
    *ay0 = ((int16_t)I2C_read_ack() << 8) | I2C_read_ack();
    *az0 = ((int16_t)I2C_read_ack() << 8) | I2C_read_ack();
    *gx0 = ((int16_t)I2C_read_ack() << 8) | I2C_read_ack();
    *gy0 = ((int16_t)I2C_read_ack() << 8) | I2C_read_ack();
    *gz0 = ((int16_t)I2C_read_ack() << 8) | I2C_read_nack();
    I2C_stop();
}

void MPU6050_init() {
    I2C_start();
    I2C_write(MPU6050_ADDR << 1);
    I2C_write(0x6B);
    I2C_write(0x00); // Wake up
    I2C_start();
    I2C_write(MPU6050_ADDR << 1);
    I2C_write(0x1B);
    I2C_write(0x08);
    I2C_start();
    I2C_write(MPU6050_ADDR << 1);
    I2C_write(0x1C);
    I2C_write(0x08);
}

void MPU6050_readData(int16_t* ax, int16_t* ay, int16_t* az, int16_t* gx, int16_t* gy, int16_t* gz) {
    I2C_start();
    I2C_write(MPU6050_ADDR << 1 | 0);
    I2C_write(0x3B); 
    I2C_start();
    I2C_write(MPU6050_ADDR << 1 | 1);
    *ax = ((int16_t)I2C_read_ack() << 8) | I2C_read_ack();
    *ay = ((int16_t)I2C_read_ack() << 8) | I2C_read_ack();
    *az = ((int16_t)I2C_read_ack() << 8) | I2C_read_ack();
    *gx = ((int16_t)I2C_read_ack() << 8) | I2C_read_ack();
    *gy = ((int16_t)I2C_read_ack() << 8) | I2C_read_ack();
    *gz = ((int16_t)I2C_read_ack() << 8) | I2C_read_nack();
    I2C_stop();
}

void MPU6050(){
    MPU6050_readData(&accelX, &accelY, &accelZ, &gyroX, &gyroY, &gyroZ);
    AX = accelX - accelX0;
    AY = accelY - accelY0;
    AZ = accelZ - accelZ0;
    GX = gyroX - gyroX0;
    GY = gyroY - gyroY0;
    GZ = gyroZ - gyroZ0;
    
    // Bug Fix: Replaced repeated AX with proper AY and AZ variables
    sprintf(stra, "Cordenada en X %d\n", AX); usart_puts(stra);
    sprintf(stra, "Cordenada en Y %d\n", AY); usart_puts(stra);
    sprintf(stra, "Cordenada en Z %d\n", AZ); usart_puts(stra);
    sprintf(stra, "Giro en X %d\n", GX); usart_puts(stra);
    sprintf(stra, "Giro en Y %d\n", GY); usart_puts(stra);
    sprintf(stra, "Giro en Z %d\n\r", GZ); usart_puts(stra);
    _delay_ms(100);
}

/**
 * @brief BH1750 Ambient Light Sensor (I2C).
 */
uint16_t BH1750_read(){
    uint16_t lux;
    I2C_start();
    I2C_slave(0x46);
    I2C_write(0x10); // High-res mode
    I2C_stop();
    _delay_ms(180);
    
    I2C_start();
    I2C_slave(0x47);
    lux = I2C_read_ack() << 8;
    lux |= I2C_read_nack();
    I2C_stop();
    
    return lux / 1.2;
}

void BH1750(void){
    uint16_t lux = BH1750_read();
    sprintf(sata, "%u\n\r", lux);
    usart_puts(sata);  
    _delay_ms(100);
}

/**
 * @brief MAX6675 Thermocouple Sensor (SPI).
 */
float leerTemperatura(void){
    uint16_t temperatura;
    
    PORTB &= ~(1<<0); 
    _delay_ms(1); 
    
    // Bug Fix: Using spi_transfer to prevent hardware flag deadlocks
    temperatura = spi_transfer(0x00);
    temperatura <<= 8;
    temperatura |= spi_transfer(0x00);
    
    PORTB |= (1<<0); 
    
    temperatura >>= 3; // Shift to remove status bits
    return temperatura;
}

void MAX6675(void){
    float temp = leerTemperatura();
    UART_sendFloat(temp * 0.25);
    usart_puts("\r\n");
    _delay_ms(500);
}

/**
 * @brief Actuates DC motor speed.
 */
void velocidad_motor(){
    for(uint8_t m = 3; m < strlen(string); m++){
        if(!isdigit((unsigned char)string[m])){
            valid = 0;
            break;
        }
    }
    if(valid){
        pwm_duty_cycle = atoi(&string[3]);
        OCR1A = ICR1 * pwm_duty_cycle / 100;
    }
}

/**
 * @brief MCP4725 Digital-to-Analog Converter (I2C).
 */
void MCP4725(uint16_t MCP){
    // Bug Fix: Corrected bit shifting for MCP4725 Fast Mode standard
    uint8_t msb = (MCP >> 8) & 0x0F; 
    uint8_t lsb = MCP & 0xFF; 
    
    I2C_start(); 
    I2C_write((MCP4725_ADDR << 1) | 0); 
    I2C_write(msb); 
    I2C_write(lsb); 
    I2C_stop(); 
}

void VALOR_ACD(){
    uint16_t MC = 0;
    for(uint8_t m = 3; m < strlen(string); m++){
        if(!isdigit((unsigned char)string[m])){
            valid = 0;
            break;
        }
    }
    if(valid){
        MC = atoi(&string[3]);
        MCP4725(MC);
        usart_puts("Voltaje: ");
        UART_sendFloat((float)MC * 5 / 4096);
        usart_puts("\r\n");
        _delay_ms(500); 
    }
}

/**
 * @brief Actuates Servo motor angle.
 */
void SERVOMOTOR(){
    for(uint8_t m = 3; m < strlen(string); m++){
        if(!isdigit((unsigned char)string[m])){
            valid = 0;
            break;
        }
    }
    if(valid){
        grados = atoi(&string[3]);
        ancho_pulso = 8 + (grados * 0.07);
        OCR0B = ancho_pulso;
    }
}

// ---------------- Command Parser ----------------

/**
 * @brief Executes actions based on USART commands.
 */
void accion(){
    if (strcmp(string, "LM35") == 0) LM35();
    else if (strcmp(string, "ULTRASONICO") == 0) ultrasonico();
    else if (strcmp(string, "GPS") == 0) recibir_gps = 1;
    else if (strcmp(string, "GPS_OFF") == 0) recibir_gps = 0;
    else if (strcmp(string, "MPU6050") == 0) MPU6050();
    else if (strcmp(string, "BH1750") == 0) BH1750();
    else if (strcmp(string, "MAX6675") == 0) MAX6675();
    else if (strcmp(string, "ACTIVAR_M") == 0) PORTA |= (1 << 0);
    else if (strcmp(string, "DESACTIVAR_M") == 0) PORTA &= ~(1 << 0);
    else if (strcmp(string, "IZQUIERDA") == 0) {
        PORTA &= ~(1 << 1);
        PORTA |= (1 << 2);
    }
    else if (strcmp(string, "DERECHA") == 0) {
        PORTA &= ~(1 << 2);
        PORTA |= (1 << 1);
    }
    else if (strncmp(string, "MOT", 3) == 0) {
        valid = 1; velocidad_motor(); valid = 0;
    }
    else if (strncmp(string, "MCP", 3) == 0) {
        valid = 1; VALOR_ACD(); valid = 0;
    }
    else if (strncmp(string, "SRV", 3) == 0) {
        valid = 1; SERVOMOTOR(); valid = 0;
    }
}

// ---------------- Main Loop ----------------

int main(void){
    // Initialize Peripherals
    I2C_init();
    init_usart0();
    init_USART3();
    ADC_Init();
    init_SPI();
    init_timer0();
    timer1_init();

    // Initialize Sensors
    HCSR04Init();
	MPU6050_init();
    // Coords0(&accelX0, &accelY0, &accelZ0, &gyroX0, &gyroY0, &gyroZ0);
    
    usart_puts("CONEXION ESTABLECIDA\n\r");
    sei();
    
    while (1){ 
        // Main execution handled asynchronously by interrupts
    }
}

// ---------------- Interrupt Service Routines (ISRs) ----------------

ISR(USART0_RX_vect){
    seleccion = UDR0;
    if (seleccion != '\r' && seleccion !='\n'){		
        string[j] = seleccion;
        j++;
    }
    if (seleccion == '\n' || seleccion == '\r'){		
        string[j] = '\0';
        // Bug Fix: Filter out empty strings caused by \r\n double-triggers
        if (j > 0) {
            accion();
        }
        j = 0;
        string[0] = '\0';
    }
}   

ISR(USART3_RX_vect){
    if(recibir_gps == 1){
        GPS = UDR3;
    
        if (GPS == '$') k = 0; 

        string_gps[k] = GPS;
        k++;

        if (GPS == '\n' || GPS == '\r'){		
            string_gps[k] = '\0';
            if(strncmp(string_gps, "$GPGGA", 6) == 0){
                usart_puts3();
            }
        }
    }
}