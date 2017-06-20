//DDRB é ponteiro dereferenciado para o registrador DDRB
//PORTB é ponteiro dereferenciado para o registrador PORTB
//PIND e o registrador usado para leitura dos pinos 0 a 7.

# define BIT0_MASK 0b00000001
# define BIT1_MASK 0b00000010
# define BIT2_MASK 0b00000100
# define BIT3_MASK 0b00001000
# define BIT4_MASK 0b00010000
# define BIT5_MASK 0b00100000
# define BIT6_MASK 0b01000000
# define BIT7_MASK 0b10000000

#define AMOSTRAGEM 96//Quantidade de amostras que serao colhidas para determinar a velocidade do motor (minimizando o erro). Colhe duas voltas de amostras ([48pulsos por volta]*2 = 96).

int teste = 0;
int milisegundos = 0;//Variaveis que cronometram o tempo decorrido desde o inicio do sistema.
long segundos = 0;
double tempoAtual = 0;//Um double que indica em um "timestamp", de [segundos,milisegundos], quanto tempo deccoreu do inicio do sistema. Extremamente util para ordenar eventos sob tempo real decorrido com poucas comparaçoes.

double botao1 = 0;//Variaveis que guardam o momento de ocorrencia de eventos em pinos especificos
double botao2 = 0;
double encoderA = 0;
double encoderB = 0;
int n_amostras_encoder = 0;


double old_botao1 = 0;//Variaveis para debouncing.
double old_botao2 = 0;
double old_encoderA = 0;
double old_encoderB = 0;
double old_tempoAtual = 0;
double threshold = 0.02;

int velocidade = 0;//Varia de -20 a +20. Diretamente proporcional a velocidade do motor.
int display_sentido = 0;//1 e -1 indicam rotaçao no sentido horario e anti horario, respectivamente.
double display_velocidade = 0;//Indica a velocidade medida atraves do encoder.

void setup() {
  
cli(); //desativa interrupções globais.

// Escrita ou Leitura
DDRB = DDRB|(BIT5_MASK)|(BIT4_MASK)|(BIT3_MASK); // Os bits 5,4 e 3 (pinos 13, 12 e 11, respectivamente) sao de saida.
DDRD = DDRD & ~(BIT2_MASK|BIT3_MASK|BIT4_MASK|BIT5_MASK);// 2 a 5 sao pinos sao de entrada para usarmos em interrupçes.


//EICRA = 0b11;//Ativa em borda de subida a interrupção.
//EIMSK = 0b1;//Ativa a interrupção INT0.

PCMSK2 = 0b11100;//Ativa interrupçao nos bits dos pinos 2 a 4 e desabilita-a nos demais DESTE registrador.
PCICR = (BIT2_MASK);//Ativa as interrupçes dos pinos de bits setado no registrador PCMSK2, desativa nos demais PCMSK.

init_PWM();

sei();//Ativa interrupções globais.

Serial.begin(9600);// A taxa na qual nos comunicmos com o Serial (monitor/plotter). Valores usuais: 9600 230400

initTimer1();

}

void loop() {
  
  //teste = PIND & 0b11111100;
  
 /**
  * Desejamos que o motor gire de 0 a 1000RPM em cada sentido, sendo que
  * o mesmo gira a 1315RPM quando alimentado com 7ob um PWM de 255.
  * Sendo assim, precisamos de um PWM de 194 para atingir o valor 1000RPM.
  * do motor.
 */ 
  if(velocidade>=0){
    OCR2A = (int)(velocidade*194)/20;//Regra de 3 para setar a velocida do motor pelo pwm. 100 esta para 93, assim como velocidade atual esta para X, onde X  o que devemos colocar em OCR2A.
    PORTB |= BIT4_MASK;
    PORTB &= ~BIT5_MASK;
  }else{
    OCR2A = (int)(-velocidade*194)/20;//Se a velocidade for negativa, o motor gira para o outro lado, mas o PWM é o mesmo.
    PORTB |= BIT5_MASK;
    PORTB &= ~BIT4_MASK;
  }
  
  Serial.println(display_sentido);
  Serial.println(display_velocidade);
  
}

