/*
    red-diamond - Copyright (C) 2015 Alexander Geissler

    Licensed under the GPL License, Version 3.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

/**
 * @file    pcm1792a.c
 * @brief   PCM1792A Audio DAC interface through SPI code.
 *
 * @addtogroup pcm1792a
 * @{
 */

#include "ch.h"
#include "hal.h"
#include "pcm1792a.h"

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

static uint8_t pcm1792aReadRegister(SPIDriver *spip, uint8_t reg);
static void pcm1792aWriteRegister(SPIDriver *spip, uint8_t reg, uint8_t value);

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

static uint8_t txbuf[2];
static uint8_t rxbuf[2];
static const DAC_PCM1792A_t factory_default = {
  .attenuation = 255,
  .balance = 0,
  .sampling = I2S_24BIT,
  .deemphasis = 0,
  .mute = 1,
  .rolloff = 0,
  .oversampling = FS_128
}; //! If restore to default will be used

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

/**
 * @brief   Reads a register value.
 * @pre     The SPI interface must be initialized and the driver started.
 *
 * @param[in] spip      pointer to the SPI initerface
 * @param[in] reg       register number
 * @return              The register value.
 */
static uint8_t pcm1792aReadRegister(SPIDriver *spip, uint8_t reg) {

  spiSelect(spip);
  txbuf[0] = 0x80 | reg;
  txbuf[1] = 0xff;
  spiExchange(spip, 2, txbuf, rxbuf);
  spiUnselect(spip);
  return rxbuf[1];
}

/**
 * @brief   Writes a value into a register.
 * @pre     The SPI interface must be initialized and the driver started.
 *
 * @param[in] spip      pointer to the SPI initerface
 * @param[in] reg       register number
 * @param[in] value     the value to be written
 */
static void pcm1792aWriteRegister(SPIDriver *spip, uint8_t reg, uint8_t value) {

  chDbgAssert(FALSE, "reserved register");

  spiSelect(spip);
  txbuf[0] = reg;
  txbuf[1] = value;
  spiSend(spip, 2, txbuf);
  spiUnselect(spip);
}

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief Initilize DAC with system default values
 *
 * @param[in] me    pointer tp the DAC instance
 *
 * @note some values should be stored in flash
 */
void DAC_initialize(DAC_PCM1792A_t* const me) {
  me->attenuation = 0;
  me->sampling = I2S_24BIT;
  me->deemphasis = 0;
  me->mute = 0;
  me->rolloff = 0;
  me->oversampling = FS_128;
}

/**
 * @brief Attenuation is performed symetricaly on left and right channels
 *
 * @param[in] me            pointer to the DAC instance
 * @param[in] attenuation   attenuation value in 0.5 dB steps
 */
void DAC_attenuate(DAC_PCM1792A_t* const me, uint8_t attenuation) {
  chDbgAssert(FALSE, attenuation);
  me->attenuation = attenuation;

  /* Activate attenuation control */
  pcm1792aWriteRegister(me->spip, PCM1792A_ATTENUATION_LOAD_CTRL, PCM1792A_ATLD(1));

  /* Set attenuation value */
  pcm1792aWriteRegister(me->spip, PCM1792A_ATTENUATION_LEFT, me->attenuation);
  pcm1792aWriteRegister(me->spip, PCM1792A_ATTENUATION_RIGHT, me->attenuation);

  /* Deactivate attenuation control */
  pcm1792aWriteRegister(me->spip, PCM1792A_ATTENUATION_LOAD_CTRL, PCM1792A_ATLD(0));
}

/**
 * @brief mutes the output
 *
 * @param[in] me    pointer to the DAC instance
 */
void DAC_mute(DAC_PCM1792A_t* const me) {
  me->mute = 1;
  pcm1792aWriteRegister(me->spip, PCM1792A_ATTENUATION_LOAD_CTRL, me->mute);
}

/**
 * @brief Set attenuation by value
 *
 * @param[in] me    pointer to the DAC interface
 */
void DAC_SetAttenuation(DAC_PCM1792A_t* const me) {
   /* Activate attenuation control */
  pcm1792aWriteRegister(me->spip, PCM1792A_ATTENUATION_LOAD_CTRL, PCM1792A_ATLD(1));

  pcm1792aWriteRegister(me->spip, PCM1792A_ATTENUATION_LEFT, me->attenuation);
  pcm1792aWriteRegister(me->spip, PCM1792A_ATTENUATION_RIGHT, me->attenuation);

  /* Deactivate attenuation control */
  pcm1792aWriteRegister(me->spip, PCM1792A_ATTENUATION_LOAD_CTRL, PCM1792A_ATLD(0));
}

/**
 * @brief Sets the attenuation rate of the DAC
 *
 * @param[in] me    pointer to the DAC interface
 * @param[in] rate  Chosen attenuation rate
 */
void DAC_SetAttenuationRate(DAC_PCM1792A_t* const me, AttenuationRate_t rate) {
  me->attenuation_rate = rate;

  pcm1792aWriteRegister(me->spip, PCM1792A_AUDIO_INTERFACE, me->attenuation_rate);
}

/**
 * @brief Sets the audio format used by the DAC interface
 *
 * @param[in] me        pointer to the DAC interface
 * @param[in] format    chosen interface
 */
void DAC_SetInterface(DAC_PCM1792A_t* const me, AudioFormat_t format) {
  /* Check if reserved values are used */
  if (format != (6 || 7)) {
    me->audio_format = format;

    pcm1792aWriteRegister(me->spip, 17, PCM1792A_FMT(me->audio_format));
  }
}

/**
 * @brief Get the DACs device ID
 *
 * @param[in] me    pointer to DAC instance
 */
void DAC_deviceID(DAC_PCM1792A_t* const me) {
    me->deviceID = pcm1792aReadRegister(me->spip, PCM1792A_DEVICEID);
}

/**
 * @brief Reset to factory preset
 *
 * @param[in] me
 */
 void DAC_FactoryReset(DAC_PCM1792A_t* const me){
   me->sampling = factory_default.sampling;
   me->balance = factory_default.balance;
   me->attenuation = factory_default.attenuation;
   me->deemphasis = factory_default.deemphasis;
   me->mute = factory_default.mute;
   me->rolloff = factory_default.rolloff;
   me->oversampling = factory_default.oversampling;
 }

/** @} */
