#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"
#include "lwip/tcp.h"
#include <string.h>
#include <stdio.h>
#include "inc/ssd1306.h"
#include "inc/font.h"
#include "hardware/i2c.h"
#include "hardware/clocks.h"
#include "hardware/pio.h"
#include "Display.pio.h"
#include "hardware/adc.h"

// Definições WiFi
#define WIFI_SSID "Paulo"
#define WIFI_PASS "7582803807"

// Definições de hardware
#define MAX_LEDS 4
#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define endereco 0x3C

// Pinos dos LEDs
const uint8_t LED_PINS[MAX_LEDS] = {11, 12, 13, 0};
#define LED_G 11
#define LED_B 12
#define LED_R 13

// Estados do sistema
typedef enum {
    WIFI_CONNECTING,
    WIFI_CONNECTED,
    WIFI_ERROR,
    SYSTEM_READY
} system_state_t;

// Estrutura para status do sistema
typedef struct {
    system_state_t state;
    char ip_address[16];
    bool led_states[3];
    bool rgb_state;
    float temperature;  // Nova variável para armazenar a temperatura
} system_status_t;

// Variáveis globais
ssd1306_t ssd;
system_status_t system_status;
bool cor = true;

// HTML Response
#define HTTP_RESPONSE_TEMPLATE "HTTP/1.1 200 OK\r\n" \
                      "Content-Type: text/html\r\n" \
                      "Cache-Control: no-cache, no-store, must-revalidate\r\n" \
                      "Pragma: no-cache\r\n" \
                      "Expires: 0\r\n" \
                      "Connection: close\r\n" \
                      "\r\n" \
                      "<!DOCTYPE html>" \
                      "<html lang='pt-BR'>" \
                      "<head>" \
                      "<meta charset='UTF-8'>" \
                      "<meta name='viewport' content='width=device-width, initial-scale=1.0'>" \
                      "<title>Painel de Controle</title>" \
                      "<style>" \
                      "* { margin: 0; padding: 0; box-sizing: border-box; }" \
                      "body {" \
                      "  font-family: 'Arial', sans-serif;" \
                      "  background: linear-gradient(to right, #1e3c72, #2a5298);" \
                      "  min-height: 100vh;" \
                      "  display: flex;" \
                      "  justify-content: center;" \
                      "  align-items: center;" \
                      "  color: white;" \
                      "}" \
                      ".container {" \
                      "  width: 80%;" \
                      "  max-width: 500px;" \
                      "  background: rgba(255, 255, 255, 0.1);" \
                      "  padding: 20px;" \
                      "  border-radius: 15px;" \
                      "  box-shadow: 0 4px 10px rgba(0,0,0,0.3);" \
                      "  text-align: center;" \
                      "}" \
                      "h1 { font-size: 24px; margin-bottom: 20px; }" \
                      ".temperature { font-size: 22px; margin-bottom: 20px; }" \
                      ".led-control { display: grid; gap: 15px; }" \
                      ".led-group {" \
                      "  display: flex;" \
                      "  justify-content: space-between;" \
                      "  background: rgba(0, 0, 0, 0.2);" \
                      "  padding: 10px;" \
                      "  border-radius: 10px;" \
                      "}" \
                      ".btn {" \
                      "  padding: 10px 20px;" \
                      "  border-radius: 8px;" \
                      "  border: none;" \
                      "  cursor: pointer;" \
                      "  font-weight: bold;" \
                      "  transition: 0.2s;" \
                      "}" \
                      ".btn:hover { transform: scale(1.05); }" \
                      ".btn-on { background: #27ae60; color: white; }" \
                      ".btn-off { background: #c0392b; color: white; }" \
                      "</style>" \
                      "</head>" \
                      "<body>" \
                      "<div class='container'>" \
                      "<h1>Painel de Controle</h1>" \
                      "<div class='temperature'>Temperatura Atual: %.1f °C</div>" \
                      "<div class='led-control'>" \
                      "<div class='led-group'>" \
                      "<span>LED 1</span>" \
                      "<button class='btn btn-on' onclick=\"location.href='/led/0/on'\">Ligar</button>" \
                      "<button class='btn btn-off' onclick=\"location.href='/led/0/off'\">Desligar</button>" \
                      "</div>" \
                      "<div class='led-group'>" \
                      "<span>LED 2</span>" \
                      "<button class='btn btn-on' onclick=\"location.href='/led/1/on'\">Ligar</button>" \
                      "<button class='btn btn-off' onclick=\"location.href='/led/1/off'\">Desligar</button>" \
                      "</div>" \
                      "<div class='led-group'>" \
                      "<span>LED 3</span>" \
                      "<button class='btn btn-on' onclick=\"location.href='/led/2/on'\">Ligar</button>" \
                      "<button class='btn btn-off' onclick=\"location.href='/led/2/off'\">Desligar</button>" \
                      "</div>" \
                      "<div class='led-group'>" \
                      "<span>LED RGB</span>" \
                      "<button class='btn btn-on' onclick=\"location.href='/led/3/on'\">Ligar</button>" \
                      "<button class='btn btn-off' onclick=\"location.href='/led/3/off'\">Desligar</button>" \
                      "</div>" \
                      "</div>" \
                      "</div>" \
                      "</body>" \
                      "</html>"
