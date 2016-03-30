# Controle para meArm via Joystick Gameport (conec DB15) "joystick1"
(meArm - robotic arm - control using Joystick Gameport(old standard conec DB15) )

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
