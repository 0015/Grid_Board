#pragma once
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void ble_server_register_callbacks(void (*on_connect)(bool connected),
                                   void (*on_write)(const uint8_t *data, uint16_t len));
void ble_server_start(const char *device_name);

#ifdef __cplusplus
}
#endif