// Função para ler a temperatura do sensor interno
float read_temperature() {
    // O sensor de temperatura está conectado ao ADC4 (canal 4)
    adc_set_temp_sensor_enabled(true);
    adc_select_input(4);
    
    // Lê o valor do ADC
    uint16_t raw = adc_read();
    
    // Converte para voltagem
    const float conversion_factor = 3.3f / (1 << 12);
    float voltage = raw * conversion_factor;
    
    // Converte para temperatura (usando a fórmula do datasheet)
    float temperature = 27.0f - (voltage - 0.706f) / 0.001721f;
    
    return temperature;
}

// Funções do Display
void display_system_status(void) {
    ssd1306_fill(&ssd, false);

    // Status do sistema
    char status_msg[32];
    switch(system_status.state) {
        case WIFI_CONNECTING:
            strcpy(status_msg, "WiFi: Conectando...");
            break;
        case WIFI_CONNECTED:
            strcpy(status_msg, "WiFi: Conectado");
            break;
        case WIFI_ERROR:
            strcpy(status_msg, "WiFi: Erro!");
            break;
        case SYSTEM_READY:
            strcpy(status_msg, "Sistema Pronto");
            break;
    }
    ssd1306_draw_string(&ssd, status_msg, 0, 0);

    // IP Address
    if (system_status.state == WIFI_CONNECTED || system_status.state == SYSTEM_READY) {
        char ip_msg[32];
        snprintf(ip_msg, sizeof(ip_msg), "IP: %s", system_status.ip_address);
        ssd1306_draw_string(&ssd, ip_msg, 0, 16);
    }

    // Combinação de Temperatura e LEDs na mesma linha
    char temp_led_msg[32];
    snprintf(temp_led_msg, sizeof(temp_led_msg), "Temp:%.fC LEDs:", system_status.temperature);
    ssd1306_draw_string(&ssd, temp_led_msg, 0, 32);
    
    // Status dos LEDs
    for (int i = 0; i < 3; i++) {
        char led_status[16];
        snprintf(led_status, sizeof(led_status), "%d:%s ", 
                i + 1, system_status.led_states[i] ? "ON" : "OF");
        ssd1306_draw_string(&ssd, led_status, i * 40 + 5, 48);
    }

    ssd1306_send_data(&ssd);
}

void update_system_status(system_state_t new_state) {
    system_status.state = new_state;
    display_system_status();
}

void update_ip_address(uint8_t *ip) {
    snprintf(system_status.ip_address, sizeof(system_status.ip_address),
             "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
    display_system_status();
}

void update_led_status(int led_index, bool state) {
    if (led_index >= 0 && led_index < 3) {
        system_status.led_states[led_index] = state;
    } else if (led_index == 3) {
        system_status.rgb_state = state;
    }
    display_system_status();
}

void update_temperature() {
    system_status.temperature = read_temperature();
    display_system_status();
}

void init_display_system(void) {
    // Inicializa I2C
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    // Inicializa display
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT);
    ssd1306_config(&ssd);
    
    // Inicializa estrutura de status
    memset(&system_status, 0, sizeof(system_status));
    system_status.state = WIFI_CONNECTING;
    
    // Mostra status inicial
    display_system_status();
}

// Controle do LED RGB
void control_rgb_led(bool state) {
    gpio_put(LED_R, state);
    gpio_put(LED_G, state);
    gpio_put(LED_B, state);
    printf("LED RGB %s\n", state ? "LIGADO (Branco)" : "DESLIGADO");
}

