#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

#include <Arduino.h>

#define BUTTON1_PIN 12
#define BUTTON2_PIN 14
#define BUTTON3_PIN 27

#define DEBOUNCE_MS 400  // Tempo mínimo entre eventos por botão

QueueHandle_t buttonQueue;

// === Variaveis globais ===
//ph
float ph = 0.0;
float ultimo_ph = -1.0;
//ec
float ec = 0.0;
float ultimo_ec = -1.0;
//temperatura ar
float temperatura_ar = 0.0;
float ultima_temperatura_ar = -999.0;
//umidade ar
float umidade_ar = 0.0;
float ultima_umidade_ar = -1.0;
//temperatura agua
float temperatura_agua = 0.0;
float ultima_temperatura_agua = -999.0;
//nivel
String nivel_tanque = "";
//fluxo sn
bool fluxo_solucao_nutritiva = false;
//fluxo ap
bool fluxo_agua_potavel = false;

// === Pinos do display OLED ST7789 ===
#define TFT_CS   15
#define TFT_RST  2
#define TFT_DC   4

//display
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);
int telaAtual = 0;
int ultimaTelaAtual = 0;

enum ButtonID {
  BUTTON1,
  BUTTON2,
  BUTTON3,
  UNKNOWN
};

void imprimir_tela_inicial(){
  imprimir_tela_inicial_esquerda();
  tft.drawLine(180, 0, 180, 240, ST77XX_WHITE);
  imprimir_tela_inicial_direita();
}



bool imprimir_float_OLED(float &variavel, float &ultimaVariavel, int x, int y, int tamanhoFonte, uint16_t cor){
  if(variavel != ultimaVariavel){
    int16_t x1, y1;
    uint16_t largura_texto, altura_texto;
    tft.setTextSize(tamanhoFonte);
    tft.getTextBounds(String(ultimaVariavel, 2), x, y, &x1, &y1, &largura_texto, &altura_texto);
    tft.fillRect(x1, y1, largura_texto, altura_texto, ST77XX_BLACK);
    bool retorno = imprimir_msg_OLED(String(variavel,2),x,y,tamanhoFonte,cor);
    ultimaVariavel = variavel;
    return retorno;
  } else {
    imprimir_msg_OLED(String(variavel,2),x,y,tamanhoFonte,cor);
  }
  return false;
}


bool imprimir_msg_OLED(String msg, int x, int y, int tamanhoFonte, uint16_t cor) {
  int largura = tft.width();
  int altura = tft.height();

  tft.setTextSize(tamanhoFonte);

  int16_t x1, y1;  //coordenadas da caixa delimitadora
  uint16_t larguraTexto, alturaTexto;  //largura e altura do texto

  //dimensões reais do texto
  tft.getTextBounds(msg, x, y, &x1, &y1, &larguraTexto, &alturaTexto);

  if ((x + larguraTexto > largura)) {
    return false;
  }

  tft.setCursor(x, y);
  tft.setTextColor(cor);
  tft.print(msg);

  return true;  // Texto impresso com sucesso
}

void imprimir_tela_inicial_esquerda(){
  int posicao_x = 5;
  imprimir_msg_OLED("pH: ",posicao_x,5,2,ST77XX_WHITE);
  imprimir_float_OLED(ph, ultimo_ph, posicao_x+40,5,2,ST77XX_GREEN);

  imprimir_msg_OLED("EC: ",posicao_x,35,2,ST77XX_WHITE);
  imprimir_float_OLED(ec, ultimo_ec, posicao_x+40,35,2,ST77XX_GREEN);

  imprimir_msg_OLED("Tmp ar: ",posicao_x,65,2,ST77XX_WHITE);
  
  imprimir_msg_OLED("Umd ar: ",posicao_x,95,2,ST77XX_WHITE);
  
  imprimir_msg_OLED("Tmp agua:",posicao_x,125,2,ST77XX_WHITE);
  
  imprimir_msg_OLED("Tmp agua:",posicao_x,125,2,ST77XX_WHITE);
  
  imprimir_msg_OLED("Nivel: ",posicao_x,155,2,ST77XX_WHITE);
  
  imprimir_msg_OLED("Flux S.N.:",posicao_x,185,2,ST77XX_WHITE);
  
  imprimir_msg_OLED("Flux A.P.:",posicao_x,215,2,ST77XX_WHITE);
}

void imprimir_tela_inicial_direita(){
  int posicao_x = 185;
  imprimir_msg_OLED("neb.: ",posicao_x,5,2,ST77XX_WHITE);
  imprimir_msg_OLED("dias: ",posicao_x,35,2,ST77XX_WHITE);
  imprimir_msg_OLED("ativ.: ",posicao_x,65,2,ST77XX_WHITE);
  imprimir_msg_OLED("exaust.: ",posicao_x,95,2,ST77XX_WHITE);
}

void imprimir_tela_2(){
  int posicao_x = 135;
  imprimir_msg_OLED("Tela 2",posicao_x,5,3,ST77XX_WHITE);
}

void imprimir_tela_3(){
  int posicao_x = 135;
  imprimir_msg_OLED("Tela 3",posicao_x,5,3,ST77XX_WHITE);
}

void imprimir_tela_4(){
  int posicao_x = 135;
  imprimir_msg_OLED("Tela 4",posicao_x,5,3,ST77XX_WHITE);
}

void imprimir_tela_5(){
  int posicao_x = 135;
  imprimir_msg_OLED("Tela 5",posicao_x,5,3,ST77XX_WHITE);
}


