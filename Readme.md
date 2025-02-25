# 🚀 Projeto: Servidor Web com Raspberry Pi Pico W

Este projeto implementa um **servidor web** utilizando o **Raspberry Pi Pico W**, permitindo o **controle de LEDs** 💡 e a **exibição da temperatura** 🌡️ lida do sensor interno via um **display OLED SSD1306**.

## 🛠️ Funcionalidades
✅ **Servidor HTTP** que permite o controle dos LEDs via interface web.  
✅ **Monitoramento de temperatura** utilizando o sensor interno do RP2040.  
✅ **Exibição de status** no display OLED SSD1306 via protocolo I2C.  
✅ **Conexão Wi-Fi** para acesso remoto à interface de controle.  

## 🔌 Hardware Utilizado
- 🖥️ **Raspberry Pi Pico W**
- 🖼️ **Display OLED SSD1306 (128x64)**
- 💡 **LEDs individuais e RGB**
- 🔧 **Resistores adequados**
- 🔌 **Fonte de alimentação USB**

## 📶 Configuração do Wi-Fi
No código, altere as credenciais do Wi-Fi para conectar o Raspberry Pi Pico W:
```c
#define WIFI_SSID "Seu_SSID"
#define WIFI_PASS "Sua_Senha"
```

## ⚡ Conexões
### 📟 Display OLED SSD1306 (I2C)
- **SDA** -> Pino 14 (GP14)
- **SCL** -> Pino 15 (GP15)

### 💡 LEDs
- **LED 1** -> GP11
- **LED 2** -> GP12
- **LED 3** -> GP13
- **LED RGB** -> GP0, GP11, GP12, GP13

## 🚀 Como Usar
1. **Compilar e carregar o firmware** para o Raspberry Pi Pico W.  
2. **Aguardar a conexão Wi-Fi** (exibida no display SSD1306).  
3. **Acessar a interface web** via o endereço IP exibido no display.  
4. **Controlar os LEDs e monitorar a temperatura** pela página web.  

## 📦 Dependências
- 📌 **Pico SDK**
- 🌐 **LWIP (Lightweight IP)**
- 🖥️ **Biblioteca SSD1306 para display OLED**

## 🏗️ Compilando o Projeto
1. Configurar e instalar o **Pico SDK**.
2. Criar um diretório `build` e rodar:
   ```sh
   cmake ..
   make
   ```
3. Carregar o arquivo `.uf2` gerado para o Raspberry Pi Pico W.

## 📜 Licença
Este projeto é de código aberto e pode ser modificado conforme necessidade. 📝




