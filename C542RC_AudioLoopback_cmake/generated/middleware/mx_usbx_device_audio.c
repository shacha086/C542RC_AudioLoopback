/**
  ******************************************************************************
  * @file    mx_usbx_device_audio.c
  * @brief   USBX Device Audio applicative source file
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

/* Includes ------------------------------------------------------------------*/
#include "mx_usbx_device_audio.h"
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
UX_DEVICE_CLASS_AUDIO                  *audio;
UX_DEVICE_CLASS_AUDIO_STREAM           *stream_read;
UX_DEVICE_CLASS_AUDIO_STREAM           *stream_write;
/* Private functions prototype -----------------------------------------------*/

/**
  * @brief  usbd_audio_activate
  *         This function is called when insertion of an Audio device.
  * @param  audio_instance: Pointer to the audio class instance.
  * @retval none
  */
VOID usbd_audio_activate(VOID *audio_instance)
{
  /* Save the Audio instance.  */
  audio = (UX_DEVICE_CLASS_AUDIO *)audio_instance;

  /* Get the streams instances.  */
  ux_device_class_audio_stream_get(audio, 0, &stream_read);
  ux_device_class_audio_stream_get(audio, 0, &stream_write);

  return;
}

/**
  * @brief  usbd_audio_deactivate
  *         This function is called when extraction of an Audio device.
  * @param  audio_instance: Pointer to the audio class instance.
  * @retval none
  */
VOID usbd_audio_deactivate(VOID *audio_instance)
{
  /* Reset the Audio instance.  */
  audio = UX_NULL;

  /* Reset the Audio streams.  */
  stream_read = UX_NULL;
  stream_write = UX_NULL;

  return;
}

/**
  * @brief  usbd_audio_control_process
  *         This function is invoked to manage the UAC class requests.
  * @param  audio_instance: Pointer to Audio class stream instance.
  * @param  transfer: Pointer to the transfer request.
  * @retval status
  */
UINT usbd_audio_control_process(UX_DEVICE_CLASS_AUDIO *audio_instance,
                               UX_SLAVE_TRANSFER *transfer)
{
  UINT status  = UX_SUCCESS;
  return status;
}
