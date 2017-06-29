/**
 * Este código faz o acionamento de um motor CC sob diferentes velocidades
 * utilizando PWM. 
 * O botão da esquerda faz ele girar mais para o sentido anti horário e o
 * da direita para o horário.
 * A velocidade e o sentido de rotação são MEDIDOS e exibidos no display.
 * A velocidade e o sentido ALVO (definidos pela quantidade de cliques nos botoes)
 * também serão exibidos n display.
 */

# define BIT0_MASK 0b00000001
# define BIT1_MASK 0b00000010
# define BIT2_MASK 0b00000100
# define BIT3_MASK 0b00001000
# define BIT4_MASK 0b00010000
# define BIT5_MASK 0b00100000
# define BIT6_MASK 0b01000000
# define BIT7_MASK 0b10000000

# define TAM_ARRAY 8

//Variaveis que cronometram o tempo decorrido desde o inicio do sistema.
int milisegundos = 0;
long segundos = 0;
double tempoAtual = 0;//Um double que indica em um "timestamp" de [segundos,milisegundos] quanto tempo deccoreu do inicio do sistema. Extremamente util para ordenar eventos sob tempo real decorrido com poucas comparaçoes.

//Variaveis que guardam o momento de ocorrencia de eventos em pinos especificos
double botao1 = 0;
double botao2 = 0;
double encoderA = 0;
double encoderB = 0;
int n_amostras_encoder = 0;

//Variaveis para debouncing.
double old_botao1 = 0;
double old_botao2 = 0;
double old_encoderA = 0;
double old_encoderB = 0;
double old_tempoAtual = 0;
double threshold = 0.02;//Temporização para fazer o debouncing.

int velocidade = 0;//Varia de -20 a +20. Diretamente proporcional a velocidade do motor.
int display_sentido = 0;// -1 e 1 indicam rotaçao no sentido horario e anti horario, respectivamente.
double display_velocidade = 0;//Indica a velocidade medida atraves do encoder.
double old_display_velocidade = 0;//Serve para verificarmos se a velocidade do motor mudou em relação à última medida que fizemos, para evitar imprimir sucessivos valores equivalentes no display.
double speed_threshlod = 0;
int AMOSTRAGEM = 4;//Indica quantas amostras devem ser obtidas para a média de cada velocidade do motor. Esta quantidade será alterada no decorrer do programa.

char imprime[TAM_ARRAY] = "teste\n";//Um array/string que armazena os dados para serem nviados à UART.


void setup() {//Rotina que só ocorre uma única vez, usada para, em geral, configuração do sistema.
  
  cli(); //desativa interrupções globais.

  initIO();//Configura os botões E SAÍDAS necessários

  initInterrupt();//Configura as interrupções que usaremos nos pinos de leitura.

  init_PWM();//Inicia o PWM.
  
  init_UART();
  UART_String("\n\nO Simulador pode \ndemorar ate 3s \npara atualizar. \nSeja Paciente. \n\nMotor ATIVO");//Mensagem para o usuário.

  initTimer1();//Inicia a base de tempo.

  sei();//Ativa interrupções globais.

}

