// ===================================================================================
// CMSIS-DAP Commands
// ===================================================================================
/*
 * Copyright (c) 2013-2017 ARM Limited. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * ----------------------------------------------------------------------
 *
 * $Date:        1. December 2017
 * $Revision:    V2.0.0
 *
 * Project:      CMSIS-DAP Source
 * Title:        DAP.c CMSIS-DAP Commands
 *
 *---------------------------------------------------------------------------*/
 
#include <string.h>
#include "dap.h"

#pragma disable_warning 110

// ===================================================================================
// Generate SWJ Sequence
//   count:  sequence bits count
//   data:   pointer to sequence bits data
//   return: none
// ===================================================================================
void SWJ_Sequence(uint8_t count, const __xdata uint8_t *data) {
  uint8_t val;
  uint8_t n;

  val = 0U;
  n   = 0U;
  while(count--) {
    if(n == 0U) {
      val = *data++;
      n   = 8U;
    }

    SWD_WRITE_BIT(val);
    val >>= 1;
    n--;
  }
}

// ===================================================================================
// Generate SWD Sequence
//   info:   sequence information
//   swdo:   pointer to SWDIO generated data
//   swdi:   pointer to SWDIO captured data
//   return: none
// ===================================================================================
void SWD_Sequence(uint8_t info, const __xdata uint8_t *swdo, __xdata uint8_t *swdi) {
  uint8_t val;
  uint8_t bits;
  uint8_t n, k;

  n = info & SWD_SEQUENCE_CLK;
  if(n == 0U) n = 64U;

  if(info & SWD_SEQUENCE_DIN) {
    while(n) {
      val = 0U;
      for (k = 8U; k && n; k--, n--) {
        SWD_READ_BIT(bits);
        val >>= 1;
        val |= bits << 7;
      }
      val >>= k;
      *swdi++ = (uint8_t)val;
    }
  }
  else {
    while(n) {
      val = *swdo++;
      for(k = 8U; k && n; k--, n--) {
        SWD_WRITE_BIT(val);
        val >>= 1;
      }
    }
  }
}

// ===================================================================================
// SWD Transfer I/O
//   request: A[3:2] RnW APnDP
//   data:    DATA[31:0]
//   return:  ACK[2:0]
// ===================================================================================
__idata uint8_t idle_cycles;
uint8_t SWD_Transfer(uint8_t req, __xdata uint8_t *data) {
  uint8_t ack;
  uint8_t bits = req;
  uint8_t val;
  uint8_t parity;
  uint8_t m, n;

  // Packet req
  parity = 0U;
  SWD_WRITE_BIT(1U);              // start bit
  SWD_WRITE_BIT(bits);            // APnDP bit
  parity += bits;
  bits >>= 1;
  SWD_WRITE_BIT(bits);            // RnW bit
  parity += bits;
  bits >>= 1;
  SWD_WRITE_BIT(bits);            // A2 bit
  parity += bits;
  bits >>= 1;
  SWD_WRITE_BIT(bits);            // A3 bit
  parity += bits;
  SWD_WRITE_BIT(parity);          // parity bit
  SWD_WRITE_BIT(0U);              // stop bit
  SWD_WRITE_BIT(1U);              // park bit

  // Turnaround is always 1 cycle
  SWD_OUT_DISABLE();
  SWD_CLOCK_CYCLE();

  // Acknowledge res
  SWD_READ_BIT(bits);
  ack = bits << 0;
  SWD_READ_BIT(bits);
  ack |= bits << 1;
  SWD_READ_BIT(bits);
  ack |= bits << 2;

  if(ack == DAP_TRANSFER_OK) {
    // OK res
    // Data transfer
    if(req & DAP_TRANSFER_RnW) {  // read data
      val = 0U;
      parity = 0U;
      for(m = 0; m < 4; m++) {
        for(n = 8U; n; n--) {
          SWD_READ_BIT(bits);     // read RDATA[0:31]
          parity += bits;
          val >>= 1;
          if(bits) val |= 0x80;
         }
         if(data) data[m] = val;
      }
      SWD_READ_BIT(bits);         // read parity
      if((parity ^ bits) & 1U)
        ack = DAP_TRANSFER_ERROR;

      // Turnaround
      SWD_CLOCK_CYCLE();
      SWD_OUT_ENABLE();
    }
    else {                        // write data
      // Turnaround 
      SWD_CLOCK_CYCLE();
      SWD_OUT_ENABLE();

      // Write WDATA[0:31]
      parity = 0U;
      for(m = 0; m < 4; m++) {
        val = data[m];
        ACC = val;
        if(P) parity++;
        for(n = 8U; n; n--) {
          SWD_WRITE_BIT(val);
          val >>= 1;
        }
      }
      SWD_WRITE_BIT(parity);      // write parity bit
    }
    // Idle cycles
    SWD_SET(0);
    n = idle_cycles;
    if(n) {
      for(; n; n--) SWD_CLOCK_CYCLE();
    }
    SWD_SET(1);
    return(ack);
  }

  if((ack == DAP_TRANSFER_WAIT) || (ack == DAP_TRANSFER_FAULT)) {
    // Turnaround
    SWD_CLOCK_CYCLE();
    SWD_OUT_ENABLE();
    SWD_SET(1);
    return(ack);
  }

  // Protocol error - clock out 32bits + parity + turnaround
  n = 32U + 1U + 1U;
  do {
    SWD_CLOCK_CYCLE();            // back off data phase
  } while(--n);

  SWD_OUT_ENABLE();
  SWD_SET(1);
  return(ack);
}

