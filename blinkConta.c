// Bibliotecas principais da Raspberry Pi Pico
#include "pico/stdlib.h"   // Funções básicas de entrada/saída e temporização
#include "hardware/gpio.h" // Controle dos pinos GPIO
#include "hardware/i2c.h"  // Comunicação via I2C

// Bibliotecas para uso com o display OLED
#include "lib/ssd1306.h" // Biblioteca para controlar o display SSD1306
#include "lib/font.h"    // Fonte para o display OLED

// Bibliotecas do FreeRTOS (sistema operacional de tempo real)
#include "FreeRTOS.h"       // Funções principais do FreeRTOS
#include "FreeRTOSConfig.h" // Configurações do FreeRTOS
#include "task.h"           // Gerenciamento de tarefas (threads)

// Biblioteca padrão para entrada/saída no terminal
#include <stdio.h> // Funções como printf()

// Biblioteca com desenhos personalizados para a matriz de led
#include "numeros.h"

// Controle de PWM para o buzzer
#include "hardware/pwm.h"

// MODO BOOTSEL (reset via botão B)
#include "pico/bootrom.h" // Biblioteca para reinicialização via USB

// Trecho para modo BOOTSEL com botão B (permite reiniciar o Pico no modo de boot USB)
#include "pico/bootrom.h"

// ----------------------------- DEFINIÇÕES DE PINOS -----------------------------
#define MATRIZ_LED_PIN 7 // Pino conectado à matriz de LEDs WS2812
#define I2C_PORT i2c1    // Porta I2C usada
#define I2C_SDA 14       // Pino SDA da comunicação I2C
#define I2C_SCL 15       // Pino SCL da comunicação I2C
#define endereco 0x3C    // Endereço I2C do display OLED

#define PIN_LED_GREEN 11 // LED Verde do semáforo
#define PIN_LED_BLUE 12  // LED Azul (usado para AMARELO)
#define PIN_LED_RED 13   // LED Vermelho do semáforo
#define BUZZER_PIN 10    // Pino do buzzer

#define ATRASO_MS 28 // Ajuste para sincronização fina entre tarefas
#define BOTAO_A 5    // Pino do botão A para alternar modos

int contador = 0; // Variável para contagem geral (não utilizada no código atual)

#define botaoB 6
// ----------------------------- MODO BOOTSEL (BOTÃO B) --------------------------

// ---------------------- TAREFA: Semáforo do motorista --------------------------

// ----------------------------- VARIÁVEIS GLOBAIS -------------------------------
volatile int modo_noturno = 0; // Flag para controlar o modo noturno (0 = normal, 1 = noturno)

// Enumeração para os estados do semáforo
typedef enum
{
    COR_VERDE,    // Sinal verde para carros
    COR_AMARELO,  // Sinal amarelo para carros
    COR_VERMELHO, // Sinal vermelho para carros
    COR_NOTURNO   // Modo noturno (piscando amarelo)
} EstadoCor;

volatile EstadoCor estado_cor_atual = COR_VERDE; // Estado inicial do semáforo

// Handler de interrupção para o botão B
void gpio_irq_handler(uint gpio, uint32_t events)
{
    reset_usb_boot(0, 0); // Reinicia o Pico no modo BOOTSEL
}

// ----------------------------- TAREFA: BOTÃO A --------------------------------
// Tarefa para monitorar o botão A e alternar entre modos normal/noturno
void vTarefaBotaoA(void *pvParameters)
{
    gpio_init(BOTAO_A);
    gpio_set_dir(BOTAO_A, GPIO_IN);
    gpio_pull_up(BOTAO_A); // Habilita resistor de pull-up

    int estado_anterior = 1; // Estado anterior do botão (inicialmente solto)

    while (1)
    {
        int estado_atual = gpio_get(BOTAO_A);

        // Detecta borda de descida (botão pressionado)
        if (estado_atual == 0 && estado_anterior == 1)
        {
            modo_noturno = !modo_noturno;   // Alterna o modo
            vTaskDelay(pdMS_TO_TICKS(300)); // Debounce reforçado
        }

        estado_anterior = estado_atual;
        vTaskDelay(pdMS_TO_TICKS(50)); // Verificação mais rápida
    }
}

