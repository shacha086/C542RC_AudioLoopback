/**
  ******************************************************************************
  * @file    mx_usbx_device_audio.c
  * @brief   USB Audio Class 1.0 full-duplex loopback application
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025-2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the
  * mx_usbx_license.md file in the same directory as the generated code.
  * If no mx_usbx_license.md file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "mx_usbx_device_audio.h"
#include "ux_device_class_audio10.h"
#include "ux_dcd_stm32.h"
#include "ux_stm32_device_descriptors.h"

/* Private define ------------------------------------------------------------*/
#define AUDIO_FRAME_PERIOD_HZ       1000U
#define AUDIO_LOOPBACK_SLOT_COUNT   8U
#define AUDIO_CAPTURE_CONTROL       0U
#define AUDIO_PLAYBACK_CONTROL      1U
#define AUDIO_FRAME_SIZE            ((USBD_AUDIO_SAMPLE_RATE / AUDIO_FRAME_PERIOD_HZ) * \
                                     USBD_AUDIO_CHANNEL_COUNT * USBD_AUDIO_RES_BYTE)

_Static_assert((USBD_AUDIO_SAMPLE_RATE % AUDIO_FRAME_PERIOD_HZ) == 0U,
               "The sample rate must produce an integral 1 ms USB frame");
_Static_assert(AUDIO_FRAME_SIZE <= USBD_AUDIO_EPIN_FS_MPS,
               "The capture endpoint is too small for one audio frame");
_Static_assert(AUDIO_FRAME_SIZE <= USBD_AUDIO_EPOUT_FS_MPS,
               "The playback endpoint is too small for one audio frame");

/* Private variables ---------------------------------------------------------*/
static UX_DEVICE_CLASS_AUDIO *audio_instance = UX_NULL;
static UX_DEVICE_CLASS_AUDIO_STREAM *capture_stream = UX_NULL;
static UX_DEVICE_CLASS_AUDIO_STREAM *playback_stream = UX_NULL;
static volatile ULONG capture_active = 0U;
static volatile ULONG playback_active = 0U;

/* The USB ISR is the single producer and the main loop is the single consumer.
   A packet is copied before the permanently armed PMA buffer can be overwritten
   by the next OUT transaction. */
static __ALIGNED(4) volatile UCHAR loopback_frames[AUDIO_LOOPBACK_SLOT_COUNT][AUDIO_FRAME_SIZE];
static volatile ULONG loopback_produced_sequence = 0U;
static volatile ULONG loopback_consumed_sequence = 0U;
static UCHAR loopback_last_frame[AUDIO_FRAME_SIZE];
static UCHAR loopback_last_frame_valid = 0U;

/* USB Audio 1.0 represents a discrete 48 kHz rate as one count byte followed
   by a three-byte, little-endian frequency. */
static UCHAR audio_frequency_descriptor[] =
{
  1U,
  (UCHAR)(USBD_AUDIO_SAMPLE_RATE & 0xFFU),
  (UCHAR)((USBD_AUDIO_SAMPLE_RATE >> 8) & 0xFFU),
  (UCHAR)((USBD_AUDIO_SAMPLE_RATE >> 16) & 0xFFU)
};

static UX_DEVICE_CLASS_AUDIO10_CONTROL audio_controls[] =
{
  {
    .ux_device_class_audio10_control_fu_id = USBD_AUDIO_CAPTURE_FEATURE_UNIT_ID,
    .ux_device_class_audio10_control_mute[0] = 0U,
  },
  {
    .ux_device_class_audio10_control_fu_id = USBD_AUDIO_PLAY_FEATURE_UNIT_ID,
    .ux_device_class_audio10_control_mute[0] = 0U,
  },
  {
    .ux_device_class_audio10_control_ep_addr = USBD_AUDIO_EPIN_ADDR,
    .ux_device_class_audio10_control_sam_freq_types = audio_frequency_descriptor,
    .ux_device_class_audio10_control_sam_freq = USBD_AUDIO_SAMPLE_RATE,
  },
  {
    .ux_device_class_audio10_control_ep_addr = USBD_AUDIO_EPOUT_ADDR,
    .ux_device_class_audio10_control_sam_freq_types = audio_frequency_descriptor,
    .ux_device_class_audio10_control_sam_freq = USBD_AUDIO_SAMPLE_RATE,
  }
};

