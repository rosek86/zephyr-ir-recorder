#include <stdint.h>
#include <stdbool.h>

#define NEC_DECODER_STATE_WAIT_START_BURST  0
#define NEC_DECODER_STATE_WAIT_START_SPACE  1

#define NEC_DECODER_DATA_SIZE               32

#define NEC_DECODER_START_BURST_TIME_US     9000
#define NEC_DECODER_START_SPACE_TIME_US     4500
#define NEC_DECODER_DATA_BURST_TIME_US      562
#define NEC_DECODER_DATA_SPACE_0_TIME_US    562
#define NEC_DECODER_DATA_SPACE_1_TIME_US    1687

struct {
  uint32_t state;
  uint32_t data;
  uint32_t bit;
} nec_decoder = {
  .state = NEC_DECODER_STATE_WAIT_START_BURST,
  .data = 0,
  .bit = 0,
};

static bool nec_decoder_is_time_valid(uint32_t value, uint32_t target, uint32_t tolerance_percent) {
  uint32_t tolerance = target * tolerance_percent / 100;
  uint32_t min = target - tolerance;
  uint32_t max = target + tolerance;
  return value > min && value < max;
}

bool nec_decoder_add_pulse(uint32_t time, uint32_t *code) {
  if (!code) {
    return false;
  }

  // start burst
  if (nec_decoder.state == NEC_DECODER_STATE_WAIT_START_BURST) {
    if (nec_decoder_is_time_valid(time, NEC_DECODER_START_BURST_TIME_US, 10)) {
      nec_decoder.state = NEC_DECODER_STATE_WAIT_START_SPACE;
    }
    return false;
  }

  // start sapce
  if (nec_decoder.state == NEC_DECODER_STATE_WAIT_START_SPACE) {
    if (nec_decoder_is_time_valid(time, NEC_DECODER_START_SPACE_TIME_US, 10)) {
      nec_decoder.state = 2;
      nec_decoder.data = 0;
      nec_decoder.bit = 0;
    } else {
      nec_decoder.state = NEC_DECODER_STATE_WAIT_START_BURST;
    }
    return false;
  }

  // data bit burst
  if ((nec_decoder.state % 2) == 0) {
    if (nec_decoder_is_time_valid(time, NEC_DECODER_DATA_BURST_TIME_US, 30)) {
      nec_decoder.state++;
    } else {
      nec_decoder.state = NEC_DECODER_STATE_WAIT_START_BURST;
    }
    return false;
  }

  // data bit space
  bool bit0 = nec_decoder_is_time_valid(time, NEC_DECODER_DATA_SPACE_0_TIME_US, 30);
  bool bit1 = nec_decoder_is_time_valid(time, NEC_DECODER_DATA_SPACE_1_TIME_US, 30);

  if (!bit0 && !bit1) {
    nec_decoder.state = NEC_DECODER_STATE_WAIT_START_BURST;
    return false;
  }

  if (bit1) {
    nec_decoder.data |= (1 << 31 - nec_decoder.bit);
  }

  nec_decoder.bit++;
  nec_decoder.state++;

  if (nec_decoder.bit == NEC_DECODER_DATA_SIZE) {
    nec_decoder.state = NEC_DECODER_STATE_WAIT_START_BURST;
    *code = nec_decoder.data;
    return true;
  }
}