// Tarefa principal do semáforo para motoristas-----------------------------------
void TaskSemaforoMotorista()
{
    // Inicializa os pinos dos LEDs
    gpio_init(PIN_LED_BLUE);
    gpio_init(PIN_LED_GREEN);
    gpio_init(PIN_LED_RED);
    gpio_set_dir(PIN_LED_BLUE, GPIO_OUT);
    gpio_set_dir(PIN_LED_GREEN, GPIO_OUT);
    gpio_set_dir(PIN_LED_RED, GPIO_OUT);

    while (true)
    {
        if (modo_noturno)
        {
            // Modo Noturno: Piscar amarelo (LED azul)
            gpio_put(PIN_LED_GREEN, 1);
            gpio_put(PIN_LED_RED, 1);
            gpio_put(PIN_LED_BLUE, 0);

            // Pisca o LED amarelo (azul) com intervalo de 1s
            for (int i = 0; i < 10; i++)
            {
                if (!modo_noturno)
                { // Sai se o modo mudar
                    estado_cor_atual = COR_VERDE;
                    break;
                }
                vTaskDelay(pdMS_TO_TICKS(100));
            }

            gpio_put(PIN_LED_BLUE, 0);
            gpio_put(PIN_LED_RED, 0);
            gpio_put(PIN_LED_GREEN, 0);

            for (int i = 0; i < 10; i++)
            {
                if (!modo_noturno)
                {
                    estado_cor_atual = COR_VERDE;
                    break;
                }
                vTaskDelay(pdMS_TO_TICKS(100));
            }
        }
        else
        {
            // Modo Normal: Ciclo normal do semáforo

            if (estado_cor_atual == COR_AMARELO)
            {
                // Estado AMARELO (4 segundos)
                gpio_put(PIN_LED_GREEN, 1);
                gpio_put(PIN_LED_BLUE, 0);
                gpio_put(PIN_LED_RED, 1);

                for (int i = 0; i < 40; i++)
                {
                    if (modo_noturno)
                        break; // Sai se entrar em modo noturno
                    vTaskDelay(pdMS_TO_TICKS(100));
                }

                estado_cor_atual = COR_VERMELHO;
            }

            if (estado_cor_atual == COR_VERMELHO)
            {
                // Estado VERMELHO (10 segundos)
                gpio_put(PIN_LED_BLUE, 0);
                gpio_put(PIN_LED_RED, 1);
                gpio_put(PIN_LED_GREEN, 0);

                for (int i = 0; i < 100; i++)
                {
                    if (modo_noturno)
                        break;
                    vTaskDelay(pdMS_TO_TICKS(100));
                }

                estado_cor_atual = COR_VERDE;
            }

            if (estado_cor_atual == COR_VERDE)
            {
                // Estado VERDE (10 segundos)

                gpio_put(PIN_LED_GREEN, 1);
                gpio_put(PIN_LED_RED, 0);
                gpio_put(PIN_LED_BLUE, 0);

                for (int i = 0; i < 100; i++)
                {
                    if (modo_noturno)
                        break;
                    vTaskDelay(pdMS_TO_TICKS(100));
                }

                estado_cor_atual = COR_AMARELO;
            }
        }
    }
}

