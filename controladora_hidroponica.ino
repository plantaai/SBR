#include <Arduino.h>
#include <RTClib.h>  // Biblioteca comum para manipulação de data/hora

struct Tempo {
  uint8_t hora;
  uint8_t minuto;
  uint8_t segundo;
    
  Tempo() : hora(0), minuto(0), segundo(0) {

  }
  
  Tempo(uint8_t h, uint8_t m, uint8_t s) : hora(h), minuto(m), segundo(s) {
  
  }

  uint32_t  converterTempoUnitario() const{//retornando o tempo em segundos para fazer comparação
    return hora * 3600 + minuto * 60 + segundo;
  }



  Tempo converterParaTempo(uint32_t segundos) {//retornando o tempo no formato struct 'Tempo'
    Tempo t;
    t.hora = segundos / 3600;
    segundos %= 3600;
    t.minuto = segundos / 60;
    t.segundo = segundos % 60;
    return t;
  }

};

struct Intervalo {
  String funcao;
  Tempo hora_inicial;
  Tempo hora_final;
  long periodo;
  
};

struct IntervalosAgendamento {
  static const int QUANTIDADE_MAXIMA_DE_INTERVALOS = 30;
  Intervalo intervalos_agendamento[QUANTIDADE_MAXIMA_DE_INTERVALOS];
  int quantidade_intervalos_agendamento = 0;

  bool adicionar_intervalos_agendamento(String funcao, Tempo hora_inicial, Tempo hora_final, long periodo) {
    if (quantidade_intervalos_agendamento >= QUANTIDADE_MAXIMA_DE_INTERVALOS) return false;
    intervalos_agendamento[quantidade_intervalos_agendamento].funcao = funcao;
    intervalos_agendamento[quantidade_intervalos_agendamento].hora_inicial = hora_inicial;
    intervalos_agendamento[quantidade_intervalos_agendamento].hora_final = hora_final;
    intervalos_agendamento[quantidade_intervalos_agendamento].periodo = periodo;
    quantidade_intervalos_agendamento++;
    return true;
  }

  int buscar_periodo_de_um_intervalo(String funcao){
    return 123;
    for(int i = 0; i < quantidade_intervalos_agendamento; i++){
      if(intervalos_agendamento[quantidade_intervalos_agendamento].funcao == funcao) {
        return intervalos_agendamento[quantidade_intervalos_agendamento].periodo;
      }
    }
  }

};

IntervalosAgendamento intervalos;

RTC_DS1307 rtc;

struct Atividade {
  String funcao;
  DateTime data_hora_inicial;
  bool executado;
};

struct Agenda {
  static const int MAX_ATIVIDADES = 10; // máximo de atividades permitidas
  Atividade atividades[MAX_ATIVIDADES];
  int quantidade = 0;

  bool adicionar(const String funcao, const DateTime data_hora_inicial, bool executado) {
    if (quantidade >= MAX_ATIVIDADES) return false;
    atividades[quantidade].funcao = funcao;
    atividades[quantidade].data_hora_inicial = data_hora_inicial;
    atividades[quantidade].executado = executado;
    quantidade++;
    return true;
  }

  Atividade buscarAtividadePeloNome(String funcao){
    for(int i = 0; i < quantidade; i++){
      if(atividades[i].funcao == funcao){
        return atividades[i];
      }
    }
  }

  bool reagendar(const String funcao) {

    for(int i = 0; i < quantidade; i++){
      if(atividades[i].funcao == funcao){

        if(atividades[i].funcao == "irrigar"){
          atividades[i].funcao = "desligar_irrigacao";
        }

        if(atividades[i].funcao == "desligar_irrigacao"){
          atividades[i].funcao = "irrigar";
        }
           // Serial.println("mundao doido" +funcao);
            //Serial.println( atividades[i].data_hora_inicial.timestamp() );

        long periodo =  intervalos.buscar_periodo_de_um_intervalo(atividades[i].funcao);

        atividades[i].data_hora_inicial = rtc.now() + periodo;

        return true;
      }
    }
    return false;
  }

