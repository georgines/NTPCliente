# NTPCliente
Cliente NTP simples para Raspberry Pi Pico W que resolve DNS, envia uma requisicao UDP de 48 bytes e retorna o timestamp Unix pronto para sincronizar o relogio do dispositivo.

## Recursos Principais
- Resolve nomes de servidor NTP via DNS antes de qualquer envio.
- Monta e transmite automaticamente o pacote NTP padrao de 48 bytes.
- Converte a resposta de 1900 para 1970, devolvendo `time_t` pronto para uso.
- Utiliza callbacks de lwIP para receber eventos sem bloquear a aplicacao principal.
- Inclui protecao em CMake para garantir que a dependencia `Wifi` esteja presente.

## Requisitos
- CMake 3.13 ou superior.
- Raspberry Pi Pico SDK 2.2.0 (ou compativel)
- Biblioteca `Wifi` do projeto (fornece `lwipopts.h`, inicializacao de rede e exporta as dependencias lwIP necessarias).

## Como Integrar
Adicione as bibliotecas ao seu `CMakeLists.txt` raiz na ordem correta para expor a dependencia de `Wifi` antes de `NTPCliente`.

```cmake
add_subdirectory(Wifi)
add_subdirectory(NTPCliente)

add_executable(main main.cpp)

target_link_libraries(main
    PRIVATE
        Wifi
        NTPCliente
)
```

Inclua o cabecalho onde precisar obter o horario:

```cpp
#include "NTPCliente.h"
```

## Exemplo Rapido de Uso (Raspberry Pi Pico W)
```cpp
#include "pico/stdlib.h"
#include "Wifi.h"
#include "NTPCliente.h"

int main() {
    stdio_init_all();

    Wifi wifi("NOME_DA_REDE", "SENHA_DA_REDE");
    
    bool wifi_ok = wifi.iniciar();
    if (!wifi_ok) {
        printf("Wifi indisponivel\r\n");
        while (true) {
            sleep_ms(2000);
        }
    }

    NTPCliente ntpCliente;
    time_t timestamp = ntpCliente.obterTimestampUnix();
    
    if (timestamp == 0) 
    {
       printf("Falha ao obter timestamp NTP.\r\n");
    } 
    else 
    {        
        printf("Timestamp NTP obtido: %ld\r\n", static_cast<long>(timestamp));       
    }

    while (true) {   
        sleep_ms(1000);
    }
}
```

## Documentacao da API
- `NTPCliente(const char *servidor = "pool.ntp.org", uint16_t porta = 123)`
  - Define o host NTP e a porta que serao usados para cada consulta.
- `time_t obterTimestampUnix()`
  - Resolve o DNS, envia o pacote NTP e aguarda a resposta por tempo limitado. Retorna `0` em caso de falha ou o timestamp Unix em segundos quando bem-sucedido.

## Boas Praticas
- Garanta que a rede Wi-Fi esteja conectada antes de chamar `obterTimestampUnix()` para evitar timeouts desnecessarios.
- Evite chamadas repetitivas em sequencia; aguarde a sincronizacao concluir e utilize um temporizador para pedir novos horarios quando necessario.
- Sempre verifique o retorno do metodo e implemente uma logica de repeticao ou fallback caso o servidor nao responda.

## Licenca
Defina aqui a licenca aplicavel ao projeto (por exemplo, MIT, Apache-2.0 ou outra conforme sua necessidade).