ISR (INT0_vect) {

}

ISR (INT1_vect) {

}


ISR (PCINT2_vect) {

  if(PIND & BIT2_MASK){//Se a interrupçao foi gerada por uma borda de SUBIDA no pino 2... (Se a borda de descida que gerou a interrupçao, este bit valera zero e nao sera executado).
    
    botao1 = tempoAtual;//Quando este botao for pressionado, armazene nele o momento em queo evento ocorreu.
    
    if( botao1 >= old_botao1 + threshold){//Debouncing
      old_botao1 = botao1;
      
      if(velocidade<20) velocidade++;//Se a velocidadenao esta no valor maximo, incremente-a
      
    }
    
  }
  
  if(PIND & BIT3_MASK){//Se a interrupçao foi gerada por uma borda de SUBIDA no pino 3... (Se a borda de descida que gerou a interrupçao, este bit valera zero e nao sera executado).
    
    //old_botao2 = botao2;
    botao2 = tempoAtual;//Quando este botao for pressionado, armazene nele o momento em queo evento ocorreu.
    
    if( botao2 >= old_botao2 + threshold){//Debouncing
      old_botao2 = botao2;
      
      if(velocidade>-20) velocidade--;//Se a velocidadenao esta no valor maximo, incremente-a
      
    }
    
  }
  
  if(PIND & BIT4_MASK){//Se a interrupçao foi gerada por uma borda de SUBIDA no pino 4... (Se a borda de descida que gerou a interrupçao, este bit valera zero e nao sera executado).
    
    
    encoderA = tempoAtual;//Quando o encoder gerar um pulso, armazene nele o momento em queo evento ocorreu.
   
    if(PIND & BIT5_MASK){//O que define o sentido de rotaçao  a ordem de acionamento dos canais do encoder.
     display_sentido = 1;//Se o canal B ja estava acionado quando A foi acionado, o sentido eh horario.
    }else{
     display_sentido = -1;//Se nao,  anti horario.
    }
    
    /**
     *Sempre que chamar esta interrupço, significa que ocorreu um pulso no encoder canal A e incrementa o nmero de amostras.
     *Nao eh necssario variavel de armazenamento temporario auxiliar, pois a variavel old_encoderA ja guarda o ultimo momento em que terminamos de colher a amostra.
     */
    
    if(++n_amostras_encoder>=AMOSTRAGEM){//Quando possuirmos um numero de amostras igual a AMOSTRAGEM, realizamos a media de velocidade.colehmos
      
      display_velocidade = (60*AMOSTRAGEM)/(48*(encoderA-old_encoderA));//Como o periodo medido  apenas 1/48 do periodo de umarotaçao completa, devemos multiplica-lo por 48. 60vezes eh para converter de hertz para RPM. A quantidade de AMOSTRAGEM clhidas diminui, proporcionalmente, o periodo.
      old_encoderA = encoderA;//Atualiza qual o novo valor de old_encoderA.
      n_amostras_encoder = 0;
      
    }
    
    //Multiplicar por 5/4 eh o fator de correçãoda amostra.
    //display_velocidade = (60*5/4)/(48*(encoderA-old_encoderA));//Como o periodo medido  apenas 1/48 do periodo de umarotaçao completa, devemos multiplica-lo por 48. 60vezes eh para converter de hertz para RPM.
    
  }
}

/**
 * Configuraçoes do Timer/Counter 2 para modo Fast PWM.
 * O pino que recebe o sinal do PWM, neste caso, eh o 22.
 */
void init_PWM(){

  DDRB |= BIT3_MASK; // pino digital 11 do Arduino (OC2A) configurado como saÃda
  
  // Timer/Counter 2 Control Register A
  TCCR2A = (1<<COM2A1)|(1<<COM2B1)|(1<<WGM21)|(1<<WGM20);
  
  // Output Compare 2 Register A
  OCR2A = 0; // Controla o duty cycle da saida OC2A (pino 22). Faixa de valores: 0 a 255
 
  // Timer/Counter 2 Control Register B
  TCCR2B = (1<<CS20); // Configura o prescaler para nos passar o clock da placa.
}