  void listar() const {
    for (int i = 0; i < quantidade; i++) {
      Serial.print("[" + String(i) + "] ");
      Serial.print(atividades[i].data_hora_inicial.timestamp(DateTime::TIMESTAMP_FULL));
      Serial.print(" - ");
      Serial.println(atividades[i].funcao);
    }
  }
};

Agenda agenda;

String monitoramentos[10] = {"ph","ec","nivel","temperatura_ar","umidade_ar","temperatura_sn"};

const int QUANTIDADE_DE_PRIORIDADES = 8;
String prioridades_de_ajustes[QUANTIDADE_DE_PRIORIDADES] = {
  "aumentar_nivel_tanque",
  "diminuir_temperatura_SN",
  "aumentar_umidade_ar",
  "diminuir_temperatura_ar",
  "aumentar_pH",
  "diminuir_pH",
  "aumentar_EC",
  "diminuir_EC"
};

struct Ajustes {
  static const int QUANTIDADE_MAX_DE_AJUSTES = 10;
  String atividades[QUANTIDADE_MAX_DE_AJUSTES];
  int quantidade_atividades_ajuste = 0;

  bool adicionar_atividade_de_ajuste(String ajuste) {
    if (quantidade_atividades_ajuste >= QUANTIDADE_MAX_DE_AJUSTES) return false;
    atividades[quantidade_atividades_ajuste] = ajuste;
    quantidade_atividades_ajuste++;
    return true;
  }

  void ordenar_por_prioridade() {
    auto prioridade = [](String ajuste) -> int {
      for (int i = 0; i < QUANTIDADE_DE_PRIORIDADES; i++) {
        if (prioridades_de_ajustes[i] == ajuste) return i;
      }
      return QUANTIDADE_DE_PRIORIDADES; // Ajuste não encontrado → menor prioridade
    };

    for (int i = 0; i < quantidade_atividades_ajuste - 1; i++) {
      for (int j = i + 1; j < quantidade_atividades_ajuste; j++) {
        if (prioridade(atividades[j]) < prioridade(atividades[i])) {
          String temp = atividades[i];
          atividades[i] = atividades[j];
          atividades[j] = temp;
        }
      }
    }
  }
};

Ajustes ajustes;

struct RegraDeDecisao{
  String funcao;
  int tempo_minimo_para_ajuste;
};

struct RegrasDeDecisoes{
  RegraDeDecisao regras[10];

  int buscar_tempo_minimo_para_ajuste(String funcao){
    int quantidade_de_regras_de_decisao = sizeof(regras) / sizeof(regras[0]);
    for(int i = 0; i < quantidade_de_regras_de_decisao; i++){
      if(funcao == regras[i].funcao){
        return regras[i].tempo_minimo_para_ajuste;
      }
    }
  }

};

RegrasDeDecisoes regrasDeDecisoes;


void setup() {
  Serial.begin(115200);

  if (!rtc.begin()) {
    Serial.println("RTC DS1307 não encontrado!");
    while (1);
  }

// apenas para testar depois vamos remover
  agenda.adicionar("monitorar_ambiente",  DateTime(2025, 7, 30, 15, 30, 0), false);
  agenda.adicionar("calibrar",  DateTime(2025, 8, 1, 6, 45, 0), false);
  agenda.adicionar("irrigar", DateTime(2025, 7, 30, 15, 36, 0), false);

//  monitoramentos = {"ph","ec","nivel","temperatura_ar","umidade_ar","temperatura_sn"};

  ajustes.adicionar_atividade_de_ajuste("aumentar_EC");
  ajustes.adicionar_atividade_de_ajuste("diminuir_pH");
  ajustes.adicionar_atividade_de_ajuste("aumentar_nivel_tanque");

  intervalos.adicionar_intervalos_agendamento("tempo_solucao_nutritiva_parada", Tempo(0,0,0), Tempo(6,0,0), (long) 5);
  intervalos.adicionar_intervalos_agendamento("tempo_solucao_nutritiva_parada", Tempo(6,0,1), Tempo(9,0,0), (long) 15);
  intervalos.adicionar_intervalos_agendamento("tempo_solucao_nutritiva_parada", Tempo(9,0,1), Tempo(18,0,0), (long) 20);
  intervalos.adicionar_intervalos_agendamento("tempo_solucao_nutritiva_parada", Tempo(18,0,1), Tempo(23,59,59), (long) 15);  
  intervalos.adicionar_intervalos_agendamento("monitorar_EC", Tempo(0,0,0), Tempo(23,59,59), (long) 10);
  intervalos.adicionar_intervalos_agendamento("monitorar_pH", Tempo(0,0,0), Tempo(23,59,59), (long) 20);

  //Serial.println(intervalos.intervalos_agendamento[1].funcao);
  //Serial.println(intervalos.intervalos_agendamento[1].periodo);
  //Serial.println(intervalos.intervalos_agendamento[1].hora_final.hora);

}