static UX_DEVICE_CLASS_AUDIO10_CONTROL_GROUP audio_control_group =
{
  .ux_device_class_audio10_control_group_controls_nb =
    sizeof(audio_controls) / sizeof(audio_controls[0]),
  .ux_device_class_audio10_control_group_controls = audio_controls,
};

static UX_DEVICE_CLASS_AUDIO_STREAM_PARAMETER audio_stream_parameters[USBD_AUDIO_STREAM_NUMBER];

/* Private function prototypes -----------------------------------------------*/
static VOID usbd_audio_stream_change(UX_DEVICE_CLASS_AUDIO_STREAM *stream,
                                     ULONG alternate_setting);
static VOID usbd_audio_stream_frame_done(UX_DEVICE_CLASS_AUDIO_STREAM *stream,
                                         ULONG frames);
static VOID usbd_audio_fill_capture_frame(VOID);
static VOID usbd_audio_drain_playback_frames(VOID);
static VOID usbd_audio_loopback_reset(VOID);
static VOID usbd_audio_loopback_pop(UCHAR *frame);

/**
  * @brief  Save the audio class instance and resolve both stream instances.
  */
VOID usbd_audio_activate(VOID *instance)
{
  UX_DEVICE_CLASS_AUDIO_STREAM *stream;

  audio_instance = (UX_DEVICE_CLASS_AUDIO *)instance;
  capture_stream = UX_NULL;
  playback_stream = UX_NULL;
  capture_active = 0U;
  playback_active = 0U;
  usbd_audio_loopback_reset();

  if (audio_instance == UX_NULL)
  {
    return;
  }

  if (ux_device_class_audio_stream_get(audio_instance, 0U, &stream) == UX_SUCCESS)
  {
    capture_stream = stream;
  }
  if (ux_device_class_audio_stream_get(audio_instance, 1U, &stream) == UX_SUCCESS)
  {
    playback_stream = stream;
  }
}

/**
  * @brief  Release the audio class and stream instances.
  */
VOID usbd_audio_deactivate(VOID *instance)
{
  UX_PARAMETER_NOT_USED(instance);

  capture_active = 0U;
  playback_active = 0U;
  capture_stream = UX_NULL;
  playback_stream = UX_NULL;
  audio_instance = UX_NULL;
  usbd_audio_loopback_reset();
}

/**
  * @brief  Process USB Audio 1.0 feature-unit and endpoint requests.
  */
UINT usbd_audio_control_process(UX_DEVICE_CLASS_AUDIO *instance,
                               UX_SLAVE_TRANSFER *transfer)
{
  return ux_device_class_audio10_control_process(instance,
                                                 transfer,
                                                 &audio_control_group);
}

/**
  * @brief  Start or stop an isochronous stream after SET_INTERFACE.
  */
static VOID usbd_audio_stream_change(UX_DEVICE_CLASS_AUDIO_STREAM *stream,
                                     ULONG alternate_setting)
{
  if (stream == capture_stream)
  {
    capture_active = 0U;
    usbd_audio_loopback_reset();

    if (alternate_setting != 0U)
    {
      usbd_audio_fill_capture_frame();
      if (ux_device_class_audio_transmission_start(stream) == UX_SUCCESS)
      {
        capture_active = 1U;
      }
    }
  }
  else if (stream == playback_stream)
  {
    playback_active = 0U;

    if (alternate_setting != 0U)
    {
      if (ux_device_class_audio_reception_start(stream) == UX_SUCCESS)
      {
        playback_active = 1U;
      }
    }
    else
    {
      usbd_audio_loopback_reset();
    }
  }
}

/**
  * @brief  Refill capture data on the USB frame cadence.
  */
static VOID usbd_audio_stream_frame_done(UX_DEVICE_CLASS_AUDIO_STREAM *stream,
                                         ULONG frames)
{
  UX_PARAMETER_NOT_USED(frames);

  if ((stream == capture_stream) && (capture_active != 0U))
  {
    usbd_audio_fill_capture_frame();
  }
}

/**
  * @brief  Queue exactly one 1 ms frame for the capture endpoint.
  */
