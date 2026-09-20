/**
  ******************************************************************************
  * @file    mx_usbx_device_audio.h
  * @brief   USBX Device Audio applicative header file
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
#ifndef MX_USBX_DEVICE_AUDIO_H
#define MX_USBX_DEVICE_AUDIO_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/
#include "ux_api.h"
#include "ux_device_class_audio.h"
/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
#ifndef USBD_AUDIO_STREAM_NUMBER
#define USBD_AUDIO_STREAM_NUMBER  2
#endif
/* Exported macros -----------------------------------------------------------*/
/* Exported functions prototypes -------------------------------------------- */
VOID usbd_audio_activate(VOID *audio_instance);
VOID usbd_audio_deactivate(VOID *audio_instance);
UINT usbd_audio_control_process(UX_DEVICE_CLASS_AUDIO *audio_instance,
                               UX_SLAVE_TRANSFER *transfer);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* MX_USBX_DEVICE_AUDIO_H */