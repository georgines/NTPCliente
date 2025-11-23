#include "NTPCliente.h"
#include <string.h>

#define TAMANHO_MSG_NTP 48
#define DIFERENCA_EPOCA 2208988800 // 1900 → 1970

NTPCliente::NTPCliente(const char *servidor, uint16_t porta)
    : servidorNTP(servidor),
      portaNTP(porta),
      dnsResolvido(false),
      pacoteRecebido(false),
      resultadoUnix(0)
{
    socketUDP = udp_new_ip_type(IPADDR_TYPE_ANY);
    if (socketUDP)
    {
        udp_recv(socketUDP, NTPCliente::callbackResposta, this);
    }
}

time_t NTPCliente::obterTimestampUnix()
{

    dnsResolvido = false;
    pacoteRecebido = false;
    resultadoUnix = 0;

    int dnsStatus = dns_gethostbyname(
        servidorNTP,
        &enderecoServidor,
        NTPCliente::callbackDNS,
        this);

    if (dnsStatus == ERR_OK)
    {
        dnsResolvido = true;
    }

    int tentativas = 0;
    while (!dnsResolvido && tentativas < 100)
    {
        sleep_ms(10);
        tentativas++;
    }

    if (!dnsResolvido)
    {
        printf("Falha DNS NTP\n");
        return 0;
    }

    enviarRequisicaoNTP();

    tentativas = 0;
    while (!pacoteRecebido && tentativas < 200)
    {
        sleep_ms(10);
        tentativas++;
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
    struct pbuf *pacote = pbuf_alloc(PBUF_TRANSPORT, TAMANHO_MSG_NTP, PBUF_RAM);
    if (!pacote)
        return;

    uint8_t *msg = (uint8_t *)pacote->payload;
    memset(msg, 0, TAMANHO_MSG_NTP);
    msg[0] = 0x1B; // LI | Versão | Cliente

    udp_sendto(socketUDP, pacote, &enderecoServidor, portaNTP);

    pbuf_free(pacote);
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
