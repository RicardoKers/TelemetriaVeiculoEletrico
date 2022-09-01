let elemento_tensao_bateria = '';
let itens_tensao_bateria = [];

const header_tensao_bateria = 
            `<table class="table table-striped bg-white">
                <thead>
                    <tr>
                        <th>Tensão da Bateria</th>
                        <th>Hora</th>
                    </tr>
                </thead>
                <tbody>`;

                const foot_tensao_bateria = `</tbody> </table>`;

function tabela_tensao_bateria() {
    elemento_tensao_bateria = '';
    if (itens_tensao_bateria.length > 15) 
        itens_tensao_bateria.splice(0, 1);
    
    let cel_tensao_bateria = document.getElementById("table_tesao_bateria");
    
    itens_tensao_bateria.forEach(function(item_tensao_bateria) {
        elemento_tensao_bateria =  
            `<tr>
            <td>`+item_tensao_bateria[0]+`</td>
            <td>`+item_tensao_bateria[1]+`</td>
            </tr>` + elemento_tensao_bateria;
        });   

    cel_tensao_bateria.innerHTML = [header_tensao_bateria + elemento_tensao_bateria + foot_tensao_bateria].join("\n");
}