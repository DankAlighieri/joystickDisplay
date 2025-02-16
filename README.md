# Tarefa Joystick Display com PWM

## 🛠️ Componentes Utilizados

- Raspberry Pi Pico W
- Kit Educacional BitDogLab

## 💻 Firmware

Desenvolvido em C utilizando:

- IDE: Visual Studio Code
- SDK: Raspberry Pi Pico SDK
- JOY_X: GP26
- JOY_Y: GP27
- JOY_BTN: GP22
- BTN_A: GP5
- LED_R_PIN: GP13
- LED_G_PIN: GP11
- LED_B_PIN: GP12
- DISPLAY_SDA: GP14
- DISPLAY_SCL: GP15

## 🎯 Funcionamento

### Fluxo de Operação

- **Tempo 0s (Início):**  
  1. O display OLED é inicializado e um quadrado de 8x8 pixels é desenhado no centro.  
  2. Temporizador inicial é criado para ler os valores do joystick e atualizar a posição do quadrado a cada 10ms.  

- **Movimento do Joystick:**  
  1. O quadrado se move proporcionalmente aos valores capturados pelo joystick.  
  2. LEDs RGB mudam de intensidade conforme o movimento do joystick.  

- **Pressionamento do Botão do Joystick:**  
  1. Alterna entre diferentes tipos de bordas no display (borda simples, borda tracejada, borda dupla).  
  2. O LED verde alterna seu estado (ligado/desligado).  

- **Pressionamento do Botão A:**  
  1. Alterna o estado dos PWMs dos LEDs vermelho e azul (ligado/desligado).  

## 🤓 Explicação

O código utiliza um temporizador para ler os valores do joystick e atualizar a posição do quadrado no display OLED. O botão do joystick alterna entre diferentes tipos de bordas no display e o botão A alterna o estado dos LEDs RGB. A intensidade dos LEDs vermelho e azul é controlada por PWM, que varia conforme o movimento do joystick.

### 🎥 Vídeo

[Vídeo demonstrativo do projeto](https://drive.google.com/drive/folders/1vdK1bU-681Yz_SrNDz7V_xnN6kx3wqPs?usp=sharing)

## Como executar

O Projeto pode ser executado clonando este repositório e importando como projeto pico no Visual Studio Code e enviando o projeto para a placa BitDogLab.

## 👥 Autoria

**Guilherme Emetério Santos Lima**  
[![GitHub](https://img.shields.io/badge/GitHub-Profile-blue?style=flat&logo=github)](https://github.com/DankAlighieri)
