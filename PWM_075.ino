

int i =0;


void setup(){
  
  DDRB = 0b1000;
  DDRD = 0b1000;
  
  TCCR2A = 0b00100001;//Caso ocorra erro, verifique se ocorre por estarmos setando pinos q sejam somente leitura.
  TCCR2B = 0b00001111;

  OCR2A = 156;
  
}



void loop(){
 
  
  OCR2B = 5;//Para largura de 0,5ms.
  delay(2000);
  OCR2B = 12;//Para 1,5ms.
  delay(2000);
  OCR2B = 20;//PAra 2,5ms.
  delay(2000);
  
  for(i=0;i<156;i+=2){
    OCR2B = i;
    delay(100);
  }
  
  for(i=156;i>0;i-=2){
    OCR2B = i;
    delay(100);
  }
}
  

