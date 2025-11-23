#pragma once
#include <time.h>
#include "pico/stdlib.h"
#include "lwip/udp.h"
#include "lwip/dns.h"

class NTPCliente
{
public:
    NTPCliente(const char *servidor = "pool.ntp.org", uint16_t porta = 123);

    time_t obterTimestampUnix();

private:
    const char *servidorNTP;
    uint16_t portaNTP;
    
    ip_addr_t enderecoServidor;
    struct udp_pcb *socketUDP;

    static void callbackDNS(const char *nome, const ip_addr_t *ip, void *arg);
    static void callbackResposta(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port);

    volatile bool dnsResolvido;
    volatile bool pacoteRecebido;
    volatile time_t resultadoUnix;

    void enviarRequisicaoNTP();
};
