#include "NTPCliente.h"
#include <string.h>
#include "pico/cyw43_arch.h"
#ifdef HABILITAR_FREERTOS
#include "FreeRTOS.h"
#include "task.h"
#endif

#define TAMANHO_MSG_NTP 48
#define DIFERENCA_EPOCA 2208988800 // 1900 → 1970

static constexpr uint16_t TOTAL_TENTATIVAS_DNS = 100U;
static constexpr uint16_t TOTAL_TENTATIVAS_RESPOSTA = 200U;
static constexpr uint32_t INTERVALO_POLL_MS = 10U;

NTPCliente::NTPCliente(const char *servidor, uint16_t porta)
{

    servidorNTP = servidor;
    portaNTP = porta;
    dnsResolvido = false;
    pacoteRecebido = false;
    resultadoUnix = 0;

    cyw43_arch_lwip_begin();
    socketUDP = udp_new_ip_type(IPADDR_TYPE_ANY);
    bool socket_valido = socketUDP != nullptr;
    if (socket_valido)
    {
        udp_recv(socketUDP, NTPCliente::callbackResposta, this);
    }
    cyw43_arch_lwip_end();
}

time_t NTPCliente::obterTimestampUnix()
{

    dnsResolvido = false;
    pacoteRecebido = false;
    resultadoUnix = 0;

    cyw43_arch_lwip_begin();
    err_t status_dns = dns_gethostbyname(
        servidorNTP,
        &enderecoServidor,
        NTPCliente::callbackDNS,
        this);
    cyw43_arch_lwip_end();

    bool dns_resolvido_imediatamente = status_dns == ERR_OK;
    if (dns_resolvido_imediatamente)
    {
        dnsResolvido = true;
    }

    bool consulta_em_andamento = status_dns == ERR_INPROGRESS;
    if (!dns_resolvido_imediatamente && !consulta_em_andamento)
    {
        printf("Falha DNS NTP\n");
        return 0;
    }

    uint16_t tentativas_dns = 0U;
    while (!dnsResolvido && tentativas_dns < TOTAL_TENTATIVAS_DNS)
    {
        executarVarreduraRede();
        pausarExecucaoPorMilissegundos(INTERVALO_POLL_MS);
        tentativas_dns++;
    }

    if (!dnsResolvido)
    {
        printf("Falha DNS NTP\n");
        return 0;
    }

    enviarRequisicaoNTP();

    uint16_t tentativas_resposta = 0U;
    while (!pacoteRecebido && tentativas_resposta < TOTAL_TENTATIVAS_RESPOSTA)
    {
        executarVarreduraRede();
        pausarExecucaoPorMilissegundos(INTERVALO_POLL_MS);
        tentativas_resposta++;
    }

    if (!pacoteRecebido)
    {
        printf("Falha: nenhuma resposta NTP\n");
        return 0;
    }

    return resultadoUnix;
}

void NTPCliente::enviarRequisicaoNTP()
{
    cyw43_arch_lwip_begin();
    struct pbuf *pacote = pbuf_alloc(PBUF_TRANSPORT, TAMANHO_MSG_NTP, PBUF_RAM);
    bool pacote_invalido = pacote == nullptr;
    if (pacote_invalido)
    {
        cyw43_arch_lwip_end();
        return;
    }

    uint8_t *msg = (uint8_t *)pacote->payload;
    memset(msg, 0, TAMANHO_MSG_NTP);
    msg[0] = 0x1B; // LI | Versão | Cliente

    udp_sendto(socketUDP, pacote, &enderecoServidor, portaNTP);

    pbuf_free(pacote);
    cyw43_arch_lwip_end();
}

void NTPCliente::callbackDNS(const char *nome, const ip_addr_t *ip, void *arg)
{
    NTPCliente *self = (NTPCliente *)arg;

    if (ip)
    {
        self->enderecoServidor = *ip;
        self->dnsResolvido = true;
    }
}

void NTPCliente::callbackResposta(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port)
{

    NTPCliente *self = (NTPCliente *)arg;

    if (!p || p->tot_len != TAMANHO_MSG_NTP)
    {
        if (p)
            pbuf_free(p);
        return;
    }

    uint8_t segundosBuffer[4];
    pbuf_copy_partial(p, segundosBuffer, 4, 40);

    uint32_t segundos1900 =
        (segundosBuffer[0] << 24) |
        (segundosBuffer[1] << 16) |
        (segundosBuffer[2] << 8) |
        (segundosBuffer[3]);

    uint32_t segundosUnix = segundos1900 - DIFERENCA_EPOCA;

    self->resultadoUnix = segundosUnix;
    self->pacoteRecebido = true;

    pbuf_free(p);
}

void NTPCliente::pausarExecucaoPorMilissegundos(uint32_t tempo_ms)
{
#ifdef HABILITAR_FREERTOS
    TickType_t intervalo_ticks = pdMS_TO_TICKS(tempo_ms);
    vTaskDelay(intervalo_ticks);
#else
    sleep_ms(tempo_ms);
#endif
}

void NTPCliente::executarVarreduraRede()
{
#ifndef HABILITAR_FREERTOS
    cyw43_arch_poll();
#endif
}
