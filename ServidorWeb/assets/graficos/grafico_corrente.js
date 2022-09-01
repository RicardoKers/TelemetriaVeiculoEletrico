   
  /*
  * PARTE DOS GRAFICOS
  */

  google.charts.load('current', {'packages':['corechart']});
  google.charts.setOnLoadCallback(draw_corrente_carro);
  const options_corrente_carro = {
    min: -5,
    max: 60,
    backgroundColor: '#fff',
    title: 'Corrente',
    curveType: 'function',
    lineWidth: 4
  };

  let draw_corrente = [['Tempo', 'Corrente']];

  function draw_corrente_carro(corrente) {
    if (draw_corrente.length > 15)
      draw_corrente.splice(1, 1);
      
    const tempo_atual = new Date();     
    draw_corrente.push([tempo_atual.toLocaleTimeString('pt-BR'), corrente]);


    let data = google.visualization.arrayToDataTable(draw_corrente);

    let chart = new google.visualization.LineChart(document.getElementById('grafico_corrente_carro'));
    chart.draw(data, options_corrente_carro);
  }