#ifndef LORA_PINS_H
#define LORA_PINS_H

#include <hardware/spi.h>

// Ajuste estes pinos conforme o cabeçalho IDC central da BitDogLab
// Mapeamento padrão (RP2040 SPI0):
//   SCK  -> GPIO18
//   MOSI -> GPIO19
//   MISO -> GPIO16
//   CS   -> GPIO17
//   DIO0 -> GPIO20 (opcional)
//   RST  -> GPIO21 (opcional)

#ifndef LORA_SPI_PORT
#define LORA_SPI_PORT spi0
#endif

#ifndef LORA_PIN_SCK
#define LORA_PIN_SCK 18
#endif

#ifndef LORA_PIN_MOSI
#define LORA_PIN_MOSI 19
#endif

#ifndef LORA_PIN_MISO
#define LORA_PIN_MISO 16
#endif

#ifndef LORA_PIN_CS
#define LORA_PIN_CS 17
#endif

#ifndef LORA_PIN_DIO0
#define LORA_PIN_DIO0 20
#endif

#ifndef LORA_PIN_RST
#define LORA_PIN_RST 21
#endif

#endif // LORA_PINS_H