// ===================================================================================
// Get DAP Information
//   id:      info identifier
//   info:    pointer to info data
//   return:  number of bytes in info data
// ===================================================================================
static uint8_t DAP_Info(uint8_t id, __xdata uint8_t *info) {
  uint8_t length = 0U;
  switch(id) {
    case DAP_ID_FW_VER:
      length = (uint8_t)sizeof(DAP_FW_VER);
      strcpy(info, DAP_FW_VER );
      break;
    case DAP_ID_CAPABILITIES:
      info[0] = DAP_PORT_SWD;
      length = 1U;
      break;
    case DAP_ID_PACKET_SIZE:
      info[0] = DAP_PACKET_SIZE;
      info[1] = 0;
      length = 2U;
      break;
    case DAP_ID_PACKET_COUNT:
      info[0] = DAP_PACKET_COUNT;
      length = 1U;
      break;
    default:
      break;
  }
  return length;
}

// ===================================================================================
// Process Host Status AKA DAP_LED command and prepare response
//   request:  pointer to request data
//   response: pointer to response data
//   return:   number of bytes in response
// ===================================================================================
static uint8_t DAP_HostStatus(const __xdata uint8_t *req, __xdata uint8_t *res) {
  *res = DAP_OK;
  switch(*req) {
    case DAP_DEBUGGER_CONNECTED:
      //LED_SET(*(req + 1) & 1U);
      break;
    case DAP_TARGET_RUNNING:
      //LED_SET(*(req + 1) & 1U);
      break;
    default:
      *res = DAP_ERROR;
  }
  return 1;
}

// ===================================================================================
// Process Connect command and prepare response
//   request:  pointer to request data
//   response: pointer to response data
//   return:   number of bytes in response
// ===================================================================================
__idata uint8_t debug_port;
static uint8_t DAP_Connect(const __xdata uint8_t *req, __xdata uint8_t *res) {
  uint8_t port;
  if(*req == DAP_PORT_AUTODETECT) port = DAP_DEFAULT_PORT;
  else port = *req;

  switch(port) {
    case DAP_PORT_SWD:
      debug_port = DAP_PORT_SWD;
      PORT_SWD_CONNECT();
      break;
    default:
      port = DAP_PORT_DISABLED;
      break;
  }

  *res = port;
  return 1;
}

// ===================================================================================
// Process Disconnect command and prepare response
//   response: pointer to response data
//   return:   number of bytes in response
// ===================================================================================
static uint8_t DAP_Disconnect(__xdata uint8_t *res) {
  *res = DAP_OK;
  debug_port = DAP_PORT_DISABLED;
  PORT_SWD_DISCONNECT();
  return 1;
}