void loop() {//Rotina que, ao chegar em seu fim, é reexecutada sucessivamente.
  
 /**
  * Desejamos que o motor gire de 0 a 1000RPM em cada sentido, sendo que
  * o mesmo gira a 1232RPM quando alimentado com 6,4V sob um PWM de 255.
  * Sendo assim, precisamos de um PWM de 207 para atingir o valor 1000RPM
  * do motor.
  */ 
  if(velocidade>0){
    OCR2A = (int)(velocidade*207)/20;//Regra de 3 para setar a velocida do motor pelo pwm. 1000 esta para 207, assim como velocidade atual esta para X, onde X  o que devemos colocar em OCR2A.
    PORTB |= BIT4_MASK;
    PORTB &= ~BIT5_MASK;
  }
  
  if(velocidade==0){
    OCR2A = (int)1;//Menor velocidade possível sem o motor parado, pois o simulador buga quando para.
    PORTB |= BIT4_MASK;
    PORTB &= ~BIT5_MASK;
  }
  
  if(velocidade<0){
    OCR2A = (int)(-velocidade*207)/20;//Se a velocidade for negativa, o motor gira para o outro lado, mas o PWM é o mesmo.
    PORTB |= BIT5_MASK;
    PORTB &= ~BIT4_MASK;
  }
  
  
  //Esta condição garante que só atualizaremos o display quando houver variação significativa na velocidade.
  if(display_velocidade >= old_display_velocidade + speed_threshlod || display_velocidade < old_display_velocidade - speed_threshlod){
    
    if(velocidade>=0){
      UART_String("\n\nSentido Alvo: Anti Horario\n");
      UART_String("Speed Alvo: ");
      doubleToAscii10((double)velocidade*50, imprime);
      UART_String(imprime);
      UART_String("\n");
    }
    if(velocidade<0){
      UART_String("\n\nSentido Alvo: Horario\n");
      UART_String("Speed Alvo: ");
      doubleToAscii10((double)-velocidade*50, imprime);
      UART_String(imprime);
      UART_String("\n");
    }
    
    if(display_sentido>=0){UART_String("Sentido Medido: Anti Horario\n");}
    else{UART_String("Sentido Medido: Horario\n");}
    
    UART_String("Speed Medida: ");
    doubleToAscii10(display_velocidade, imprime);
    UART_String(imprime);
    UART_String("\n");
    old_display_velocidade = display_velocidade;//Atualizamos o valor da última velocidade impressa na display.
    
  }
  
    /**
     * Nossa margem de erro de leitura é 1/100 do valor de velocidade atual (para mais ou para menos).
     * Isto nos auxilia a verificar se a velocidade mudou realmente ou se foi apenas uma oscilação na 
     * leitura, a qual não precisamos imprimir no display.
     * (Nunca vale zero, pois acrescentamos 0.5 fixos).
     */
  	if(velocidade>=0){speed_threshlod = velocidade/(double)2 + 0.5;}
    else{speed_threshlod = -velocidade/(double)2 + 0.5;}
                     
    /**
     * Para que  velocidade medida seja mais proximo do valor real, tiramos a média das medidas. Em velocidades baixas, as medidas demoram
     * mais para serem adquiridas, por isso a taxa de amostragem é proporcional à velocidade.
     */
  	if(velocidade>=0){AMOSTRAGEM = velocidade*5;}
    else{AMOSTRAGEM = -velocidade*5;}
  
}

ISR (INT0_vect){ //Se a interrupçao foi gerada por uma borda de SUBIDA no pino 4... (Se a borda de descida que gerou a interrupçao, este bit valera zero e nao sera executado).
    
    encoderA = tempoAtual;//Quando o encoder gerar um pulso, armazene nele o momento em que o evento ocorreu.
   
    if(PIND & BIT5_MASK){//O que define o sentido de rotaçao é a ordem de acionamento dos canais do encoder.
     display_sentido = 1;//Se o canal B ja estava acionado quando A foi acionado, o sentido é anti horario.
    }else{
     display_sentido = -1;//Se nao, horario.
    }
    
    /**
     *Sempre que chamar esta interrupço, significa que ocorreu um pulso no encoder canal A e incrementa o número de amostras colhidas.
     *Nao eh necessario variavel de armazenamento temporario auxiliar, pois a variavel old_encoderA ja guarda o ultimo momento em que terminamos de colher a amostra.
     */
    
    if(n_amostras_encoder++>=AMOSTRAGEM){//Quando possuirmos um numero de amostras igual a AMOSTRAGEM, realizamos a media de velocidade.
      
      display_velocidade = (60*(n_amostras_encoder))/(48*(encoderA-old_encoderA));//Como o periodo de pulso do encoder é apenas 1/48 do periodo de uma rotaçao completa, devemos multiplica-lo por 48. 60vezes eh para converter de hertz para RPM. A quantidade de n_amostras_encoder colhidas diminui proporcionalmente ao periodo.
      old_encoderA = encoderA;//Atualiza qual o novo valor de old_encoderA.
      n_amostras_encoder = 0;//Reiniciamos a variável para tornarmos a colher outra amostra de velocidade.
      
    }
    
  }



ISR (PCINT2_vect) {//Rotina de interrupção que trata as chamadas referentes a ambos pinos 3 e 4, sempre que ocorrer VRIAÇÃO de nível lógico num destes.
  
  if(PIND & BIT3_MASK){//Se a interrupçao foi gerada por uma borda de SUBIDA no pino 3... (Se a borda de descida que gerou a interrupçao, este bit valera zero e nao sera executado).
    
    botao2 = tempoAtual;//Quando este botao for pressionado, armazene nele o momento em que o evento ocorreu.
    
    if( botao2 >= old_botao2 + threshold){//Debouncing
      old_botao2 = botao2;
      
      if(velocidade>-20) velocidade--;//Se a velocidadenao esta no valor maximo, incremente-a
      
    }
    
  }
  
  if(PIND & BIT4_MASK){//Se a interrupçao foi gerada por uma borda de SUBIDA no pino 2... (Se a borda de descida que gerou a interrupçao, este bit valera zero e nao sera executado).
    
    botao1 = tempoAtual;//Quando este botao for pressionado, armazene nele o momento em que o evento ocorreu.
    
    if( botao1 >= old_botao1 + threshold){//Debouncing
      old_botao1 = botao1;
      
      if(velocidade<20) velocidade++;//Se a velocidadenao esta no valor maximo, incremente-a
      
    }
    
  }
}