void (*telas[])() = { //vetor de ponteiros para funções de exibição de tela
  imprimir_tela_inicial,
  imprimir_tela_2,
  imprimir_tela_3,
  imprimir_tela_4,
  imprimir_tela_5
};

void exibir_tela_atual(){
  telas[telaAtual]();
}

// ISR única para os três botões
void IRAM_ATTR sharedISR() {
  ButtonID id = UNKNOWN;

  if (digitalRead(BUTTON1_PIN) == LOW) {
    id = BUTTON1;
  } else if (digitalRead(BUTTON2_PIN) == LOW) {
    id = BUTTON2;
  } else if (digitalRead(BUTTON3_PIN) == LOW) {
    id = BUTTON3;
  }

  if (id != UNKNOWN) {
    xQueueSendFromISR(buttonQueue, &id, NULL);
  }
}

// Loop do núcleo 0 com tratamento e debounce
void buttonLoopTask(void *parameter) {
  ButtonID received;
  unsigned long lastPressed[3] = {0, 0, 0};

  while (true) {
    if (xQueueReceive(buttonQueue, &received, pdMS_TO_TICKS(10))) {
      unsigned long now = millis();
      bool process = false;
      int quantidadeTotalTelas = sizeof(telas) / sizeof(telas[0]);
      switch (received) {
        case BUTTON1:
          if (now - lastPressed[0] >= DEBOUNCE_MS) {
            lastPressed[0] = now;
            process = true;
            Serial.println("[Núcleo 0] Botão 1 pressionado");
            telaAtual = (telaAtual - 1 + quantidadeTotalTelas) % quantidadeTotalTelas;
            ultimaTelaAtual = telaAtual;
            tft.fillScreen(ST77XX_BLACK);
          }
          break;
        case BUTTON2:
          if (now - lastPressed[1] >= DEBOUNCE_MS) {
            lastPressed[1] = now;
            process = true;
            Serial.println("[Núcleo 0] Botão 2 pressionado");
          }
          break;
        case BUTTON3:
          if (now - lastPressed[2] >= DEBOUNCE_MS) {
            lastPressed[2] = now;
            process = true;
            Serial.println("[Núcleo 0] Botão 3 pressionado");
            telaAtual = (telaAtual + 1 + quantidadeTotalTelas) % quantidadeTotalTelas;
            ultimaTelaAtual = telaAtual;
            tft.fillScreen(ST77XX_BLACK);
          }
          break;
        default:
          break;
      }

      // Se necessário, coloque ações adicionais aqui, dentro de `if (process)`
    }

    exibir_tela_atual();

    vTaskDelay(pdMS_TO_TICKS(10));  // Tempo de espera no "loop"
  }
}


void setup() {
  //==============TECNOLOGIA==============
  Serial.begin(115200);
  
  //ST7789
  tft.init(240, 320);
  tft.setRotation(3);
  tft.fillScreen(ST77XX_BLACK);

  // Configuração dos pinos com pull-up
  pinMode(BUTTON1_PIN, INPUT_PULLUP);
  pinMode(BUTTON2_PIN, INPUT_PULLUP);
  pinMode(BUTTON3_PIN, INPUT_PULLUP);

  // Cria fila de eventos dos botões
  buttonQueue = xQueueCreate(10, sizeof(ButtonID));

  // Cria tarefa fixa no núcleo 0
  xTaskCreatePinnedToCore(
    buttonLoopTask,
    "ButtonLoopTask",
    2048,
    NULL,
    1,
    NULL,
    0
  );

  // Registra ISR única
  attachInterrupt(BUTTON1_PIN, sharedISR, FALLING);
  attachInterrupt(BUTTON2_PIN, sharedISR, FALLING);
  attachInterrupt(BUTTON3_PIN, sharedISR, FALLING);

 /* 
  //bool ha_login_na_flash = verificar_se_ha_login_e_senha_wifi_flash();
  
  if(ha_login_na_flash)}{
    bool ha_pc_flash = verificar_se_ha_plano_de_cultivo_na_flash();
    if(ha_pc_flash){
      buscar_parametros_crescimento_da_fase(); //parametros são - cabeçalho, constantes da controladora, prioridades, regras de decisões, parametros ambientas e intervalos de agendamento da fase
      criar_agenda();


    }
  }
*/
  
}

void loop() {
  /*
  Agenda copia_agenda[] = fazer_copia_da_agenda();
  para cada atividade da cópia da agenda:
    bool esta_na_hora_executar = verrificar_se_esta_na_hora_de_executar
    if(esta_na_hora_executar):
      oled.set_atividade(atividade.nome);
      apagar_a_atividade_na_agenda(atividade.nome);
      executor.executar(atividade.nome);
  */
}


/*
salvar_calibracao_sd_card(){

}
oled.set_atividade(String atividade.nome){
  ...
  oled.atualizar_atividade();
}

executor.executar(){
  acessa um switch-case:
    escolhe o caso de uso
    executa o caso de uso
}

caso_de_uso(){
  medir();
  buscar_regras_de_decisoes();
  tomar_decisão_de_acordo_com_regras_de_decisoes();
  se necessário:
    agendar_ajuste(ajustar_o_caso_de_uso);
  registrar_o_log(chave, valor, observacao);
  agendar(monitoramento_do_caso);
  return;
}

agendar(atividade.nome){
  insire a atividade.nome na agenda observando os intervalos de agendamento
}

agendar_ajuste(ajuste.nome){
  insere ajuste.nome na tabela de ajustes
}

orquestrar_ajustes(){
  buscar_prioridades();
  buscar_tabela_de_ajustes();
  organizar_tabela_De_acordo_com_as_prioridades();
  executar_os_ajustes_pela_sequencia_da_tabela();
}

*/