// ===================================================================================
// Process SWJ Pins command and prepare response
//   request:  pointer to request data
//   response: pointer to response data
//   return:   number of bytes in response
// ===================================================================================
static uint8_t DAP_SWJ_Pins(const __xdata uint8_t *req, __xdata uint8_t *res) {
  uint8_t value;
  uint8_t select;
  uint16_t wait;

  value = * (req + 0);
  select = (uint8_t) * (req + 1);
  wait = (uint16_t)(*(req + 2) << 0) | (uint16_t)(*(req + 3) << 8);
  if((uint8_t)(*(req + 4)) || (uint8_t)(*(req + 5))) wait |= 0x8000;

  if((select & DAP_SWJ_SWCLK_TCK_BIT) != 0U) {
    ((value & DAP_SWJ_SWCLK_TCK_BIT) != 0U) ? (SWK_SET(1)) : (SWK_SET(0));
  }
  if((select & DAP_SWJ_SWDIO_TMS_BIT) != 0U) {
    ((value & DAP_SWJ_SWDIO_TMS_BIT) != 0U) ? (SWD_SET(1)) : (SWD_SET(0));
  }
  if((select & DAP_SWJ_nRESET_BIT) != 0U) {
    RST_SET(value >> DAP_SWJ_nRESET);
  }

  if(wait != 0U) {
    do {
      if((select & DAP_SWJ_SWCLK_TCK_BIT) != 0U) {
        if((value >> DAP_SWJ_SWCLK_TCK) ^ (SWK_GET())) continue;
      }
      if((select & DAP_SWJ_SWDIO_TMS_BIT) != 0U) {
        if((value >> DAP_SWJ_SWDIO_TMS) ^ (SWD_GET())) continue;
      }
      if((select & DAP_SWJ_nRESET_BIT) != 0U) {
        if((value >> DAP_SWJ_nRESET) ^ (RST_GET())) continue;
      }
      break;
    } while(wait--);
  }

  value = ((uint8_t)SWK_GET() << DAP_SWJ_SWCLK_TCK)
        | ((uint8_t)SWD_GET() << DAP_SWJ_SWDIO_TMS)
        | (0 << DAP_SWJ_TDI)
        | (0 << DAP_SWJ_TDO)
        | (0 << DAP_SWJ_nTRST)
        | ((uint8_t)RST_GET() << DAP_SWJ_nRESET);
  *res = value;
  return 1;
}

// ===================================================================================
// Process SWJ Sequence command and prepare response
//   request:  pointer to request data
//   response: pointer to response data
//   return:   number of bytes in response
// ===================================================================================
static uint8_t DAP_SWJ_Sequence(const __xdata uint8_t *req, __xdata uint8_t *res) {
  uint8_t count;
  count = *req++;
  if(count == 0U) count = 255U;
  SWJ_Sequence(count, req);
  *res = DAP_OK;
  return 1;
}

// ===================================================================================
// Process SWD Sequence command and prepare response
//   request:  pointer to request data
//   response: pointer to response data
//   return:   number of bytes in response
// ===================================================================================
static uint8_t DAP_SWD_Sequence(const __xdata uint8_t *req, __xdata uint8_t *res) {
  uint8_t sequence_info;
  uint8_t sequence_count;
  uint8_t request_count;
  uint8_t response_count;
  uint8_t count;

  *res++ = DAP_OK;
  request_count = 1U;
  response_count = 1U;
  sequence_count = *req++;

  while(sequence_count--) {
    sequence_info = *req++;
    count = sequence_info & SWD_SEQUENCE_CLK;
    if(count == 0U) count = 64U;
    count = (count + 7U) / 8U;
    ((sequence_info & SWD_SEQUENCE_DIN) != 0U) ? (SWD_SET(1)) : (SWD_SET(1));
    SWD_Sequence(sequence_info, req, res);
    if(sequence_count == 0U) SWD_SET(1);
    if((sequence_info & SWD_SEQUENCE_DIN) != 0U) {
      request_count++;
      res += count;
      response_count += count;
    }
    else {
      req += count;
      request_count += count + 1U;
    }
  }
  return response_count;
}

// ===================================================================================
// Process Transfer Configure command and prepare response
//   request:  pointer to request data
//   response: pointer to response data
//   return:   number of bytes in response
// ===================================================================================
__idata uint16_t retry_count;
__idata uint16_t match_retry;
static uint8_t DAP_TransferConfigure(const __xdata uint8_t *req, __xdata uint8_t *res) {
  idle_cycles = *(req + 0);
  retry_count = (uint16_t) * (req + 1)
              | (uint16_t)(*(req + 2) << 8);
  match_retry = (uint16_t) * (req + 3)
              | (uint16_t)(*(req + 4) << 8);
  *res = DAP_OK;
  return 1;
}