/**
 * Configuraçoes do Timer/Counter 2 para modo Fast PWM.
 * O pino que recebe o sinal do PWM, neste caso, eh o 22.
 */
void init_PWM(){

  DDRB |= BIT3_MASK; // pino digital 11 do Arduino (OC2A) configurado como saída
  
  // Timer/Counter 2 Control Register A
  TCCR2A = (1<<COM2A1)|(1<<COM2B1)|(1<<WGM21)|(1<<WGM20);//Fast PWM mode, clear OC2A on compare match.
  
  // Output Compare 2 Register A
  OCR2A = 0; // Controla o duty cycle da saida OC2A (pino 22 do atmega). Faixa de valores: 0 a 255
 
  // Timer/Counter 2 Control Register B
  TCCR2B = (1<<CS20); // Configura o prescaler para nos passar o clock da placa.
}

/**
 * Esta interrupçao ocorre sempre que o contador atinge o valor maximo da contagem, que,
 * na atual configuraçao, vale OCR2A.
 * Esta interrupçao sera chamada a cada 1ms, incrementando a variavel milisegundos e a 
 * variavel segundos (esta somente a cada 1000 chamadas).
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

void initIO(){//Faz a configuração dos botoes e saídas nos pinos que designamos
 
  //Definindo quais terminais são Escrita ou Leitura.
  DDRB = DDRB|(BIT5_MASK)|(BIT4_MASK)|(BIT3_MASK); // Os bits 5,4 e 3 (pinos 13, 12 e 11, respectivamente) sao de saida.
  DDRD = DDRD & ~(BIT2_MASK|BIT3_MASK|BIT4_MASK|BIT5_MASK);// 2 a 5 sao pinos sao de entrada para usarmos em interrupçes ou leituras. 
  
}

void initInterrupt(){//Configura as interupções que requisitamos aos pinos de leitura.
  
  EICRA = 0b11;//Configurra para borda de subida a interrupção INT0.
  EIMSK = 0b1;//Ativa a interrupção INT0.

  PCMSK2 = 0b11000;//Ativa interrupçao nos bits dos pinos 3 a 4 e desabilita-a nos demais DESTE registrador.
  PCICR = (BIT2_MASK);//Ativa as interrupçes dos pinos de bits setado no registrador PCMSK2, desativa nos demais PCMSK. 
}


void init_UART(){//Configura a UART como transmissora.
  UBRR0 = 103;//Tabela 20-7 do datasheet.
  UCSR0A &= ~(1<<U2X0);//Configura velocidade
  UCSR0B = (1<<TXEN0); //Deixamos a transmissão da UART habilitada.
  UCSR0C = ((0<<USBS0)|(1<<UCSZ01)|(1<<UCSZ00));//Modo assíncrono, sem paridade, com 1 stop bit e 8 bits de dados.
}

void UART_send(char data){//Transmite um caractere parao monitor serial via UART
  while(!(UCSR0A & (1<<UDRE0)));//Aguarda a UART estar desocupada.
  UDR0 = data;
}

void UART_String(char *msg){//Função para conveniência: Envia um array de caracteres para a UART/Monitor Serial
  while(*msg){//Quebra este laço ao fim do array string.
    UART_send(*msg);
    msg++;
  }
}

void doubleToAscii10(double d, char *s){//Recebe um número i de até seis dígitos (mais a vírgula) e o converte em array de char s.
	
  int i = 0;
  int aux = (int)(d*100);//Deslocamos as casas para retirar as vírgulas.
  
  for(i = 0; i<TAM_ARRAY-1; i++){
    
    if(i != 2){//A vírgula tem que ser colocada após duas casas decimais. Se nao estivermos nesta casa, a conversão ocorre normalmente.
      s[(TAM_ARRAY-1)-i-1] = aux%10 + '0';//Convertemos o número para caractere e o salvamos em seu devido lugar.
      aux /= 10;//Vamos para a próxima casa a ser convertida.
    }else{//Se é a casa da vírgula, insira-a.
      s[(TAM_ARRAY-1)-i-1] = '.';
    }
      
  }s[i]='\0';//Fechamos a string no último índice.
  
}