// Callbacks TCP/HTTP
static err_t http_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    if (p == NULL) {
        tcp_close(tpcb);
        return ERR_OK;
    }

    char *request = (char *)p->payload;

    for (int i = 0; i < MAX_LEDS; i++) {
        char on_pattern[20], off_pattern[20];
        snprintf(on_pattern, sizeof(on_pattern), "/led/%d/on", i);
        snprintf(off_pattern, sizeof(off_pattern), "/led/%d/off", i);

        if (strstr(request, on_pattern)) {
            if (i == 3) {
                control_rgb_led(true);
                update_led_status(3, true);
            } else {
                gpio_put(LED_PINS[i], 1);
                update_led_status(i, true);
            }
            break;
        } else if (strstr(request, off_pattern)) {
            if (i == 3) {
                control_rgb_led(false);
                update_led_status(3, false);
            } else {
                gpio_put(LED_PINS[i], 0);
                update_led_status(i, false);
            }
            break;
        }
    }

    // Atualiza a temperatura antes de enviar a resposta
    update_temperature();
    
    // Formata a resposta HTTP com a temperatura atual
    char http_response[4096]; // Tamanho suficiente para o HTML completo
    snprintf(http_response, sizeof(http_response), HTTP_RESPONSE_TEMPLATE, system_status.temperature);
    
    tcp_write(tpcb, http_response, strlen(http_response), TCP_WRITE_FLAG_COPY);
    pbuf_free(p);
    return ERR_OK;
}

static err_t connection_callback(void *arg, struct tcp_pcb *newpcb, err_t err) {
    tcp_recv(newpcb, http_callback);
    return ERR_OK;
}

void start_http_server(void) {
    struct tcp_pcb *pcb = tcp_new();
    if (!pcb) {
        printf("Erro ao criar PCB\n");
        return;
    }

    if (tcp_bind(pcb, IP_ADDR_ANY, 80) != ERR_OK) {
        printf("Erro ao ligar o servidor na porta 80\n");
        return;
    }

    pcb = tcp_listen(pcb);
    tcp_accept(pcb, connection_callback);
    printf("Servidor HTTP rodando na porta 80\n");
}

int main() {
    stdio_init_all();
    
    // Inicializa ADC para leitura de temperatura
    adc_init();
    adc_set_temp_sensor_enabled(true);
    
    // Inicializa o sistema de display
    init_display_system();
    
    printf("\nIniciando servidor HTTP\n");

    // Inicializa LEDs
    for (int i = 0; i < MAX_LEDS; i++) {
        gpio_init(LED_PINS[i]);
        gpio_set_dir(LED_PINS[i], GPIO_OUT);
        gpio_put(LED_PINS[i], 0);
        printf("LED %d inicializado no pino %d\n", i + 1, LED_PINS[i]);
    }

    // Inicializa WiFi
    if (cyw43_arch_init()) {
        printf("Erro ao inicializar o Wi-Fi\n");
        update_system_status(WIFI_ERROR);
        return 1;
    }

    cyw43_arch_enable_sta_mode();
    printf("Conectando ao Wi-Fi...\n");

    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASS, 
        CYW43_AUTH_WPA2_AES_PSK, 10000)) {
        printf("Falha ao conectar ao Wi-Fi\n");
        update_system_status(WIFI_ERROR);
        return 1;
    }

    // Atualiza status com IP
    uint8_t *ip = (uint8_t *)&(cyw43_state.netif[0].ip_addr.addr);
    update_system_status(WIFI_CONNECTED);
    update_ip_address(ip);
    
    printf("Conectado ao Wi-Fi!\n");
    printf("Endereço IP: %d.%d.%d.%d\n", ip[0], ip[1], ip[2], ip[3]);
    printf("Para controlar os LEDs, acesse: http://%d.%d.%d.%d\n",
           ip[0], ip[1], ip[2], ip[3]);

    // Faz uma leitura inicial da temperatura
    update_temperature();
    
    // Inicia servidor HTTP
    start_http_server();
    update_system_status(SYSTEM_READY);

    // Variáveis para controle de atualização de temperatura
    uint32_t last_temp_update = 0;
    const uint32_t temp_update_interval = 5000; // Atualiza temperatura a cada 5 segundos
    
    // Loop principal
    while (true) {
        cyw43_arch_poll();
        
        // Verifica se é hora de atualizar a temperatura
        uint32_t current_time = to_ms_since_boot(get_absolute_time());
        if (current_time - last_temp_update >= temp_update_interval) {
            update_temperature();
            last_temp_update = current_time;
        }
        
        sleep_ms(100);
    }

    return 0;
}