// ===================================================================================
// Process SWD Transfer command and prepare response
//   request:  pointer to request data
//   response: pointer to response data
//   return:   number of bytes in response
// ===================================================================================
__xdata uint8_t data[4];
__idata uint8_t match_mask[4];
__idata uint8_t match_value[4];
__idata uint8_t DAP_TransferAbort = 0U;
__idata uint8_t request_count;
__idata uint8_t request_value;
__idata uint8_t response_count;
__idata uint8_t response_value;
__idata uint16_t retry;
static uint8_t DAP_SWD_Transfer(const __xdata uint8_t *req, __xdata uint8_t *res) {
  const __xdata uint8_t *request_head;
  __xdata uint8_t *response_head;
  uint8_t post_read;
  uint8_t check_write;

  request_head = req;
  response_count = 0U;
  response_value = 0U;
  response_head = res;
  res += 2;
  DAP_TransferAbort = 0U;
  post_read = 0U;
  check_write = 0U;

  req++;                                          // ignore DAP index
  request_count = *req++;
  for(; request_count != 0U; request_count--) {
    request_value = *req++;

    // RnW == 1 for read, 0 for write
    if(request_value & DAP_TRANSFER_RnW) {
      // Read registers
      if(post_read) {
        // Read was posted before
        retry = retry_count;
        if((request_value & (DAP_TRANSFER_APnDP | DAP_TRANSFER_MATCH_VALUE)) == DAP_TRANSFER_APnDP) {
          // Read previous AP data and post next AP read
          do {
            response_value = SWD_Transfer(request_value, data);
          } while((response_value == DAP_TRANSFER_WAIT) && retry-- && !DAP_TransferAbort);
        }
        else {
          // Read previous AP data
          do {
            response_value = SWD_Transfer(DP_RDBUFF | DAP_TRANSFER_RnW, data);
          } while((response_value == DAP_TRANSFER_WAIT) && retry-- && !DAP_TransferAbort);
          post_read = 0U;
        }
        if(response_value != DAP_TRANSFER_OK) break;

        // Store previous AP data
        *res++ = (uint8_t)data[0];
        *res++ = (uint8_t)data[1];
        *res++ = (uint8_t)data[2];
        *res++ = (uint8_t)data[3];
      }
      if((request_value & DAP_TRANSFER_MATCH_VALUE) != 0U) {
        // Read with value match
        match_value[0] = (uint8_t)(*(req + 0));
        match_value[1] = (uint8_t)(*(req + 1));
        match_value[2] = (uint8_t)(*(req + 2));
        match_value[3] = (uint8_t)(*(req + 3));
        req += 4;
        match_retry = match_retry;
        if((request_value & DAP_TRANSFER_APnDP) != 0U) {
          // Post AP read
          retry = retry_count;
          do {
            response_value = SWD_Transfer(request_value, NULL);
          } while((response_value == DAP_TRANSFER_WAIT) && retry-- && !DAP_TransferAbort);
          if(response_value != DAP_TRANSFER_OK) break;
        }
        do {
          // Read registers until its value matches or retry counter expires
          retry = retry_count;
          do {
            response_value = SWD_Transfer(request_value, data);
          } while((response_value == DAP_TRANSFER_WAIT) && retry-- && !DAP_TransferAbort);
          if(response_value != DAP_TRANSFER_OK) break;
        }
        while(((data[0] & match_mask[0]) != match_value[0] ||
               (data[1] & match_mask[1]) != match_value[1] ||
               (data[2] & match_mask[2]) != match_value[2] ||
               (data[3] & match_mask[3]) != match_value[3]) &&
                match_retry-- && !DAP_TransferAbort);
        if((data[0] & match_mask[0]) != match_value[0] ||
           (data[1] & match_mask[1]) != match_value[1] ||
           (data[2] & match_mask[2]) != match_value[2] ||
           (data[3] & match_mask[3]) != match_value[3])
               response_value |= DAP_TRANSFER_MISMATCH;
        if(response_value != DAP_TRANSFER_OK) break;
      }
      else {
        // Normal read
        retry = retry_count;
        if((request_value & DAP_TRANSFER_APnDP) != 0U) {
          // Read AP registers
          if(post_read == 0U) {
            // Post AP read
            do {
              response_value = SWD_Transfer(request_value, NULL);
            } while((response_value == DAP_TRANSFER_WAIT) && retry-- && !DAP_TransferAbort);
            if(response_value != DAP_TRANSFER_OK) break;
            post_read = 1U;
          }
        }
        else {
          // Read DP register
          do {
            response_value = SWD_Transfer(request_value, data);
          } while((response_value == DAP_TRANSFER_WAIT) && retry-- && !DAP_TransferAbort);
          if(response_value != DAP_TRANSFER_OK) break;

          // Store data
          *res++ = data[0];
          *res++ = data[1];
          *res++ = data[2];
          *res++ = data[3];
        }
      }
      check_write = 0U;
    }
    else {                                        // RnW == 0
      // Write register
      if(post_read) {
        // Read previous data
        retry = retry_count;
        do {
          response_value = SWD_Transfer(DP_RDBUFF | DAP_TRANSFER_RnW, data);
        } while((response_value == DAP_TRANSFER_WAIT) && retry-- && !DAP_TransferAbort);
        if(response_value != DAP_TRANSFER_OK) break;

        // Store previous data
        *res++ = data[0];
        *res++ = data[1];
        *res++ = data[2];
        *res++ = data[3];
        post_read = 0U;
      }
      // Load data
      data[0] = (uint8_t)(*(req + 0));
      data[1] = (uint8_t)(*(req + 1));
      data[2] = (uint8_t)(*(req + 2));
      data[3] = (uint8_t)(*(req + 3));
      req += 4;
      if((request_value & DAP_TRANSFER_MATCH_MASK) != 0U) {
        // Write match mask
        match_mask[0] = data[0];
        match_mask[1] = data[1];
        match_mask[2] = data[2];
        match_mask[3] = data[3];
        response_value = DAP_TRANSFER_OK;
      }
      else {
        // Write DP/AP register
        retry = retry_count;
        do {
          response_value = SWD_Transfer(request_value, data);
        } while((response_value == DAP_TRANSFER_WAIT) && retry-- && !DAP_TransferAbort);
        if(response_value != DAP_TRANSFER_OK) break;
        check_write = 1U;
      }
    }
    response_count++;
    if(DAP_TransferAbort) break;
  }

  for(; request_count != 0U; request_count--) {
    // Process canceled requests
    request_value = *req++;
    if((request_value & DAP_TRANSFER_RnW) != 0U) {  // Read register
      if((request_value & DAP_TRANSFER_MATCH_VALUE) != 0U) req += 4;
    }
    else req += 4;                                  // Write register
  }

  if(response_value == DAP_TRANSFER_OK) {
    if(post_read) {
      // Read previous data
      retry = retry_count;
      do {
        response_value = SWD_Transfer(DP_RDBUFF | DAP_TRANSFER_RnW, data);
      } while((response_value == DAP_TRANSFER_WAIT) && retry-- && !DAP_TransferAbort);
      if(response_value != DAP_TRANSFER_OK) goto end;

      // Store previous data
      *res++ = (uint8_t)data[0];
      *res++ = (uint8_t)data[1];
      *res++ = (uint8_t)data[2];
      *res++ = (uint8_t)data[3];
    }
    else if(check_write) {
      // Check last write
      retry = retry_count;
      do {
        response_value = SWD_Transfer(DP_RDBUFF | DAP_TRANSFER_RnW, NULL);
      } while((response_value == DAP_TRANSFER_WAIT) && retry-- && !DAP_TransferAbort);
    }
  }

end:
  *(response_head + 0) = (uint8_t)response_count;
  *(response_head + 1) = (uint8_t)response_value;
  return((uint8_t)(res - response_head));
}

