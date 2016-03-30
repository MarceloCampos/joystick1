/*
 *  Controle via Joystick analógico com conector DB15 (GamePort - antigo)
 * 
 *  by Marcelo Campos
 *  Garoa Hacker Club
 * 
 *  Dependências: 
 *    - Incluir EEPROMAnything.h que deve estar nesta mesma pasta
 *
 * Ligações ao Joystick - padrão GamePort (conector DB15):
 *   Pino Arduino | Pino DB15   | Função
 *    A0          |      6      | eixo y - movimento do "ombro"
 *    A1          |      3      | eixo x - movimento da base
 *    A2          |      13     | "Hat" - controle no topo Manche ver tensões ativas nas variáveis h_toLowValue / h_toUpValue
 *    2           |      7      | Botão Setup (Nível Ativo = 0)
 *    3           |      2      | Botão Gatilho - Abre- fecha garra (Nível Ativo = 0)
 *    5V          |      1      | alimentação p/ o joystick de 5 volts
 *    Gnd         |      4      | Gnd
 * 
 *
 *  Notas: 
 *    1. Pode ser necessário modificar joystick p/ níveis ativo 0-Gnd e tensões do Hat
 *    2. A saída de 5 volts do Arduino (normalmente)não tem corrente suficiente para alimentar os servos do meArm, usar para eles alimentação externa de 5V
 *    
 *  Uso: movimentos de acordo com posição do joystick
 *    acionando (Nível Ativo = 0) botão Gatilho Garra fecha  
 *    acionando (Nível Ativo = 0) botão Setup entra em modo calibração valores do Joystick
 *
 *  REV.0 - Mar/16
 */

#include <EEPROM.h>
#include "EEPROMAnything.h" // arquivo deve estar na mesma pasta deste Sketch
#include <Servo.h>

Servo meArmServo[4];

int xAxis;
int yAxis;
int hAxis = 90;

int xAxisMin;
int xAxisMax;
int xAxisCent;
int yAxisMin;
int yAxisMax;
int yAxisCent;

int hAxisMaxValue = 132;  // valor máximo do eixo
int hAxisMinValue = 10;  // valor máximo do eixo

byte xPin = 0;
byte yPin = 1;
byte hPin = A2;   // "hat" do Joystick, usualmente fica na parte de cima do manche - controle altura braço

int h_toLowValue = 950;   // valor analog comando para baixo eixo h
int h_toUpValue = 630;    // valor analog comando para cima eixo h
int h_ToleranceValue = 70; // tolerância !

byte SetupButton_Pin = 2;
byte TrigerButton_Pin = 3;

volatile byte Flag_Setup_On = 0;
volatile byte Flag_Int_Svc = 0;

byte SM = 0;

byte Flag_Invert_xAxis = 0;
byte Flag_Invert_yAxis = 0;

byte GripOpen = 80;
byte GripClose = 100;


void trigerOn()
{
  detachInterrupt(digitalPinToInterrupt(SetupButton_Pin)); 
  Flag_Setup_On = 1; 
  Flag_Int_Svc = 0;
}

void setup() 
{
  
  EEPROM_readAnything(0, xAxisMin);
  EEPROM_readAnything(2, xAxisMax);
  EEPROM_readAnything(4, yAxisMin);
  EEPROM_readAnything(6, yAxisMax);
  EEPROM_readAnything(8, xAxisCent);  
  EEPROM_readAnything(0x0A, yAxisCent);

  // verifica se valores inválidos na
  if (xAxisMin < 0 || xAxisMax>1023 || yAxisMin < 0 || yAxisMax > 1023 )
  {
    resetAxisValues();
    Serial.println("Valores eixos resetados, sugerido calibrar...");    
  }

  meArmServo[0].attach(4);    // eixo x: base - ombro
  meArmServo[1].attach(5);    // eixo y: 
  meArmServo[2].attach(6); 
  meArmServo[3].attach(7); 
  meArmServo[0].write( 90 );
  meArmServo[1].write( 90 ); 
  meArmServo[2].write( 90 );
  meArmServo[3].write( GripOpen ); 
    
//  testSevosCenter();        // teste somente para verificar/ acertar centro servos braço, 
                              // comentar para desativar e operação normal        
  Serial.begin(9600);
  pinMode(SetupButton_Pin, INPUT_PULLUP);
  pinMode(TrigerButton_Pin, INPUT_PULLUP);
  pinMode(hPin, INPUT);
  
  delay(750);                 // espera
  
  attachInterrupt(digitalPinToInterrupt(SetupButton_Pin), trigerOn, FALLING);

  Serial.println("");
  Serial.println("");
  Serial.println("Init Ok");
  printMinMax();
  printCenter();  
}