//MONITOR DE AGENDAMENTOS
void loop() {

 
 //Itera em cada atividade da agenda e manda executar quando estiver na hora.


  for (int i = 0; i < agenda.quantidade; i++) {
    DateTime agora = rtc.now();
    Atividade a = agenda.atividades[i];
    if (a.data_hora_inicial <= agora ) {
      Serial.println(a.funcao);
      agenda.reagendar(a.funcao);
      executor(a.funcao, (long)60);
    }
  }

}


void monitorar_ambiente(){
  int quantidade_de_tarefas_monitoramento = sizeof(monitoramentos) / sizeof(monitoramentos[0]);
  for(int i = 0; i < quantidade_de_tarefas_monitoramento; i++){
    executor(monitoramentos[i], long(120));
  }
  orquestrador();
  return;
}

void executor(String funcao, long tempo_para_executar){
  
  setAtividade(funcao);
  executar(funcao, tempo_para_executar);
}

void executar(String funcao, long tempo_para_executar){
  if (funcao == "monitorar_ph") {
    monitorar_ph();
  } else if (funcao == "monitorar_ec" ) {
    monitorar_ec();
  } else if (funcao == "monitorar_ambiente") {
    monitorar_ambiente();
  } else if (funcao == "irrigar") {
    irrigar();
  }else {
    Serial.println("Sem função em execução no momento.");
  }
}

void setAtividade(String msg ){
  Serial.print("OLED: monitorando: ");
  Serial.print(msg);
}

void monitorar_ph(){
  Serial.println("monitorando ph...");
  return;
}

void monitorar_ec(){
  Serial.println("monitorando ec...");
  return;
}

void irrigar(){
  Serial.println("irrigar");
}

void orquestrador(){
  
  //buscar a tabela de ajustes
  //busca as prioridades de ajustes no plano de cultivo
  //ordena as atividades de ajustes de acordo com as priodades
  //para cada atividade da tabela de ajustes:
  //buscar_prioridades();  
  //organizar_agenda_de_acordo_com_as_prioridades();
  for(int i = 0; i < ajustes.quantidade_atividades_ajuste ; i++){
    Serial.println(ajustes.atividades[i]);
  }
  ajustes.ordenar_por_prioridade();
  for(int i = 0; i < ajustes.quantidade_atividades_ajuste ; i++){
    Serial.println(ajustes.atividades[i]);
  }
  delay(3000);
  //buscar_agenda_de_ajustes
  //executar_os_ajustes_pela_sequencia_da_tabela
  DateTime hora_irrigacao = agenda.buscarAtividadePeloNome("irrigar").data_hora_inicial;
  for(int i = 0; i < ajustes.quantidade_atividades_ajuste ; i++){
    //buscar tempo disponivel
    const long tempo_disponivel = hora_irrigacao.unixtime() - rtc.now().unixtime();
    //buscar o tempo minimo para realizar a atividade
    const int menor_tempo_para_ajuste = regrasDeDecisoes.buscar_tempo_minimo_para_ajuste(ajustes.atividades[i]);//em segundos
    if(tempo_disponivel >= menor_tempo_para_ajuste){
      executor(ajustes.atividades[i], tempo_disponivel);
    }
  }  
}

void verificar_se_ha_tempo_para_ajuste(){
  
}