// ----------------------------- TAREFA: Buzzer sonoro -----------------------------
// Função para iniciar o PWM do buzzer com frequência específica
void iniciar_pwm_buzzer(uint freq_hz)
{
    gpio_set_function(BUZZER_PIN, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(BUZZER_PIN);
    uint clock = 125000000; // Clock padrão de 125 MHz
    uint divider = 4;       // Divisor de clock
    uint top = clock / (divider * freq_hz);

    pwm_set_clkdiv(slice, divider);
    pwm_set_wrap(slice, top);
    pwm_set_chan_level(slice, pwm_gpio_to_channel(BUZZER_PIN), top / 2); // Duty cycle de 50%
    pwm_set_enabled(slice, true);
}

// Função para parar o PWM do buzzer
void parar_pwm_buzzer()
{
    uint slice = pwm_gpio_to_slice_num(BUZZER_PIN);
    pwm_set_enabled(slice, false);
}

// Tarefa do buzzer para pedestres
void TaskBuzzerPedestre()
{
    while (true)
    {
        if (modo_noturno)
        {
            // Modo Noturno: beep lento a cada 2s
            iniciar_pwm_buzzer(500); // Beep de 500Hz
            vTaskDelay(pdMS_TO_TICKS(1000));
            parar_pwm_buzzer();

            for (int i = 1; i <= 20; i++)
            {
                if (!modo_noturno)
                    i = 999; // Sai se mudar de modo
                vTaskDelay(pdMS_TO_TICKS(100));
            }
        }
        else
        {
            // Modo Normal - Sincronizado com o semáforo
            switch (estado_cor_atual)
            {
            case COR_VERDE:
                // Verde: 1 beep curto por segundo (10 beeps em 10s)
                for (int i = 0; i < 10; i++)
                {
                    iniciar_pwm_buzzer(500);
                    vTaskDelay(pdMS_TO_TICKS(100));
                    parar_pwm_buzzer();

                    for (int j = 1; j <= 9; j++)
                    {
                        if (modo_noturno || (estado_cor_atual != COR_VERDE))
                        {
                            j = 999;
                            i = 999;
                        }
                        vTaskDelay(pdMS_TO_TICKS(100));
                    }
                }
                break;

            case COR_AMARELO:
                // Amarelo: beep rápido intermitente (20 beeps em 4s)
                for (int i = 0; i < 20; i++)
                {
                    iniciar_pwm_buzzer(500);
                    vTaskDelay(pdMS_TO_TICKS(100));
                    parar_pwm_buzzer();

                    for (int j = 0; j <= 0; j++)
                    {
                        if (modo_noturno || (estado_cor_atual != COR_AMARELO))
                        {
                            j = 999;
                            i = 999;
                        }
                        vTaskDelay(pdMS_TO_TICKS(100));
                    }
                }
                break;

            case COR_VERMELHO:
                // Vermelho: tom contínuo curto (500ms ligado, 1.5s desligado)
                for (int i = 0; i < 5; i++)
                {
                    iniciar_pwm_buzzer(500);
                    vTaskDelay(pdMS_TO_TICKS(500));
                    parar_pwm_buzzer();

                    for (int j = 1; j <= 15; j++)
                    {
                        if (modo_noturno || (estado_cor_atual != COR_VERMELHO))
                        {
                            j = 999;
                            i = 999;
                        }
                        vTaskDelay(pdMS_TO_TICKS(100));
                    }
                }
                break;

            default:
                break;
            }
        }
    }
}

// ----------------------------- TAREFA: Display OLED -----------------------------
void TaskDisplaySemaforo()
{
    // Inicializa I2C
    i2c_init(I2C_PORT, 400 * 1000); // 400 kHz
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    // Inicializa o display OLED
    ssd1306_t ssd;
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT);
    ssd1306_config(&ssd);
    ssd1306_send_data(&ssd);
    ssd1306_fill(&ssd, false); // Limpa o display
    ssd1306_send_data(&ssd);

    while (true)
    {
        if (modo_noturno)
        {
            // Modo Noturno: Exibe mensagem no display

            ssd1306_fill(&ssd, false);
            ssd1306_draw_string(&ssd, "MODO NOTURNO", 15, 20);
            ssd1306_rect(&ssd, 3, 3, 123, 60, true, false); // Moldura
            ssd1306_send_data(&ssd);

            for (int i = 0; i < 5; i++)
            {
                if (!modo_noturno)
                {
                    i = 999;
                }

                vTaskDelay(pdMS_TO_TICKS(100));
            }
        }

        if (!modo_noturno && (estado_cor_atual == COR_AMARELO))
        {
            // Estado AMARELO no display

            ssd1306_fill(&ssd, false);

            // Desenha o semáforo
            ssd1306_rect(&ssd, 3, 3, 123, 60, true, false); // Moldura

            // LEDs do semáforo (apenas amarelo aceso)
            ssd1306_rect(&ssd, 10, 10, 8, 8, true, false); // Vermelho (apagado)
            ssd1306_rect(&ssd, 10, 20, 8, 8, true, true);  // Amarelo (aceso)
            ssd1306_rect(&ssd, 10, 30, 8, 8, true, false); // Verde (apagado)

            // Textos informativos
            ssd1306_draw_string(&ssd, "ATENCAO", 45, 10);
            ssd1306_draw_string(&ssd, "MOTORISTA", 10, 22);
            ssd1306_line(&ssd, 3, 35, 123, 35, 1); // Linha divisória
            ssd1306_draw_string(&ssd, "  PEDESTRE", 10, 40);
            ssd1306_draw_string(&ssd, "   ESPERE", 10, 50);
            ssd1306_send_data(&ssd);

            for (int i = 1; i <= 40; i++)
            {
                if (modo_noturno || (estado_cor_atual == !COR_AMARELO))
                {
                    i = 999;
                }

                if (i == 40)
                {
                    vTaskDelay(pdMS_TO_TICKS(100 - ATRASO_MS));
                }
                else
                {
                    vTaskDelay(pdMS_TO_TICKS(100));
                }
            }
        }

        if (!modo_noturno && (estado_cor_atual == COR_VERMELHO))
        {
            // Estado VERMELHO no display

            ssd1306_fill(&ssd, false);
            ssd1306_rect(&ssd, 3, 3, 123, 60, true, false); // Moldura

            // LEDs do semáforo (apenas vermelho aceso)
            ssd1306_rect(&ssd, 10, 10, 8, 8, true, true);  // Vermelho (aceso)
            ssd1306_rect(&ssd, 10, 20, 8, 8, true, false); // Amarelo (apagado)
            ssd1306_rect(&ssd, 10, 30, 8, 8, true, false); // Verde (apagado)

            // Textos informativos
            ssd1306_draw_string(&ssd, " PARE", 45, 10);
            ssd1306_draw_string(&ssd, "MOTORISTA", 10, 22);
            ssd1306_line(&ssd, 3, 35, 123, 35, 1); // Linha divisória
            ssd1306_draw_string(&ssd, "  PEDESTRE", 10, 40);
            ssd1306_draw_string(&ssd, "  ATRAVESE", 10, 50);
            ssd1306_send_data(&ssd);

            for (int i = 1; i <= 100; i++)
            {
                if (modo_noturno || (estado_cor_atual == !COR_VERMELHO))
                {
                    i = 999;
                }

                if (i == 100)
                {
                    vTaskDelay(pdMS_TO_TICKS(100 - ATRASO_MS));
                }
                else
                {
                    vTaskDelay(pdMS_TO_TICKS(100));
                }
            }
        }

        if (!modo_noturno && (estado_cor_atual == COR_VERDE))
        {
            // Estado VERDE no display
            ssd1306_fill(&ssd, false);
            ssd1306_rect(&ssd, 3, 3, 123, 60, true, false); // Moldura

            // LEDs do semáforo (apenas verde aceso)
            ssd1306_rect(&ssd, 10, 10, 8, 8, true, false); // Vermelho (apagado)
            ssd1306_rect(&ssd, 10, 20, 8, 8, true, false); // Amarelo (apagado)
            ssd1306_rect(&ssd, 10, 30, 8, 8, true, true);  // Verde (aceso)

            // Textos informativos
            ssd1306_draw_string(&ssd, " SIGA", 45, 10);
            ssd1306_draw_string(&ssd, "MOTORISTA", 10, 22);
            ssd1306_line(&ssd, 3, 35, 123, 35, 1); // Linha divisória
            ssd1306_draw_string(&ssd, "  PEDESTRE", 10, 40);
            ssd1306_draw_string(&ssd, "   ESPERE", 10, 50);
            ssd1306_send_data(&ssd);

            for (int i = 1; i <= 100; i++)
            {
                if (modo_noturno  || (estado_cor_atual == !COR_VERDE))
                    i = 999;
                if (i == 100)
                {
                    vTaskDelay(pdMS_TO_TICKS(100 - ATRASO_MS));
                }
                else
                {
                    vTaskDelay(pdMS_TO_TICKS(100));
                }
            }
        }
    }
}

