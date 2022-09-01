let elemento_temp_veiculo = '';
let itens_temp_vei = [];

const header_tempvei = 
            `<table class="table table-striped bg-white">
                <thead>
                    <tr>
                        <th>Temperatura do Veículo</th>
                        <th>Hora</th>
                    </tr>
                </thead>
                <tbody>`;

                const foot_tempvei = `</tbody> </table>`;

function tabela_temp_veiculo() {
    elemento_temp_veiculo = '';
    if (itens_temp_vei.length > 15) 
      itens_temp_vei.splice(0, 1);
    
    let cel_tempvei = document.getElementById("table_tempvei");
    
    itens_temp_vei.forEach(function(item_tempvei) {
        elemento_temp_veiculo =  
            `<tr>
            <td>`+item_tempvei[0]+`</td>
            <td>`+item_tempvei[1]+`</td>
            </tr>` + elemento_temp_veiculo;
    });

    cel_tempvei.innerHTML = [header_tempvei + elemento_temp_veiculo + foot_tempvei].join("\n");
}