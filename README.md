<div align="center">
  <img src="EmbarcaTech_logo_Azul-1030x428.png" alt="EmbarcaTech Logo" width="500"/>
</div>
# 🚦Semáforo Inteligente com Modo Noturno e Acessibilidade

## 📌 Descrição do Projeto

Este projeto tem como objetivo consolidar conhecimentos sobre tarefas no **FreeRTOS** por meio do desenvolvimento de um **sistema embarcado funcional e acessível**, utilizando a placa **BitDog Lab com RP2040**.

O sistema simula um **semáforo inteligente** com dois modos de operação:

- **Modo Normal:** funcionamento padrão do semáforo com sinalização sonora.
- **Modo Noturno:** apenas o LED amarelo piscando, também com sinalização sonora.

O sistema foi desenvolvido **utilizando apenas tarefas do FreeRTOS**, sem uso de filas, semáforos ou mutexes. Cada periférico do sistema é controlado por uma tarefa específica.

---

## 🧠 Funcionalidades

### ✅ Modo Normal
- **Ciclo do semáforo:** verde → amarelo → vermelho (SEMAFORO MOTORISTA).
- **Sinalização sonora (buzzer):**
  - Verde: 1 beep curto por segundo (indica que o pedestre pode atravessar).
  - Amarelo: beeps rápidos intermitentes (atenção).
  - Vermelho: tom contínuo curto (500ms ligado / 1.5s desligado) indicando "pare".
- **Display OLED:** mostra o modo atual e a cor ativa do semáforo.
- **Matriz de LEDs:** animação correspondente à cor do semáforo do pedestre .
  
### 🌙 Modo Noturno
- Apenas o LED **amarelo pisca lentamente**.
- **Buzzer:** beep curto a cada 2 segundos.
- **Display e Matriz de LEDs:** informam que o sistema está em modo noturno.

### 👆 Alternância de Modo
- O botão **A** alterna entre **Modo Normal** e **Modo Noturno**.
- A alternância é gerenciada por uma **flag global** modificada por uma tarefa dedicada.


## 🚦 Modos de Operação

### ✅ Modo Normal
- **Ciclo do semáforo (motorista):** verde → amarelo → vermelho.
- **Sinalização sonora (buzzer):**
  - **Verde:** 1 beep curto por segundo → indica que o pedestre pode atravessar.
  - **Amarelo:** beeps rápidos intermitentes → atenção.
  - **Vermelho:** tom contínuo curto (500ms ligado / 1.5s desligado) → pare.
- **Display OLED:** mostra o modo atual e a cor ativa do semáforo.
- **Matriz de LEDs:** representa a **cor do semáforo para pedestres** com animações.

### 🌙 Modo Noturno
- **Funcionamento:** apenas o LED amarelo pisca lentamente.
- **Sinalização sonora:** beep a cada 2 segundos.
- **Display OLED:** exibe "Modo Noturno".
- **Matriz de LEDs:** acompanha o estado de alerta do pedestre.

---

## 🛠️ Periféricos Utilizados

- **LEDs comuns** (RGB) para sinalização de cores do semáforo.
- **Matriz de LEDs WS2812** para representar o estado do semáforo de forma visual.
- **Display OLED** para exibir o estado e modo atual.
- **Buzzer** para sinalização sonora com acessibilidade.
- **Botão A** para troca de modo.

---

## 🔄 Estrutura de Tarefas (FreeRTOS)

O projeto utiliza **cinco tarefas**, cada uma com prioridade e função específica:


## ⚙️ Organização por Tarefas (FreeRTOS)

O projeto é dividido em 5 tarefas principais:

| Tarefa                      | Responsabilidade                                 |
|----------------------------|--------------------------------------------------|
| `TaskSemaforoMotorista`    | Controla as cores do semáforo de motoristas.    |
| `TaskBuzzerPedestre`       | Emite sinais sonoros conforme o estado atual.   |
| `TaskDisplaySemaforo`      | Atualiza o display OLED com informações.        |
| `TaskMatrizLedPedestre`    | Controla as animações da matriz de LEDs.        |
| `vTarefaBotaoA`            | Monitora o botão A e alterna o modo do sistema. |

```c
// Criação das tarefas com FreeRTOS
xTaskCreate(TaskSemaforoMotorista, "Semaforo", 256, NULL, 1, NULL);      // LEDs RGB
xTaskCreate(TaskBuzzerPedestre, "Buzzer", 256, NULL, 1, NULL);           // Buzzer
xTaskCreate(TaskDisplaySemaforo, "Display", 512, NULL, 1, NULL);         // OLED
xTaskCreate(TaskMatrizLedPedestre, "MatrizLED", 512, NULL, 4, NULL);     // WS2812
xTaskCreate(vTarefaBotaoA, "BOTAO A", 512, NULL, 1, NULL);               // Botão A

--
**Desenvolvido por:** Daniel Silva de Souza  
**Plataforma:** BitDog Lab + RP2040  
**CURSO:** Sistemas Embarcados  