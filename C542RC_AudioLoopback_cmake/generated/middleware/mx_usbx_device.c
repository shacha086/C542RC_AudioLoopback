/**
  ******************************************************************************
  * @file    mx_usbx_device.c
  * @brief   USBX Device applicative source file
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
#include "mx_usbx_device.h"
#include "ux_dcd_stm32.h"
#include "mx_usbx_device_audio.h"
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
hal_pcd_handle_t *p_usb_device = UX_NULL;
/**
  * @brief  Application USBX Device Initialization.
  */
UINT app_usbx_device_init(VOID)
{
  UINT status;
  USB_DESCRIPTOR usbd_desc;
  UX_DEVICE_CLASS_AUDIO_PARAMETER audio_parameter = {0};

  status = usb_device_descriptor_register_class(USBD_CLASS_TYPE_AUDIO_10,
                                                 audio_interface,
                                                 UX_NULL);
  if (status != USBD_DESCRIPTOR_SUCCESS)
  {
    return status;
  }

  status = usb_device_descriptor_get_framework(&usbd_desc);
  if (status != USBD_DESCRIPTOR_SUCCESS)
  {
    return status;
  }

  /* Install the device portion of USBX */
  status = ux_device_stack_initialize(usbd_desc.device_high_speed.framework,
                                      usbd_desc.device_high_speed.framework_length,
                                      usbd_desc.device_full_speed.framework,
                                      usbd_desc.device_full_speed.framework_length,
                                      usbd_desc.string.framework,
                                      usbd_desc.string.framework_length,
                                      usbd_desc.languageid.framework,
                                      usbd_desc.languageid.framework_length,
                                      UX_NULL);

  if (status != UX_SUCCESS)
  {
    return status;
  }
  /* Configure both AudioStreaming interfaces and their callbacks. */
  usbd_audio_get_parameters(&audio_parameter);

  /* Initialize the device audio class */
  status = ux_device_stack_class_register(_ux_system_slave_class_audio_name,
                                          ux_device_class_audio_entry,
                                          0x01,
                                          audio_interface->interface_numbers,
                                          &audio_parameter);

  if (status != UX_SUCCESS)
  {
    return status;
  }
  return UX_SUCCESS;
}

/**
  * @brief  Application USBX Device De-Initialization.
  * @retval none
  */
UINT app_usbx_device_deinit(VOID)
{
  UINT status = UX_SUCCESS;

  if (p_usb_device != UX_NULL)
  {
  /* Unregister USB device controller. */
  status = _ux_dcd_stm32_uninitialize(0, (ULONG)p_usb_device);

  if (status != UX_SUCCESS)
  {
    return status;
  }
    p_usb_device = UX_NULL;
  }

  /* Unregister audio class. */
  status = ux_device_stack_class_unregister(_ux_system_slave_class_audio_name, ux_device_class_audio_entry);
  if (status != UX_SUCCESS)
  {
    return status;
  }

  /* The code below is required for uninstalling the device portion of USBX.  */
  status = ux_device_stack_uninitialize();

  if (status != UX_SUCCESS)
  {
    return status;
  }
  return UX_SUCCESS;
}
/**
  * @brief  Application USBX Device Process.
  * @retval none
  */
UINT app_usbx_device_process(VOID)
{
  UINT status;

  /* Initialization of USB device */
  if (p_usb_device == UX_NULL)
  {
    /* Initialization of USB device */
    p_usb_device = mx_usb_drd_fs_device_gethandle();
    if (p_usb_device == UX_NULL)
    {
      return UX_ERROR;
    }

    /* Initialize the device controller driver */
    status = ux_dcd_stm32_initialize(0, (ULONG)p_usb_device);
    if (status != UX_SUCCESS)
    {
      p_usb_device = UX_NULL;
      return status;
    }
  }

  if (_ux_system_slave->ux_system_slave_device.ux_slave_device_state == UX_DEVICE_CONFIGURED)
  {
    usbd_audio_process();
  }

  status = ux_device_stack_tasks_run();

  if ((status != UX_STATE_RESET) &&
      (_ux_system_slave->ux_system_slave_device.ux_slave_device_state == UX_DEVICE_CONFIGURED))
  {
    usbd_audio_process();
  }

  return status;
}