// ===================================================================================
// Process SWD Transfer Block command and prepare response
//   request:  pointer to request data
//   response: pointer to response data
//   return:   number of bytes in response
// ===================================================================================
static uint8_t DAP_SWD_TransferBlock(const __xdata uint8_t *req, __xdata uint8_t *res) {
  __xdata uint8_t *response_head;
  response_count = 0U;
  response_value = 0U;
  response_head = res;
  res += 3;
  DAP_TransferAbort = 0U;
  req++;                                          // ignore DAP index
  request_count = (uint16_t)(*(req + 0) << 0)
                | (uint16_t)(*(req + 1) << 8);
  req += 2;
  if(request_count == 0U) goto end;

  request_value = *req++;
  if((request_value & DAP_TRANSFER_RnW) != 0U) {
    // Read register block
    if((request_value & DAP_TRANSFER_APnDP) != 0U) {
      // Post AP read
      retry = retry_count;
      do {
        response_value = SWD_Transfer(request_value, NULL);
      } while((response_value == DAP_TRANSFER_WAIT) && retry-- && !DAP_TransferAbort);
      if(response_value != DAP_TRANSFER_OK) goto end;
    }
    while(request_count--) {
      // Read DP/AP register
      if((request_count == 0U) && ((request_value & DAP_TRANSFER_APnDP) != 0U))
        request_value = DP_RDBUFF | DAP_TRANSFER_RnW;   // Last AP read
      retry = retry_count;
      do {
        response_value = SWD_Transfer(request_value, data);
      } while((response_value == DAP_TRANSFER_WAIT) && retry-- && !DAP_TransferAbort);
      if(response_value != DAP_TRANSFER_OK) goto end;

      // Store data
      *res++ = (uint8_t)data[0];
      *res++ = (uint8_t)data[1];
      *res++ = (uint8_t)data[2];
      *res++ = (uint8_t)data[3];
      response_count++;
    }
  }
  else {
    // Write register block
    while(request_count--) {
      // Load data
      data[0] = (uint8_t)(*(req + 0));
      data[1] = (uint8_t)(*(req + 1));
      data[2] = (uint8_t)(*(req + 2));
      data[3] = (uint8_t)(*(req + 3));
      req += 4;

      // Write DP/AP register
      retry = retry_count;
      do {
        response_value = SWD_Transfer(request_value, data);
      } while((response_value == DAP_TRANSFER_WAIT) && retry-- && !DAP_TransferAbort);
      if(response_value != DAP_TRANSFER_OK) goto end;
      response_count++;
    }
    // Check last write
    retry = retry_count;
    do {
      response_value = SWD_Transfer(DP_RDBUFF | DAP_TRANSFER_RnW, NULL);
    } while((response_value == DAP_TRANSFER_WAIT) && retry-- && !DAP_TransferAbort);
  }

end:
  *(response_head + 0) = response_count;
  *(response_head + 1) = 0; 
  *(response_head + 2) = response_value;
  return((uint8_t)(res - response_head));
}

