   
  /*
  * PARTE DOS GRAFICOS
  */

  google.charts.load('current', {'packages':['corechart']});
  google.charts.setOnLoadCallback(draw_velocidade_carro);
  const options_velocidade_carro = {
    min: -5,
    max: 60,
    backgroundColor: '#fff',
    title: 'Velocidade do Carro',
    curveType: 'function',
    lineWidth: 4
  };

  let draw_velocidade = [['Tempo', 'Velocidade do Carro']];

  function draw_velocidade_carro(velocidade) {
      if (draw_velocidade.length > 15) 
        draw_velocidade.splice(1, 1);
      
    const tempo_atual = new Date();     
    draw_velocidade.push([tempo_atual.toLocaleTimeString('pt-BR'), velocidade]);


    let data = google.visualization.arrayToDataTable(draw_velocidade);

    let chart = new google.visualization.LineChart(document.getElementById('grafico_velocidade_carro'));
    chart.draw(data, options_velocidade_carro);
  }