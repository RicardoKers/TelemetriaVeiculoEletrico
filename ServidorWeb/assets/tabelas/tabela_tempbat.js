let elemento_temp_bateria = '';
let itens_tempbat = [];

const header = 
            `<table class="table table-striped bg-white">
                <thead>
                    <tr>
                        <th>Temperatura da Bateria</th>
                        <th>Hora</th>
                    </tr>
                </thead>
                <tbody>`;

                const foot = `</tbody> </table>`;

function tabela_temp_bateria() {
    elemento_temp_bateria = '';
    if (itens_tempbat.length > 15) 
      itens_tempbat.splice(0, 1);
      
    let el_tempbat = document.getElementById("table_tempbat");
    
    itens_tempbat.forEach(function(item) {
    elemento_temp_bateria =  
        `<tr>
        <td>`+item[0]+`</td>
        <td>`+item[1]+`</td>
        </tr>` + elemento_temp_bateria;
    });

    el_tempbat.innerHTML = [header + elemento_temp_bateria + foot].join("\n");
}