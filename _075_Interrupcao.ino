//DDRB é ponteiro dereferenciado para o registrador DDRB
//PORTB é ponteiro dereferenciado para o registrador PORTB
# define BIT5_MASK 0x20
# define BIT2_MASK 0b100;

void setup() {
  // put your setup code here, to run once:


// Escrita ou Leitura
DDRB = DDRB | BIT5_MASK; // os bits são de saída
//DDRD = DDRD & ~BIT2_MASK; // os bits são de entrada

EICRA = 0b11;//Ativa em borda de subida a interrupção.
EIMSK = 0b1;//Ativa a interrupção INT0

sei();//Ativa interrupções globais.
//cli(); desativa interrupções globais.

}

void loop() {
  // put your main code here, to run repeatedly:

}

void muda_Estado(){
  
  if(PORTB & BIT5_MASK){//Se vale 1, led já estava aceso.
    PORTB = PORTB & ~BIT5_MASK;
    //delay(1000);
  }else{
    PORTB = PORTB | BIT5_MASK;
    //delay(1000);
  }
  
}

ISR (INT0_vect) {
  muda_Estado();
}


