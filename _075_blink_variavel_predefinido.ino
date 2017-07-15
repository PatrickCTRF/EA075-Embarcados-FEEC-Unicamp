//DDRB é ponteiro dereferenciado para o registrador DDRB
//PORTB é ponteiro dereferenciado para o registrador PORTB
# define BIT5_MASK 0x20

void setup() {
  // put your setup code here, to run once:


// Escrita
DDRB = DDRB | BIT5_MASK; // todos os bits são de saída

}

void loop() {
  // put your main code here, to run repeatedly:
PORTB = PORTB | BIT5_MASK;
delay(1000);
PORTB = PORTB & ~BIT5_MASK;
delay(1000);
}