// ------------------------ TAREFA: Matriz de LED do pedestre ----------------------
void TaskMatrizLedPedestre(void *params)
{
    while (true)
    {
        if (modo_noturno)
        {
            // Modo Noturno: Piscar matriz de LEDs em amarelo
            Amarelo_Noturno(); // Função externa para acionar LEDs amarelos

            for (int i = 1; i <= 10; i++)
            {
                if (!modo_noturno)
                    i = 999;
                vTaskDelay(pdMS_TO_TICKS(100));
            }

            DesligaMatriz(); // Função externa para desligar LEDs

            for (int i = 1; i <= 10; i++)
            {
                if (!modo_noturno)
                    i = 999;
                vTaskDelay(pdMS_TO_TICKS(100));
            }
        }
        else
        {
            // Modo Normal: Sincronizado com o semáforo
            if (estado_cor_atual == COR_AMARELO)
            {
                // Estado AMARELO: Pedestre deve parar
                PedrestePARE(); // Função externa para mostrar "PARE"

                for (int i = 1; i <= 40; i++)
                {
                    if (modo_noturno || (estado_cor_atual != COR_AMARELO))
                        i = 999;
                    vTaskDelay(pdMS_TO_TICKS(100));
                }
            }

            if (estado_cor_atual == COR_VERMELHO)
            {
                // Estado VERMELHO: Pedestre pode atravessar
                PedresteSIGA(); // Função externa para mostrar "SIGA"

                for (int i = 1; i <= 100; i++)
                {
                    if (modo_noturno || (estado_cor_atual != COR_VERMELHO))
                        i = 999;
                    vTaskDelay(pdMS_TO_TICKS(100));
                }
            }

            if (estado_cor_atual == COR_VERDE)
            {
                // Estado VERDE: Pedestre deve esperar
                PedrestePARE();

                for (int i = 1; i <= 100; i++)
                {
                    if (modo_noturno || (estado_cor_atual != COR_VERDE))
                        i = 999;
                    vTaskDelay(pdMS_TO_TICKS(100));
                }
            }
        }
    }
}

// --------------------------- FUNÇÃO PRINCIPAL (main) ----------------------------
int main()
{
    stdio_init_all();       // Inicializa entrada/saída padrão
    npInit(MATRIZ_LED_PIN); // Inicializa a matriz de LEDs WS2812

    // Configuração do botão B para modo BOOTSEL
    gpio_init(botaoB);
    gpio_set_dir(botaoB, GPIO_IN);
    gpio_pull_up(botaoB);
    gpio_set_irq_enabled_with_callback(botaoB, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

    // Criação das tarefas com FreeRTOS
    xTaskCreate(TaskSemaforoMotorista, "Semaforo", 256, NULL, 1, NULL);
    xTaskCreate(TaskBuzzerPedestre, "Buzzer", 256, NULL, 1, NULL);
    xTaskCreate(TaskDisplaySemaforo, "Display", 512, NULL, 1, NULL);
    xTaskCreate(TaskMatrizLedPedestre, "MatrizLED", 512, NULL, 4, NULL);
    xTaskCreate(vTarefaBotaoA, "BOTAO A", 512, NULL, 1, NULL);

    vTaskStartScheduler(); // Inicia o escalonador do FreeRTOS

    // Nunca deve chegar aqui
    while (1)
        ;
}