const reconnectTimeout = 2000;
const host = "automacao.luzerna.ifc.edu.br";
const port = 8000;
const topico_velocidade = "/tve/velocidade";
const topico_temperatura_bateria = "/tve/tempbat";
const topico_temperatura_veiculo = "/tve/tempvei";
const topico_tensao_bateria = "/tve/tensaobat";
const topico_corrente = "/tve/corrente";

function onConnect() {
  mqtt.subscribe(topico_velocidade);
  mqtt.subscribe(topico_temperatura_bateria);
  mqtt.subscribe(topico_temperatura_veiculo);
  mqtt.subscribe(topico_tensao_bateria);
  mqtt.subscribe(topico_corrente);
}
  
function onError(message) {
  console.log("Falha: " + message.errorCode + " " + message.errorMessage);
  setTimeout(MQTTConnect, reconnectTimeout);
}

function onMessageArrived(msg) {

  if (msg.destinationName == topico_velocidade)
    draw_velocidade_carro(Number.parseFloat(msg.payloadString));   
  
  if (msg.destinationName == topico_corrente)
    draw_corrente_carro(Number.parseFloat(msg.payloadString));   

  if (msg.destinationName == topico_temperatura_bateria) { 
    const hora_atual_bateria = new Date();    
    itens_tempbat.push([Number.parseFloat(msg.payloadString), hora_atual_bateria.toLocaleTimeString('pt-BR')]);
    tabela_temp_bateria();
  }

  if (msg.destinationName == topico_temperatura_veiculo) {
    const hora_atual_temp_vei = new Date();    
    itens_temp_vei.push([Number.parseFloat(msg.payloadString), hora_atual_temp_vei.toLocaleTimeString('pt-BR')]);
    tabela_temp_veiculo();
  }

  if (msg.destinationName == topico_tensao_bateria) {
    const hora_tensao_bateria = new Date();    
    itens_tensao_bateria.push([Number.parseFloat(msg.payloadString), hora_tensao_bateria.toLocaleTimeString('pt-BR')]);
    tabela_tensao_bateria();
  }
}

function MQTTConnect() {
  
  mqtt = new Paho.MQTT.Client(host, port, "IeCJSClient" + parseInt(Math.random() * 1e5));
  const options = { timeout: 30,
                  keepAliveInterval: 30,
                  onSuccess: onConnect,
                  onFailure: onError
                };
  mqtt.onMessageArrived = onMessageArrived;
  mqtt.onConnectionLost = onError;
  mqtt.connect(options);
}

 