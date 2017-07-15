unsigned char *portB_DDR;
unsigned char *portB_pin_Write;
# define BIT5_MASK 0x20

void setup() {
  // put your setup code here, to run once:
// criar o ponteiro e atribuir o endereço do registrador
portB_DDR = (unsigned char*) 0x24;
portB_pin_Write = (unsigned char *) 0x25;


// Escrita
*portB_DDR = *portB_DDR | BIT5_MASK; // todos os bits são de saída

}

void loop() {
  // put your main code here, to run repeatedly:
*portB_pin_Write = *portB_pin_Write | BIT5_MASK;
delay(1000);
*portB_pin_Write = *portB_pin_Write & ~BIT5_MASK;
delay(1000);
}