// ===================================================================================
// Process SWD Transfer Block command and prepare response
//   request:  pointer to request data
//   response: pointer to response data
//   return:   number of bytes in response
// ===================================================================================
static uint8_t DAP_TransferBlock(const __xdata uint8_t *req, __xdata uint8_t *res) {
  uint8_t num;

  switch(debug_port) {
    case DAP_PORT_SWD:
      num = DAP_SWD_TransferBlock(req, res);
      break;
    default:
      *(res + 0) = 0U;    // res count [7:0]
      *(res + 1) = 0U;    // res count[15:8]
      *(res + 2) = 0U;    // res value
      num = 3U;
      break;
  }
  return num;
}

// ===================================================================================
// DAP Thread.
// ===================================================================================
uint8_t DAP_Thread(void) {
  uint8_t num;
  uint8_t __xdata *req = DAP_READ_BUF_PTR;
  uint8_t __xdata *res = DAP_WRITE_BUF_PTR;

  *res++ = *req;
  switch(*req++) {
    case ID_DAP_Transfer:
      num = DAP_SWD_Transfer(req, res);
      break;
    case ID_DAP_TransferBlock:
      num = DAP_TransferBlock(req, res);
      break;
    case ID_DAP_Info:
      num = DAP_Info(*req, res + 1);
      *res = (uint8_t)num;
      num++;
      break;
    case ID_DAP_HostStatus:
      num = DAP_HostStatus(req, res);
      break;
    case ID_DAP_Connect:
      num = DAP_Connect(req, res);
      break;
    case ID_DAP_Disconnect:
      num = DAP_Disconnect(res);
      break;
    case ID_DAP_SWJ_Pins:
      num = DAP_SWJ_Pins(req, res);
      break;
    case ID_DAP_SWJ_Clock:
      *res = DAP_OK;
      num = 1;
      break;
    case ID_DAP_SWJ_Sequence:
      num = DAP_SWJ_Sequence(req, res);
      break;
    case ID_DAP_SWD_Configure:
      *res = DAP_OK;
      num = 1;
      break;
    case ID_DAP_SWD_Sequence:
      num = DAP_SWD_Sequence(req, res);
      break;
    case ID_DAP_TransferConfigure:
      num = DAP_TransferConfigure(req, res);
      break;
    case ID_DAP_WriteABORT:
      *res = DAP_OK;
      num = 1;
      break;
    case ID_DAP_ExecuteCommands:
    case ID_DAP_QueueCommands:
    default:
      *(res - 1) = ID_DAP_Invalid;
      num = 1;
      break;
  }
  return(num + 1);
}
