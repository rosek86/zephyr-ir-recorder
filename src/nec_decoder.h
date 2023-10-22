#ifndef NEC_DECODER_H__
#define NEC_DECODER_H__

#include <stdint.h>
#include <stdbool.h>

bool nec_decoder_add_pulse(uint32_t time, uint32_t *code);

#endif // NEC_DECODER_H__