static VOID usbd_audio_fill_capture_frame(VOID)
{
  UCHAR *frame;
  ULONG maximum_length;

  if (capture_stream == UX_NULL)
  {
    return;
  }

  if (ux_device_class_audio_write_frame_get(capture_stream,
                                            &frame,
                                            &maximum_length) != UX_SUCCESS)
  {
    return;
  }

  if (maximum_length < AUDIO_FRAME_SIZE)
  {
    return;
  }

  usbd_audio_loopback_pop(frame);

  /* Feature units advertise mute only. Keep consuming playback while muted so
     unmuting resumes at the current frame instead of replaying stale audio. */
  if ((audio_controls[AUDIO_CAPTURE_CONTROL].ux_device_class_audio10_control_mute[0] != 0U) ||
      (audio_controls[AUDIO_PLAYBACK_CONTROL].ux_device_class_audio10_control_mute[0] != 0U))
  {
    ux_utility_memory_set(frame, 0, AUDIO_FRAME_SIZE);
  }

  (VOID)ux_device_class_audio_write_frame_commit(capture_stream, AUDIO_FRAME_SIZE);
}

/**
  * @brief  Release class receive frames after the ISR has snapshotted them.
  */
static VOID usbd_audio_drain_playback_frames(VOID)
{
  UCHAR *frame;
  ULONG length;

  if (playback_stream == UX_NULL)
  {
    return;
  }

  while (ux_device_class_audio_read_frame_get(playback_stream,
                                               &frame,
                                               &length) == UX_SUCCESS)
  {
    UX_PARAMETER_NOT_USED(frame);
    UX_PARAMETER_NOT_USED(length);

    if (ux_device_class_audio_read_frame_free(playback_stream) != UX_SUCCESS)
    {
      break;
    }
  }
}

/**
  * @brief  Snapshot a completed isochronous OUT packet in USB interrupt context.
  * @note   This strong definition overrides the controller driver's weak hook.
  */
VOID ux_dcd_stm32_iso_out_received(uint8_t endpoint_number,
                                   UCHAR *data,
                                   ULONG length)
{
  ULONG sequence;
  ULONG slot;
  ULONG copy_length;
  UCHAR *destination;

  if ((endpoint_number != (USBD_AUDIO_EPOUT_ADDR & 0x7FU)) || (data == UX_NULL))
  {
    return;
  }

  sequence = loopback_produced_sequence + 1U;
  slot = sequence % AUDIO_LOOPBACK_SLOT_COUNT;
  copy_length = (length < AUDIO_FRAME_SIZE) ? length : AUDIO_FRAME_SIZE;
  destination = (UCHAR *)(VOID *)loopback_frames[slot];

  if (copy_length != 0U)
  {
    ux_utility_memory_copy(destination, data, copy_length);
  }
  if (copy_length < AUDIO_FRAME_SIZE)
  {
    ux_utility_memory_set(destination + copy_length,
                          0,
                          AUDIO_FRAME_SIZE - copy_length);
  }

  __DMB();
  loopback_produced_sequence = sequence;
}

/**
  * @brief  Drop queued frames and invalidate the repeat cache.
  */
static VOID usbd_audio_loopback_reset(VOID)
{
  __DMB();
  loopback_consumed_sequence = loopback_produced_sequence;
  loopback_last_frame_valid = 0U;
}

/**
  * @brief  Consume one queued frame, or repeat the last frame on short starvation.
  */
static VOID usbd_audio_loopback_pop(UCHAR *frame)
{
  ULONG produced_sequence;
  ULONG pending_count;
  ULONG next_sequence;

  __DMB();
  produced_sequence = loopback_produced_sequence;
  pending_count = produced_sequence - loopback_consumed_sequence;

  if (pending_count == 0U)
  {
    if (loopback_last_frame_valid != 0U)
    {
      ux_utility_memory_copy(frame, loopback_last_frame, AUDIO_FRAME_SIZE);
    }
    else
    {
      ux_utility_memory_set(frame, 0, AUDIO_FRAME_SIZE);
    }
    return;
  }

  next_sequence = loopback_consumed_sequence + 1U;
  if (pending_count > AUDIO_LOOPBACK_SLOT_COUNT)
  {
    next_sequence = produced_sequence - (AUDIO_LOOPBACK_SLOT_COUNT - 1U);
  }

  ux_utility_memory_copy(frame,
                         (UCHAR *)(VOID *)loopback_frames[next_sequence % AUDIO_LOOPBACK_SLOT_COUNT],
                         AUDIO_FRAME_SIZE);
  __DMB();
  loopback_consumed_sequence = next_sequence;

  ux_utility_memory_copy(loopback_last_frame, frame, AUDIO_FRAME_SIZE);
  loopback_last_frame_valid = 1U;
}

