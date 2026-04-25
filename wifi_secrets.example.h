#ifndef WIFI_SECRETS_EXAMPLE_H
#define WIFI_SECRETS_EXAMPLE_H

#include <IPAddress.h>

// Copie este arquivo para wifi_secrets.h e preencha com a sua rede.

#define WIFI_SSID "SUA_REDE"
#define WIFI_PASS "SUA_SENHA"

static const IPAddress WIFI_LOCAL_IP(192, 168, 1, 70);
static const IPAddress WIFI_GATEWAY(192, 168, 1, 1);
static const IPAddress WIFI_SUBNET(255, 255, 255, 0);
static const IPAddress WIFI_DNS_PRIMARY(8, 8, 8, 8);
static const IPAddress WIFI_DNS_SECONDARY(8, 8, 4, 4);

#endif
