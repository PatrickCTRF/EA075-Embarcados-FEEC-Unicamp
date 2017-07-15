/**
 * Senóide sai do pino D11 e a leitura é no pino A0. O filtro RC passa baixa deve ir do 11 até o GND com saída em A0. Resistor no 11, capacitor no GND.
 */

unsigned char seno[20] = {128,167,202,231,249,255,249,231,202,167,128,88,53,24,6,0,6,24,53,88};

int leituras[500]= {128,167,202,231,249,255,249,231,202,167,128,88,53,24,6,0,6,24,53,88};
int j = 0;
int pronto = 0;

void init_PWM(){
  // ConfiguraÃ§Ãµes do Timer/Counter 2 para modo Fast PWM
  DDRB = 0b00001000; // pino digital 11 do Arduino (OC2A) configurado como saÃda
  
  // Timer/Counter 2 Control Register A
  TCCR2A = 0b10100011;
  
  // Output Compare 2 Ragister A
  OCR2A = 0; // Controla o duty cycle da saÃda OC2A (pino 11). Faixa de valores: 0 a 255
 
  // Timer/Counter 2 Control Register B
  TCCR2B = 0b00000001; // Configura pre-scaler: sem prescaler
}

void setup() {
  //cli();
  ADCSRA  = 0b11101111;//10001111   0b11101000
  ADMUX = (1<<5)|(1<<6);
  ADCSRB = 0;
  Serial.begin(230400);
  init_PWM();
  sei();
  DIDR0 = 0xF;
}

void loop() {
  if(pronto == 0){
        for (int i = 0; i < 20; i++) {
      OCR2A = seno[i];
      delayMicroseconds(200);
    }
  }
  if(pronto == 1){
    for (int i = 0; i < 500; i++) {
      Serial.println(leituras[i]);
    }
    delay(2000);
    pronto = 0;
  }
}

ISR(ADC_vect)
{
 if(j==500){j=0;pronto = 1;}

  if(pronto == 0)leituras[j++] = ADCH;
}