/**
  * @brief  Run non-interrupt audio maintenance from the application loop.
  */
VOID usbd_audio_process(VOID)
{
  if (playback_active != 0U)
  {
    usbd_audio_drain_playback_frames();
  }
}

/**
  * @brief  Fill the USBX audio class parameters for capture and playback.
  */
VOID usbd_audio_get_parameters(UX_DEVICE_CLASS_AUDIO_PARAMETER *audio_parameter)
{
  if (audio_parameter == UX_NULL)
  {
    return;
  }

  ux_utility_memory_set(audio_stream_parameters, 0, sizeof(audio_stream_parameters));
  ux_utility_memory_set(audio_parameter, 0, sizeof(*audio_parameter));

  audio_stream_parameters[0].ux_device_class_audio_stream_parameter_endpoint_direction = UX_ENDPOINT_IN;
  audio_stream_parameters[0].ux_device_class_audio_stream_parameter_max_frame_buffer_size = USBD_AUDIO_EPIN_FS_MPS;
  audio_stream_parameters[0].ux_device_class_audio_stream_parameter_max_frame_buffer_nb = AUDIO_LOOPBACK_SLOT_COUNT;
#if defined(UX_DEVICE_STANDALONE)
  audio_stream_parameters[0].ux_device_class_audio_stream_parameter_task_function =
    _ux_device_class_audio_write_task_function;
#else
  audio_stream_parameters[0].ux_device_class_audio_stream_parameter_thread_entry =
    ux_device_class_audio_write_thread_entry;
#endif
  audio_stream_parameters[0].ux_device_class_audio_stream_parameter_callbacks.ux_device_class_audio_stream_change =
    usbd_audio_stream_change;
  audio_stream_parameters[0].ux_device_class_audio_stream_parameter_callbacks.ux_device_class_audio_stream_frame_done =
    usbd_audio_stream_frame_done;

  audio_stream_parameters[1].ux_device_class_audio_stream_parameter_endpoint_direction = UX_ENDPOINT_OUT;
  audio_stream_parameters[1].ux_device_class_audio_stream_parameter_max_frame_buffer_size = USBD_AUDIO_EPOUT_FS_MPS;
  audio_stream_parameters[1].ux_device_class_audio_stream_parameter_max_frame_buffer_nb = AUDIO_LOOPBACK_SLOT_COUNT;
#if defined(UX_DEVICE_STANDALONE)
  audio_stream_parameters[1].ux_device_class_audio_stream_parameter_task_function =
    _ux_device_class_audio_read_task_function;
#else
  audio_stream_parameters[1].ux_device_class_audio_stream_parameter_thread_entry =
    ux_device_class_audio_read_thread_entry;
#endif
  audio_stream_parameters[1].ux_device_class_audio_stream_parameter_callbacks.ux_device_class_audio_stream_change =
    usbd_audio_stream_change;
  audio_stream_parameters[1].ux_device_class_audio_stream_parameter_callbacks.ux_device_class_audio_stream_frame_done =
    usbd_audio_stream_frame_done;

  audio_parameter->ux_device_class_audio_parameter_streams_nb = USBD_AUDIO_STREAM_NUMBER;
  audio_parameter->ux_device_class_audio_parameter_streams = audio_stream_parameters;
  audio_parameter->ux_device_class_audio_parameter_callbacks.ux_slave_class_audio_instance_activate =
    usbd_audio_activate;
  audio_parameter->ux_device_class_audio_parameter_callbacks.ux_slave_class_audio_instance_deactivate =
    usbd_audio_deactivate;
  audio_parameter->ux_device_class_audio_parameter_callbacks.ux_device_class_audio_control_process =
    usbd_audio_control_process;
}