void loop() 
{

  if (Flag_Setup_On)
  {
      if (!Flag_Int_Svc)  // executa só 1a vez
      {
        SM++;
        if (SM == 1)
        {
          Serial.println("");
          Serial.println("Mova o Joystick para posicoes Minimas e Maximas..."); 
        }
        if (SM == 2)
        {
          Serial.println("");
          Serial.println("Posicione o Joystick no centro e aperte novamente o Botao Triger..."); 
        }        

        
      }

      if (digitalRead(SetupButton_Pin) == 1) 
      {
        Flag_Setup_On = 0;
        attachInterrupt(digitalPinToInterrupt(SetupButton_Pin), trigerOn, FALLING);
      }

      Flag_Int_Svc = 1;
  }

  if (SM) // executa leitura se ativo captura setup valores
  {
    xAxis = analogRead(xPin);
    yAxis = analogRead(yPin); 
    delay(250); 
  }

  switch (SM)
  {
    case 0:
      joystickControl();  // joystick contronlado real time meArm
      break;
    case 1:
      compareMinMax();    // captura e seta pontos minimo e maximo eixos x y      
      break;
    case 2:
      captureCenter();    // captura e seta centro joystick      
      break;
    case 3:
      saveValues();       // ao final salva valores      
      break;      
  }
  
}


int lastTimeRead = 0;         // tempo última leitura canal analog. hat - controle hAxis
  
void joystickControl()    
{
  int analogValue;

  
  xAxis = analogRead(xPin);
  yAxis = analogRead(yPin);

  if ( Flag_Invert_yAxis )  // inverte eixo Y
    yAxis = yAxisMax - yAxis + yAxisMin ;
    
  if ( Flag_Invert_xAxis )  // inverte eixo  X
    xAxis = xAxisMax - xAxis + xAxisMin ;  

  if ( digitalRead(TrigerButton_Pin) == 0)  // verifica botão controle garra
  {
    meArmServo[3].write( GripClose ); 
  }
  else
  {
    meArmServo[3].write( GripOpen ); 
  }

    analogValue = analogRead(hPin);       // captura valor lido "hat"
    if ( analogValue > h_toLowValue && hAxis > hAxisMinValue && (millis() - lastTimeRead > 20))
    {
        hAxis--;
        lastTimeRead = millis();        // armazena tempo última leira pra não acionar mais que x vezes por segundo     
    }
    else if ( analogValue > (h_toUpValue-h_ToleranceValue) && analogValue < (h_toUpValue+h_ToleranceValue) && hAxis < hAxisMaxValue && (millis() - lastTimeRead > 20))
    {
        hAxis++;
        lastTimeRead = millis();        // armazena tempo última leira pra não acionar mais que x vezes por segundo
    }
    
  meArmServo[0].write( map (xAxis, xAxisMin, xAxisMax, 0, 180) );
  meArmServo[1].write( map (yAxis, yAxisMin, yAxisMax, 0, 180) ); 
  meArmServo[2].write( hAxis );    
}

void testSevosCenter ()   // teste somente para verificar/ acertar centro servos braço
{
  meArmServo[0].write(90);
  meArmServo[1].write(90);
  meArmServo[2].write(90);
  meArmServo[3].write(90); 
  Serial.println("Servos em modo teste..."); 
  while(1); // permanece aqui  
}

void saveValues() // salva valores na memória eeprom
{ 
  EEPROM_writeAnything(0, xAxisMin);
  EEPROM_writeAnything(2, xAxisMax);
  EEPROM_writeAnything(4, yAxisMin);
  EEPROM_writeAnything(6, yAxisMax);
  EEPROM_writeAnything(8, xAxisCent);  
  EEPROM_writeAnything(0x0A, yAxisCent);
    
  Serial.println("");  
  Serial.print("Valores salvos na memoria eeprom :");  
  Serial.println("");
  printMinMax();
  printCenter();
  Serial.println("");
  SM = 0; // reinicia máquina de estados
}

void captureCenter()
{
  yAxisCent = yAxis;
  xAxisCent = xAxis;  

  printCenter();

}

void compareMinMax()
{
  if (xAxis > xAxisMax)
    xAxisMax = xAxis;

  if (yAxis > yAxisMax)
    yAxisMax = yAxis;

  if (xAxis < xAxisMin) 
    xAxisMin = xAxis;

  if (yAxis < yAxisMin) 
    yAxisMin = yAxis; 

  printMinMax();

            
}

void printCenter()
{
  Serial.print("xCenter= ");
  Serial.print(xAxisCent);
  
  Serial.print(" | yCenter= ");
  Serial.print(yAxisCent);

  Serial.println("");  
}

void printMinMax () 
{
  Serial.print("xMin= ");
  Serial.print(xAxisMin);
  
  Serial.print(" | xMax= ");
  Serial.print(xAxisMax);

  Serial.print(" | yMin= ");
  Serial.print(yAxisMin);
  
  Serial.print(" | yMax= ");
  Serial.print(yAxisMax);
 
  Serial.println("");  
  
}

void resetAxisValues()
{
  xAxisMin=512;
  xAxisMax=512;
  xAxisCent=0;
  yAxisMin=512;
  yAxisMax=512;
  yAxisCent=0;  
  hAxis  = 90;
}

