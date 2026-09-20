/**
  ******************************************************************************
  * @file    ux_stm32_device_descriptors_config.h
  * @brief   Configuration for STM32 USBX device descriptors
  *
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025-2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the mx_usbx_license.md file
  * in the same directory as the generated code.
  * If no mx_usbx_license.md file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef UX_STM32_DEVICE_DESCRIPTORS_CONFIG_H
#define UX_STM32_DEVICE_DESCRIPTORS_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define USBD_HIGH_SPEED_SUPPORTED               0U
#define USBD_COMPOSITE_USE_IAD                  1U
#define USBD_HID_MOUSE_ACTIVATED                0U
#define USBD_HID_KEYBOARD_ACTIVATED             0U
#define USBD_HID_CUSTOM_ACTIVATED               0U
#define USBD_CDC_ACM_CLASS_ACTIVATED            0U
#define USBD_MSC_CLASS_ACTIVATED                0U
#define USBD_DFU_CLASS_ACTIVATED                0U
#define USBD_AUDIO_CLASS_ACTIVATED              1U
#define USBD_VIDEO_CLASS_ACTIVATED              0U
#define USBD_PRINTER_CLASS_ACTIVATED            0U
#define USBD_CCID_CLASS_ACTIVATED               0U
#define USBD_MTP_CLASS_ACTIVATED                0U
#define USBD_VENDOR_ID                          1155
#define USBD_PRODUCT_ID                         22288
#define USBD_LANG_ID_STRING                     1033
#define USBD_MANUFACTURER_STRING                "STMicroelectronics"
#define USBD_PRODUCT_STRING                     "STM32 USB Audio Loopback"
#define USBD_SERIAL_NUMBER_STRING               "000000000001"
#define USBD_MAX_POWER                          50
/* Fixed full-speed format: 48 kHz, stereo, signed 16-bit PCM. */
#define USBD_AUDIO_SAMPLE_RATE                  48000U

/* The two-stream audio framework is larger than the library's 256-byte
   default descriptor storage. */
#define USBD_FRAMEWORK_FS_MAX_LENGTH            512U
#define USBD_FRAMEWORK_HS_MAX_LENGTH            512U

/* Build USB Audio Class 1.0 descriptors for broad native-host support. */
#define USBD_AUDIO_USE_UAC1_BUILDER             1U

/* Endpoint IN for AUDIO. EP2 is separate from double-buffered OUT EP1. */
#define USBD_AUDIO_EPIN_ADDR                    0x82U
#define USBD_AUDIO_EPIN_TYPE                    UX_ISOCHRONOUS_ENDPOINT
#define USBD_AUDIO_EPIN_FS_MPS                  192
#define USBD_AUDIO_EPIN_FS_BINTERVAL            1
#if USBD_HIGH_SPEED_SUPPORTED == 1U
#define USBD_AUDIO_EPIN_HS_MPS                  512
#define USBD_AUDIO_EPIN_HS_BINTERVAL            1
#endif /* USBD_HIGH_SPEED_SUPPORTED */
/* Endpoint OUT for AUDIO. 0x0D selects synchronous isochronous data so one
   full-speed USB frame always represents exactly 48 samples. */
#define USBD_AUDIO_EPOUT_ADDR                   0x01U
#define USBD_AUDIO_EPOUT_TYPE                   UX_ISOCHRONOUS_ENDPOINT
#define USBD_AUDIO_EPOUT_DATA_TYPE              0x0DU
#define USBD_AUDIO_EPOUT_FS_MPS                 192
#define USBD_AUDIO_EPOUT_FS_BINTERVAL           1
#if USBD_HIGH_SPEED_SUPPORTED == 1U
#define USBD_AUDIO_EPOUT_HS_MPS                 512
#define USBD_AUDIO_EPOUT_HS_BINTERVAL           1
#endif /* USBD_HIGH_SPEED_SUPPORTED */

#ifdef __cplusplus
}
#endif
#endif  /* UX_STM32_DEVICE_DESCRIPTORS_CONFIG_H */