/**
 * Esta interrupçao ocorre sempre que o contador atinge o valor maximo da contagem, que,
 * na atual configuraçao, vale OCR2A.
 * Esta interrupçao ser chamada a cada 2ms, incrementando a variavel milisegundos e a 
 * variavel segundos (esta somente a cada 2000 chamadas).
 * O valor destas variaveis eh convertido para um double, de forma que podemos facilmente 
 * comparar a ordem cronologica entre dois eventos.
 */
ISR(TIMER1_OVF_vect){
 
 TCNT1 = 0;//Zere a contagem apos atingirmos o valor alvo (OCR1A).
 milisegundos++;//Indique que se passou mais 1 milisegundo.
 
 if(milisegundos>=1000){//A cada passagem de 1000 milisegundos, acrescente 1 segundo e zere a contagem dos milisegundos.
    milisegundos = 0;
    segundos++;
  }
  
  tempoAtual = segundos + milisegundos/(double)1000;//Concatene os dados para podermos comparar a ORDEM DOS EVENTOS mais facilmente - como se fosse o timestamp UNIX.
  
  TIFR1 |= (1<<OCR2A)|(1<<TOIE2);//Zera as flags relacionadas a esta interrupçao. Elas supostamente se zeram sozinhas, estamos somente garantindo isso para nao perder tempo debuggando.
}


/**
 * Esta funço inicializa o Timer 1 atraves
 * dos registradores necessrios.
 */
void initTimer1(){
  
  // TCNT1H e TCNT1L - TIMER/COUNTER1 REGISTER
  // Inicia o contador com o valor 0x0000
  TCNT1 = 0;
  
  // TCCR1A - TIMER/COUNTER1 CONTROL REGISTER A
  // COM1A1 = 0; COM1A0 = 0 - OC1A desabilitado
  // COM1B1 = 0; COM1B0 = 0 - OC1B desabilitado
  // WGM11 = 0; WGM10 = 0 - Operação Fast PWM do timer: TOP = OCR2A e TOV2 flag ativada quando o contador atinge
  TCCR1A = (1<<WGM11)|(1<<WGM10);
  
  // TIMSK1 - Timer/Counter 1 Interrupt Mask Register
  // ICIE (Input Capture Interrupt Enable) = 0 - Interrupções de captura desabilitadas.
  // OCIEB (Output Compare B Match Interrupt Enable) = 0 - Interrupções geradas pela comparação com o registrador B desabilitadas.
  // OCIEA (Output Compare A Match Interrupt Enable) = 0 - Interrupções geradas pela comparação com o registrador A desabilitadas.
  // TOIE (Overflow Interrupt Enable) = 1 - As iterrupções relacionadas ao overflow do contador estão habilitadas.
  TIMSK1 = (1<<TOIE1);
  
  // TCCR1B - TIMER/COUNTER1 CONTROL REGISTER B
  // ICNC1 = 0 - O modo de captura de entrada não está sendo usado
  // ICES1 = 0 - O modo de captura de entrada não está sendo usado
  // WGM13 = 0; WGM12 = 0 - Operação fast PWM do timer: TOP = OCR1A e TOV1 flag ativada quando o contador atinge
  // contagem igual a TOP
  // CS12-0 = 011 - Pre-scaler = 64 -> clock do contador é 26MHz/64 = 250kHz. Portanto, cada ciclo de contagem de 0 a 250 (Valor de OCR1A) dura
  // 250 * 1/250000 = 1ms.
  TCCR1B = (1<<WGM13)|(1<<WGM12)|(1<<CS11)|(1<<CS10); 
  
  OCR1A = 250;//Valor o qual, quando TCNT1 atingir, fara ser executada a interrupço de overflow A.
}
