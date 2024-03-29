/*
 *  PCM Interface - local header file
 *  Copyright (c) 2000 by Jaroslav Kysela <perex@perex.cz>
 *                        Abramo Bagnara <abramo@alsa-project.org>
 *
 *
 *   This library is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Lesser General Public License as
 *   published by the Free Software Foundation; either version 2.1 of
 *   the License, or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Lesser General Public License for more details.
 *
 *   You should have received a copy of the GNU Lesser General Public
 *   License along with this library; if not, write to the Free Software
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifndef __PCM_LOCAL_H
#define __PCM_LOCAL_H

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/uio.h>
#include <time.h>
#include <sys/time.h>

#define _snd_mask sndrv_mask
#define _snd_pcm_access_mask _snd_mask
#define _snd_pcm_format_mask _snd_mask
#define _snd_pcm_subformat_mask _snd_mask

#include "local.h"

#ifdef THREAD_SAFE_API
#define __USE_UNIX98 1	/* for old glibc */
#include <pthread.h>
#endif

#define SND_INTERVAL_INLINE
#include "interval.h"

#define SND_MASK_INLINE
#include "mask.h"

#define SND_PCM_HW_PARAM_ACCESS SNDRV_PCM_HW_PARAM_ACCESS
#define SND_PCM_HW_PARAM_FORMAT SNDRV_PCM_HW_PARAM_FORMAT
#define SND_PCM_HW_PARAM_SUBFORMAT SNDRV_PCM_HW_PARAM_SUBFORMAT
#define SND_PCM_HW_PARAM_SAMPLE_BITS SNDRV_PCM_HW_PARAM_SAMPLE_BITS
#define SND_PCM_HW_PARAM_FRAME_BITS SNDRV_PCM_HW_PARAM_FRAME_BITS
#define SND_PCM_HW_PARAM_CHANNELS SNDRV_PCM_HW_PARAM_CHANNELS
#define SND_PCM_HW_PARAM_RATE SNDRV_PCM_HW_PARAM_RATE
#define SND_PCM_HW_PARAM_PERIOD_TIME SNDRV_PCM_HW_PARAM_PERIOD_TIME
#define SND_PCM_HW_PARAM_PERIOD_SIZE SNDRV_PCM_HW_PARAM_PERIOD_SIZE
#define SND_PCM_HW_PARAM_PERIOD_BYTES SNDRV_PCM_HW_PARAM_PERIOD_BYTES
#define SND_PCM_HW_PARAM_PERIODS SNDRV_PCM_HW_PARAM_PERIODS
#define SND_PCM_HW_PARAM_BUFFER_TIME SNDRV_PCM_HW_PARAM_BUFFER_TIME
#define SND_PCM_HW_PARAM_BUFFER_SIZE SNDRV_PCM_HW_PARAM_BUFFER_SIZE
#define SND_PCM_HW_PARAM_BUFFER_BYTES SNDRV_PCM_HW_PARAM_BUFFER_BYTES
#define SND_PCM_HW_PARAM_TICK_TIME SNDRV_PCM_HW_PARAM_TICK_TIME
#define SND_PCM_HW_PARAM_LAST_MASK SNDRV_PCM_HW_PARAM_LAST_MASK
#define SND_PCM_HW_PARAM_FIRST_MASK SNDRV_PCM_HW_PARAM_FIRST_MASK
#define SND_PCM_HW_PARAM_LAST_INTERVAL SNDRV_PCM_HW_PARAM_LAST_INTERVAL
#define SND_PCM_HW_PARAM_FIRST_INTERVAL SNDRV_PCM_HW_PARAM_FIRST_INTERVAL

/** device accepts mmaped access */
#define SND_PCM_INFO_MMAP SNDRV_PCM_INFO_MMAP
/** device accepts  mmaped access with sample resolution */
#define SND_PCM_INFO_MMAP_VALID SNDRV_PCM_INFO_MMAP_VALID
/** device is doing double buffering */
#define SND_PCM_INFO_DOUBLE SNDRV_PCM_INFO_DOUBLE
/** device transfers samples in batch */
#define SND_PCM_INFO_BATCH SNDRV_PCM_INFO_BATCH
/** device does perfect drain (silencing not required) */
#define SND_PCM_INFO_PERFECT_DRAIN SNDRV_PCM_INFO_PERFECT_DRAIN
/** device accepts interleaved samples */
#define SND_PCM_INFO_INTERLEAVED SNDRV_PCM_INFO_INTERLEAVED
/** device accepts non-interleaved samples */
#define SND_PCM_INFO_NONINTERLEAVED SNDRV_PCM_INFO_NONINTERLEAVED
/** device accepts complex sample organization */
#define SND_PCM_INFO_COMPLEX SNDRV_PCM_INFO_COMPLEX
/** device is capable block transfers */
#define SND_PCM_INFO_BLOCK_TRANSFER SNDRV_PCM_INFO_BLOCK_TRANSFER
/** device can detect DAC/ADC overrange */
#define SND_PCM_INFO_OVERRANGE SNDRV_PCM_INFO_OVERRANGE
/** device supports resume */
#define SND_PCM_INFO_RESUME SNDRV_PCM_INFO_RESUME
/** device is capable to pause */
#define SND_PCM_INFO_PAUSE SNDRV_PCM_INFO_PAUSE
/** device can do only half duplex */
#define SND_PCM_INFO_HALF_DUPLEX SNDRV_PCM_INFO_HALF_DUPLEX
/** device can do only joint duplex (same parameters) */
#define SND_PCM_INFO_JOINT_DUPLEX SNDRV_PCM_INFO_JOINT_DUPLEX
/** device can do a kind of synchronized start */
#define SND_PCM_INFO_SYNC_START SNDRV_PCM_INFO_SYNC_START
/** device can disable period wakeups */
#define SND_PCM_INFO_NO_PERIOD_WAKEUP SNDRV_PCM_INFO_NO_PERIOD_WAKEUP

#define SND_PCM_HW_PARAMS_NORESAMPLE SNDRV_PCM_HW_PARAMS_NORESAMPLE
#define SND_PCM_HW_PARAMS_EXPORT_BUFFER SNDRV_PCM_HW_PARAMS_EXPORT_BUFFER
#define SND_PCM_HW_PARAMS_NO_PERIOD_WAKEUP SNDRV_PCM_HW_PARAMS_NO_PERIOD_WAKEUP
#define SND_PCM_HW_PARAMS_NO_DRAIN_SILENCE SNDRV_PCM_HW_PARAMS_NO_DRAIN_SILENCE

#define SND_PCM_INFO_MONOTONIC	0x80000000

typedef struct _snd_pcm_rbptr {
	snd_pcm_t *master;
	volatile snd_pcm_uframes_t *ptr;
	int fd;
	off_t offset;
	int link_dst_count;
	snd_pcm_t **link_dst;
	void *private_data;
	void (*changed)(snd_pcm_t *pcm, snd_pcm_t *src);
} snd_pcm_rbptr_t;

typedef struct _snd_pcm_channel_info {
	unsigned int channel;
	void *addr;			/* base address of channel samples */
	unsigned int first;		/* offset to first sample in bits */
	unsigned int step;		/* samples distance in bits */
	enum { SND_PCM_AREA_SHM, SND_PCM_AREA_MMAP, SND_PCM_AREA_LOCAL } type;
	union {
		struct {
			struct snd_shm_area *area;
			int shmid;
		} shm;
		struct {
			int fd;
			off_t offset;
		} mmap;
	} u;
	char reserved[64];
} snd_pcm_channel_info_t;

typedef struct {
	int (*close)(snd_pcm_t *pcm);
	int (*nonblock)(snd_pcm_t *pcm, int nonblock); /* always locked */
	int (*async)(snd_pcm_t *pcm, int sig, pid_t pid);
	int (*info)(snd_pcm_t *pcm, snd_pcm_info_t *info);
	int (*hw_refine)(snd_pcm_t *pcm, snd_pcm_hw_params_t *params);
	int (*hw_params)(snd_pcm_t *pcm, snd_pcm_hw_params_t *params);
	int (*hw_free)(snd_pcm_t *pcm);
	int (*sw_params)(snd_pcm_t *pcm, snd_pcm_sw_params_t *params); /* always locked */
	int (*channel_info)(snd_pcm_t *pcm, snd_pcm_channel_info_t *info);
	void (*dump)(snd_pcm_t *pcm, snd_output_t *out);
	int (*mmap)(snd_pcm_t *pcm);
	int (*munmap)(snd_pcm_t *pcm);
	snd_pcm_chmap_query_t **(*query_chmaps)(snd_pcm_t *pcm);
	snd_pcm_chmap_t *(*get_chmap)(snd_pcm_t *pcm);
	int (*set_chmap)(snd_pcm_t *pcm, const snd_pcm_chmap_t *map);
} snd_pcm_ops_t;

typedef struct {
	int (*status)(snd_pcm_t *pcm, snd_pcm_status_t *status); /* locked */
	int (*prepare)(snd_pcm_t *pcm); /* locked */
	int (*reset)(snd_pcm_t *pcm); /* locked */
	int (*start)(snd_pcm_t *pcm); /* locked */
	int (*drop)(snd_pcm_t *pcm); /* locked */
	int (*drain)(snd_pcm_t *pcm); /* need own locking */
	int (*pause)(snd_pcm_t *pcm, int enable); /* locked */
	snd_pcm_state_t (*state)(snd_pcm_t *pcm); /* locked */
	int (*hwsync)(snd_pcm_t *pcm); /* locked */
	int (*delay)(snd_pcm_t *pcm, snd_pcm_sframes_t *delayp); /* locked */
	int (*resume)(snd_pcm_t *pcm); /* need own locking */
	int (*link)(snd_pcm_t *pcm1, snd_pcm_t *pcm2);
	int (*link_slaves)(snd_pcm_t *pcm, snd_pcm_t *master);
	int (*unlink)(snd_pcm_t *pcm);
	snd_pcm_sframes_t (*rewindable)(snd_pcm_t *pcm); /* locked */
	snd_pcm_sframes_t (*rewind)(snd_pcm_t *pcm, snd_pcm_uframes_t frames); /* locked */
	snd_pcm_sframes_t (*forwardable)(snd_pcm_t *pcm); /* locked */
	snd_pcm_sframes_t (*forward)(snd_pcm_t *pcm, snd_pcm_uframes_t frames); /* locked */
	snd_pcm_sframes_t (*writei)(snd_pcm_t *pcm, const void *buffer, snd_pcm_uframes_t size); /* need own locking */
	snd_pcm_sframes_t (*writen)(snd_pcm_t *pcm, void **bufs, snd_pcm_uframes_t size); /* need own locking */
	snd_pcm_sframes_t (*readi)(snd_pcm_t *pcm, void *buffer, snd_pcm_uframes_t size); /* need own locking */
	snd_pcm_sframes_t (*readn)(snd_pcm_t *pcm, void **bufs, snd_pcm_uframes_t size); /* need own locking */
	snd_pcm_sframes_t (*avail_update)(snd_pcm_t *pcm); /* locked */
	snd_pcm_sframes_t (*mmap_commit)(snd_pcm_t *pcm, snd_pcm_uframes_t offset, snd_pcm_uframes_t size); /* locked */
	int (*htimestamp)(snd_pcm_t *pcm, snd_pcm_uframes_t *avail, snd_htimestamp_t *tstamp); /* locked */
	int (*poll_descriptors_count)(snd_pcm_t *pcm); /* locked */
	int (*poll_descriptors)(snd_pcm_t *pcm, struct pollfd *pfds, unsigned int space); /* locked */
	int (*poll_revents)(snd_pcm_t *pcm, struct pollfd *pfds, unsigned int nfds, unsigned short *revents); /* locked */
	int (*may_wait_for_avail_min)(snd_pcm_t *pcm, snd_pcm_uframes_t avail);
	int (*mmap_begin)(snd_pcm_t *pcm, const snd_pcm_channel_area_t **areas, snd_pcm_uframes_t *offset, snd_pcm_uframes_t *frames); /* locked */
} snd_pcm_fast_ops_t;

struct _snd_pcm {
	void *open_func;
	char *name;
	snd_pcm_type_t type;
	snd_pcm_stream_t stream;
	int mode;
	long minperiodtime;		/* in us */
	int poll_fd_count;
	int poll_fd;
	unsigned short poll_events;
	int setup: 1,
	    compat: 1;
	snd_pcm_access_t access;	/* access mode */
	snd_pcm_format_t format;	/* SND_PCM_FORMAT_* */
	snd_pcm_subformat_t subformat;	/* subformat */
	unsigned int channels;		/* channels */
	unsigned int rate;		/* rate in Hz */
	snd_pcm_uframes_t period_size;
	unsigned int period_time;	/* period duration */
	snd_interval_t periods;
	snd_pcm_tstamp_t tstamp_mode;	/* timestamp mode */
	snd_pcm_tstamp_type_t tstamp_type;	/* timestamp type */
	unsigned int period_step;
	snd_pcm_uframes_t avail_min;	/* min avail frames for wakeup */
	int period_event;
	snd_pcm_uframes_t start_threshold;
	snd_pcm_uframes_t stop_threshold;
	snd_pcm_uframes_t silence_threshold;	/* Silence filling happens when
					   noise is nearest than this */
	snd_pcm_uframes_t silence_size;	/* Silence filling size */
	snd_pcm_uframes_t boundary;	/* pointers wrap point */
	unsigned int info;		/* Info for returned setup */
	unsigned int msbits;		/* used most significant bits */
	unsigned int rate_num;		/* rate numerator */
	unsigned int rate_den;		/* rate denominator */
	unsigned int hw_flags;		/* actual hardware flags */
	snd_pcm_uframes_t fifo_size;	/* chip FIFO size in frames */
	snd_pcm_uframes_t buffer_size;
	snd_interval_t buffer_time;
	unsigned int sample_bits;
	unsigned int frame_bits;
	snd_pcm_rbptr_t appl;
	snd_pcm_rbptr_t hw;
	snd_pcm_uframes_t min_align;
	unsigned int mmap_rw: 1;	/* use always mmapped buffer */
	unsigned int mmap_shadow: 1;	/* don't call actual mmap,
					 * use the mmaped buffer of the slave
					 */
	unsigned int donot_close: 1;	/* don't close this PCM */
	unsigned int own_state_check:1; /* plugin has own PCM state check */
	snd_pcm_channel_info_t *mmap_channels;
	snd_pcm_channel_area_t *running_areas;
	snd_pcm_channel_area_t *stopped_areas;
	const snd_pcm_ops_t *ops;
	const snd_pcm_fast_ops_t *fast_ops;
	snd_pcm_t *op_arg;
	snd_pcm_t *fast_op_arg;
	void *private_data;
	struct list_head async_handlers;
#ifdef THREAD_SAFE_API
	int need_lock;		/* true = this PCM (plugin) is thread-unsafe,
				 * thus it needs a lock.
				 */
	int lock_enabled;	/* thread-safety lock is enabled on the system;
				 * it's set depending on $LIBASOUND_THREAD_SAFE.
				 */
	pthread_mutex_t lock;
#endif
};

/* make local functions really local */
/* Grrr, these cannot be local - a bad aserver uses them!
#define snd_pcm_async \
	snd1_pcm_async
#define snd_pcm_mmap \
	snd1_pcm_mmap
#define snd_pcm_munmap \
	snd1_pcm_munmap
#define snd_pcm_hw_refine \
	snd1_pcm_hw_refine
*/
#define snd_pcm_new \
	snd1_pcm_new
#define snd_pcm_free \
	snd1_pcm_free
#define snd_pcm_areas_from_buf \
	snd1_pcm_areas_from_buf
#define snd_pcm_areas_from_bufs \
	snd1_pcm_areas_from_bufs
#define snd_pcm_open_named_slave \
	snd1_pcm_open_named_slave
#define snd_pcm_hw_open_fd \
	snd1_pcm_hw_open_fd
#define snd_pcm_wait_nocheck \
	snd1_pcm_wait_nocheck
#define snd_pcm_rate_get_default_converter \
	snd1_pcm_rate_get_default_converter
#define snd_pcm_set_hw_ptr \
	snd1_pcm_set_hw_ptr
#define snd_pcm_set_appl_ptr \
	snd1_pcm_set_appl_ptr
#define snd_pcm_link_hw_ptr \
	snd1_pcm_link_hw_ptr
#define snd_pcm_link_appl_ptr \
	snd1_pcm_link_appl_ptr
#define snd_pcm_unlink_hw_ptr \
	snd1_pcm_unlink_hw_ptr
#define snd_pcm_unlink_appl_ptr \
	snd1_pcm_unlink_appl_ptr
#define snd_pcm_mmap_appl_ptr \
	snd1_pcm_mmap_appl_ptr
#define snd_pcm_mmap_appl_backward \
	snd1_pcm_mmap_appl_backward
#define snd_pcm_mmap_appl_forward \
	snd1_pcm_mmap_appl_forward
#define snd_pcm_mmap_hw_backward \
	snd1_pcm_mmap_hw_backward
#define snd_pcm_mmap_hw_forward \
	snd1_pcm_mmap_hw_forward
#define snd_pcm_read_areas \
	snd1_pcm_read_areas
#define snd_pcm_write_areas \
	snd1_pcm_write_areas
#define snd_pcm_read_mmap \
	snd1_pcm_read_mmap
#define snd_pcm_write_mmap \
	snd1_pcm_write_mmap
#define snd_pcm_channel_info_shm \
	snd1_pcm_channel_info_shm
#define snd_pcm_hw_refine_soft \
	snd1_pcm_hw_refine_soft
#define snd_pcm_hw_refine_slave \
	snd1_pcm_hw_refine_slave
#define snd_pcm_hw_params_slave \
	snd1_pcm_hw_params_slave
#define snd_pcm_hw_param_refine_near \
	snd1_pcm_hw_param_refine_near
#define snd_pcm_hw_param_refine_multiple \
	snd1_pcm_hw_param_refine_multiple
#define snd_pcm_hw_param_empty \
	snd1_pcm_hw_param_empty
#define snd_pcm_hw_param_always_eq \
	snd1_pcm_hw_param_always_eq
#define snd_pcm_hw_param_never_eq \
	snd1_pcm_hw_param_never_eq
#define snd_pcm_hw_param_get_mask \
	snd1_pcm_hw_param_get_mask
#define snd_pcm_hw_param_get_interval \
	snd1_pcm_hw_param_get_interval
#define snd_pcm_hw_param_any \
	snd1_pcm_hw_param_any
#define snd_pcm_hw_param_set_integer \
	snd1_pcm_hw_param_set_integer
#define snd_pcm_hw_param_set_first \
	snd1_pcm_hw_param_set_first
#define snd_pcm_hw_param_set_last \
	snd1_pcm_hw_param_set_last
#define snd_pcm_hw_param_set_near \
	snd1_pcm_hw_param_set_near
#define snd_pcm_hw_param_set_min \
	snd1_pcm_hw_param_set_min
#define snd_pcm_hw_param_set_max \
	snd1_pcm_hw_param_set_max
#define snd_pcm_hw_param_set_minmax \
	snd1_pcm_hw_param_set_minmax
#define snd_pcm_hw_param_set \
	snd1_pcm_hw_param_set
#define snd_pcm_hw_param_set_mask \
	snd1_pcm_hw_param_set_mask
#define snd_pcm_hw_param_get \
	snd1_pcm_hw_param_get
#define snd_pcm_hw_param_get_min \
	snd1_pcm_hw_param_get_min
#define snd_pcm_hw_param_get_max \
	snd1_pcm_hw_param_get_max
#define snd_pcm_hw_param_name		\
	snd1_pcm_hw_param_name
#define snd_pcm_sw_params_current_no_lock \
	snd1_pcm_sw_params_current_no_lock

int snd_pcm_new(snd_pcm_t **pcmp, snd_pcm_type_t type, const char *name,
		snd_pcm_stream_t stream, int mode);
int snd_pcm_free(snd_pcm_t *pcm);

void snd_pcm_areas_from_buf(snd_pcm_t *pcm, snd_pcm_channel_area_t *areas, void *buf);
void snd_pcm_areas_from_bufs(snd_pcm_t *pcm, snd_pcm_channel_area_t *areas, void **bufs);

int snd_pcm_async(snd_pcm_t *pcm, int sig, pid_t pid);
int snd_pcm_mmap(snd_pcm_t *pcm);
int snd_pcm_munmap(snd_pcm_t *pcm);
int snd_pcm_mmap_ready(snd_pcm_t *pcm);
void snd_pcm_set_hw_ptr(snd_pcm_t *pcm, volatile snd_pcm_uframes_t *hw_ptr, int fd, off_t offset);
void snd_pcm_set_appl_ptr(snd_pcm_t *pcm, volatile snd_pcm_uframes_t *appl_ptr, int fd, off_t offset);
void snd_pcm_link_hw_ptr(snd_pcm_t *pcm, snd_pcm_t *slave);
void snd_pcm_link_appl_ptr(snd_pcm_t *pcm, snd_pcm_t *slave);
void snd_pcm_unlink_hw_ptr(snd_pcm_t *pcm, snd_pcm_t *slave);
void snd_pcm_unlink_appl_ptr(snd_pcm_t *pcm, snd_pcm_t *slave);
snd_pcm_sframes_t snd_pcm_mmap_appl_ptr(snd_pcm_t *pcm, off_t offset);
void snd_pcm_mmap_appl_backward(snd_pcm_t *pcm, snd_pcm_uframes_t frames);
void snd_pcm_mmap_appl_forward(snd_pcm_t *pcm, snd_pcm_uframes_t frames);
void snd_pcm_mmap_hw_backward(snd_pcm_t *pcm, snd_pcm_uframes_t frames);
void snd_pcm_mmap_hw_forward(snd_pcm_t *pcm, snd_pcm_uframes_t frames);

void snd_pcm_sw_params_current_no_lock(snd_pcm_t *pcm, snd_pcm_sw_params_t *params);

snd_pcm_sframes_t snd_pcm_mmap_writei(snd_pcm_t *pcm, const void *buffer, snd_pcm_uframes_t size);
snd_pcm_sframes_t snd_pcm_mmap_readi(snd_pcm_t *pcm, void *buffer, snd_pcm_uframes_t size);
snd_pcm_sframes_t snd_pcm_mmap_writen(snd_pcm_t *pcm, void **bufs, snd_pcm_uframes_t size);
snd_pcm_sframes_t snd_pcm_mmap_readn(snd_pcm_t *pcm, void **bufs, snd_pcm_uframes_t size);

typedef snd_pcm_sframes_t (*snd_pcm_xfer_areas_func_t)(snd_pcm_t *pcm, 
						       const snd_pcm_channel_area_t *areas,
						       snd_pcm_uframes_t offset, 
						       snd_pcm_uframes_t size);

snd_pcm_sframes_t snd_pcm_read_areas(snd_pcm_t *pcm, const snd_pcm_channel_area_t *areas,
				     snd_pcm_uframes_t offset, snd_pcm_uframes_t size,
				     snd_pcm_xfer_areas_func_t func);
snd_pcm_sframes_t snd_pcm_write_areas(snd_pcm_t *pcm, const snd_pcm_channel_area_t *areas,
				      snd_pcm_uframes_t offset, snd_pcm_uframes_t size,
				      snd_pcm_xfer_areas_func_t func);
snd_pcm_sframes_t snd_pcm_read_mmap(snd_pcm_t *pcm, snd_pcm_uframes_t offset,
				    snd_pcm_uframes_t size);
snd_pcm_sframes_t snd_pcm_write_mmap(snd_pcm_t *pcm, snd_pcm_uframes_t offset,
				     snd_pcm_uframes_t size);
static inline int snd_pcm_channel_info(snd_pcm_t *pcm, snd_pcm_channel_info_t *info)
{
	if (!pcm->ops->channel_info)
		return -ENOSYS;
	return pcm->ops->channel_info(pcm, info);
}
int snd_pcm_channel_info_shm(snd_pcm_t *pcm, snd_pcm_channel_info_t *info, int shmid);
int _snd_pcm_poll_descriptor(snd_pcm_t *pcm);
#define _snd_pcm_link_descriptor _snd_pcm_poll_descriptor /* FIXME */
#define _snd_pcm_async_descriptor _snd_pcm_poll_descriptor /* FIXME */

/* locked versions */
int __snd_pcm_mmap_begin_generic(snd_pcm_t *pcm, const snd_pcm_channel_area_t **areas,
				 snd_pcm_uframes_t *offset, snd_pcm_uframes_t *frames);
int __snd_pcm_mmap_begin(snd_pcm_t *pcm, const snd_pcm_channel_area_t **areas,
			 snd_pcm_uframes_t *offset, snd_pcm_uframes_t *frames);
snd_pcm_sframes_t __snd_pcm_mmap_commit(snd_pcm_t *pcm,
					snd_pcm_uframes_t offset,
					snd_pcm_uframes_t frames);
int __snd_pcm_wait_in_lock(snd_pcm_t *pcm, int timeout);

static inline snd_pcm_sframes_t __snd_pcm_avail_update(snd_pcm_t *pcm)
{
	if (!pcm->fast_ops->avail_update)
		return -ENOSYS;
	return pcm->fast_ops->avail_update(pcm->fast_op_arg);
}

static inline int __snd_pcm_start(snd_pcm_t *pcm)
{
	if (!pcm->fast_ops->start)
		return -ENOSYS;
	return pcm->fast_ops->start(pcm->fast_op_arg);
}

static inline snd_pcm_state_t __snd_pcm_state(snd_pcm_t *pcm)
{
	if (!pcm->fast_ops->state)
		return SND_PCM_STATE_OPEN;
	return pcm->fast_ops->state(pcm->fast_op_arg);
}

static inline int __snd_pcm_hwsync(snd_pcm_t *pcm)
{
	if (!pcm->fast_ops->hwsync)
		return -ENOSYS;
	return pcm->fast_ops->hwsync(pcm->fast_op_arg);
}

static inline int __snd_pcm_delay(snd_pcm_t *pcm, snd_pcm_sframes_t *delayp)
{
	if (!pcm->fast_ops->delay)
		return -ENOSYS;
	return pcm->fast_ops->delay(pcm->fast_op_arg, delayp);
}

/* handle special error cases */
static inline int snd_pcm_check_error(snd_pcm_t *pcm, int err)
{
	if (err == -EINTR) {
		switch (__snd_pcm_state(pcm)) {
		case SND_PCM_STATE_XRUN:
			return -EPIPE;
		case SND_PCM_STATE_SUSPENDED:
			return -ESTRPIPE;
		case SND_PCM_STATE_DISCONNECTED:
			return -ENODEV;
		default:
			break;
		}
	}
	return err;
}

/**
 * \retval number of frames available to the application for playback
 *
 * This is how far ahead the hardware position in the ring buffer is,
 * compared to the application position. ie. for playback it's the
 * number of frames in the empty part of the ring buffer.
 */
static inline snd_pcm_uframes_t __snd_pcm_playback_avail(snd_pcm_t *pcm,
							 const snd_pcm_uframes_t hw_ptr,
							 const snd_pcm_uframes_t appl_ptr)
{
	snd_pcm_sframes_t avail;
	avail = hw_ptr + pcm->buffer_size - appl_ptr;
	if (avail < 0)
		avail += pcm->boundary;
	else if ((snd_pcm_uframes_t) avail >= pcm->boundary)
		avail -= pcm->boundary;
	return avail;
}

static inline snd_pcm_uframes_t snd_pcm_mmap_playback_avail(snd_pcm_t *pcm)
{
	return __snd_pcm_playback_avail(pcm, *pcm->hw.ptr, *pcm->appl.ptr);
}

/*
 * \retval number of frames available to the application for capture
 *
 * This is how far ahead the hardware position in the ring buffer is
 * compared to the application position.  ie. for capture, it's the
 * number of frames in the filled part of the ring buffer.
 */
static inline snd_pcm_uframes_t __snd_pcm_capture_avail(snd_pcm_t *pcm,
							const snd_pcm_uframes_t hw_ptr,
							const snd_pcm_uframes_t appl_ptr)
{
	snd_pcm_sframes_t avail;
	avail = hw_ptr - appl_ptr;
	if (avail < 0)
		avail += pcm->boundary;
	return avail;
}

static inline snd_pcm_uframes_t snd_pcm_mmap_capture_avail(snd_pcm_t *pcm)
{
	return __snd_pcm_capture_avail(pcm, *pcm->hw.ptr, *pcm->appl.ptr);
}

static inline snd_pcm_uframes_t __snd_pcm_avail(snd_pcm_t *pcm,
						const snd_pcm_uframes_t hw_ptr,
						const snd_pcm_uframes_t appl_ptr)
{
	if (pcm->stream == SND_PCM_STREAM_PLAYBACK)
		return __snd_pcm_playback_avail(pcm, hw_ptr, appl_ptr);
	else
		return __snd_pcm_capture_avail(pcm, hw_ptr, appl_ptr);
}

static inline snd_pcm_uframes_t snd_pcm_mmap_avail(snd_pcm_t *pcm)
{
	return __snd_pcm_avail(pcm, *pcm->hw.ptr, *pcm->appl.ptr);
}

/*
 * \retval number of frames available to the hardware for playback
 *
 * ie. the filled part of the ring buffer
 */
static inline snd_pcm_sframes_t snd_pcm_mmap_playback_hw_avail(snd_pcm_t *pcm)
{
	return pcm->buffer_size - snd_pcm_mmap_playback_avail(pcm);
}

/*
 * \retval number of frames available to the hardware for capture
 *
 * ie. the empty part of the ring buffer.
 */
static inline snd_pcm_sframes_t snd_pcm_mmap_capture_hw_avail(snd_pcm_t *pcm)
{
	return pcm->buffer_size - snd_pcm_mmap_capture_avail(pcm);
}

static inline snd_pcm_sframes_t snd_pcm_mmap_hw_avail(snd_pcm_t *pcm)
{
	return pcm->buffer_size - snd_pcm_mmap_avail(pcm);
}

static inline snd_pcm_sframes_t snd_pcm_mmap_playback_hw_rewindable(snd_pcm_t *pcm)
{
	snd_pcm_sframes_t ret = snd_pcm_mmap_playback_hw_avail(pcm);
	return (ret >= 0) ? ret : 0;
}

static inline snd_pcm_sframes_t snd_pcm_mmap_capture_hw_rewindable(snd_pcm_t *pcm)
{
	snd_pcm_sframes_t ret = snd_pcm_mmap_capture_hw_avail(pcm);
	return (ret >= 0) ? ret : 0;
}

static inline snd_pcm_uframes_t snd_pcm_mmap_hw_rewindable(snd_pcm_t *pcm)
{
	snd_pcm_sframes_t ret = snd_pcm_mmap_hw_avail(pcm);
	return (ret >= 0) ? ret : 0;
}

static inline const snd_pcm_channel_area_t *snd_pcm_mmap_areas(snd_pcm_t *pcm)
{
	if (pcm->stopped_areas &&
	    __snd_pcm_state(pcm) != SND_PCM_STATE_RUNNING)
		return pcm->stopped_areas;
	return pcm->running_areas;
}

static inline snd_pcm_uframes_t snd_pcm_mmap_offset(snd_pcm_t *pcm)
{
        assert(pcm);
	return *pcm->appl.ptr % pcm->buffer_size;
}

static inline snd_pcm_uframes_t snd_pcm_mmap_hw_offset(snd_pcm_t *pcm)
{
        assert(pcm);
	return *pcm->hw.ptr % pcm->buffer_size;
}

/*
 * \retval number of frames pending from application to hardware
 */
static inline snd_pcm_uframes_t snd_pcm_mmap_playback_delay(snd_pcm_t *pcm)
{
	return snd_pcm_mmap_playback_hw_avail(pcm);
}

/*
 * \retval number of frames pending from hardware to application
 */
static inline snd_pcm_uframes_t snd_pcm_mmap_capture_delay(snd_pcm_t *pcm)
{
	return snd_pcm_mmap_capture_avail(pcm);
}

static inline snd_pcm_sframes_t snd_pcm_mmap_delay(snd_pcm_t *pcm)
{
	if (pcm->stream == SND_PCM_STREAM_PLAYBACK)
		return snd_pcm_mmap_playback_delay(pcm);
	else
		return snd_pcm_mmap_capture_delay(pcm);
}

static inline snd_pcm_sframes_t _snd_pcm_writei(snd_pcm_t *pcm, const void *buffer, snd_pcm_uframes_t size)
{
	/* lock handled in the callback */
	if (!pcm->fast_ops->writei)
		return -ENOSYS;
	return pcm->fast_ops->writei(pcm->fast_op_arg, buffer, size);
}

static inline snd_pcm_sframes_t _snd_pcm_writen(snd_pcm_t *pcm, void **bufs, snd_pcm_uframes_t size)
{
	/* lock handled in the callback */
	if (!pcm->fast_ops->writen)
		return -ENOSYS;
	return pcm->fast_ops->writen(pcm->fast_op_arg, bufs, size);
}

static inline snd_pcm_sframes_t _snd_pcm_readi(snd_pcm_t *pcm, void *buffer, snd_pcm_uframes_t size)
{
	/* lock handled in the callback */
	if (!pcm->fast_ops->readi)
		return -ENOSYS;
	return pcm->fast_ops->readi(pcm->fast_op_arg, buffer, size);
}

static inline snd_pcm_sframes_t _snd_pcm_readn(snd_pcm_t *pcm, void **bufs, snd_pcm_uframes_t size)
{
	/* lock handled in the callback */
	if (!pcm->fast_ops->readn)
		return -ENOSYS;
	return pcm->fast_ops->readn(pcm->fast_op_arg, bufs, size);
}

static inline int muldiv(int a, int b, int c, int *r)
{
	int64_t n = (int64_t)a * b;
	int64_t v = n / c;
	if (v > INT_MAX) {
		*r = 0;
		return INT_MAX;
	}
	if (v < INT_MIN) {
		*r = 0;
		return INT_MIN;
	}
	*r = n % c;
	return v;
}

static inline int muldiv_down(int a, int b, int c)
{
	int64_t v = (int64_t)a * b / c;
	if (v > INT_MAX) {
		return INT_MAX;
	}
	if (v < INT_MIN) {
		return INT_MIN;
	}
	return v;
}

static inline int muldiv_near(int a, int b, int c)
{
	int r;
	int n = muldiv(a, b, c, &r);
	if (r >= (c + 1) / 2)
		n++;
	return n;
}

int snd_pcm_hw_refine(snd_pcm_t *pcm, snd_pcm_hw_params_t *params);
int _snd_pcm_hw_params_internal(snd_pcm_t *pcm, snd_pcm_hw_params_t *params);
#undef _snd_pcm_hw_params
int snd_pcm_hw_refine_soft(snd_pcm_t *pcm, snd_pcm_hw_params_t *params);
int snd_pcm_hw_refine_slave(snd_pcm_t *pcm, snd_pcm_hw_params_t *params,
			    int (*cprepare)(snd_pcm_t *pcm,
					    snd_pcm_hw_params_t *params),
			    int (*cchange)(snd_pcm_t *pcm,
					   snd_pcm_hw_params_t *params,
					   snd_pcm_hw_params_t *sparams),
			    int (*sprepare)(snd_pcm_t *pcm,
					    snd_pcm_hw_params_t *params),
			    int (*schange)(snd_pcm_t *pcm,
					   snd_pcm_hw_params_t *params,
					   snd_pcm_hw_params_t *sparams),
			    int (*srefine)(snd_pcm_t *pcm,
					   snd_pcm_hw_params_t *sparams));
int snd_pcm_hw_params_slave(snd_pcm_t *pcm, snd_pcm_hw_params_t *params,
			    int (*cchange)(snd_pcm_t *pcm,
					   snd_pcm_hw_params_t *params,
					   snd_pcm_hw_params_t *sparams),
			    int (*sprepare)(snd_pcm_t *pcm,
					    snd_pcm_hw_params_t *params),
			    int (*schange)(snd_pcm_t *pcm,
					   snd_pcm_hw_params_t *params,
					   snd_pcm_hw_params_t *sparams),
			    int (*sparams)(snd_pcm_t *pcm,
					   snd_pcm_hw_params_t *sparams));


void _snd_pcm_hw_params_any(snd_pcm_hw_params_t *params);
void _snd_pcm_hw_param_set_empty(snd_pcm_hw_params_t *params,
				 snd_pcm_hw_param_t var);
int _snd_pcm_hw_param_set_interval(snd_pcm_hw_params_t *params,
				   snd_pcm_hw_param_t var,
				   const snd_interval_t *val);
int _snd_pcm_hw_param_set_mask(snd_pcm_hw_params_t *params,
			   snd_pcm_hw_param_t var, const snd_mask_t *mask);
int _snd_pcm_hw_param_first(snd_pcm_hw_params_t *params,
			    snd_pcm_hw_param_t var);
int _snd_pcm_hw_param_last(snd_pcm_hw_params_t *params,
			   snd_pcm_hw_param_t var);
int _snd_pcm_hw_param_set(snd_pcm_hw_params_t *params,
			  snd_pcm_hw_param_t var, unsigned int val, int dir);
static inline int _snd_pcm_hw_params_set_format(snd_pcm_hw_params_t *params,
						snd_pcm_format_t val)
{
	return _snd_pcm_hw_param_set(params, SND_PCM_HW_PARAM_FORMAT,
				     (unsigned long) val, 0);
}

static inline int _snd_pcm_hw_params_set_subformat(snd_pcm_hw_params_t *params,
				     snd_pcm_subformat_t val)
{
	return _snd_pcm_hw_param_set(params, SND_PCM_HW_PARAM_SUBFORMAT,
				     (unsigned long) val, 0);
}

int _snd_pcm_hw_param_set_min(snd_pcm_hw_params_t *params,
			      snd_pcm_hw_param_t var, unsigned int val, int dir);
int _snd_pcm_hw_param_set_max(snd_pcm_hw_params_t *params,
			      snd_pcm_hw_param_t var, unsigned int val, int dir);
int _snd_pcm_hw_param_set_minmax(snd_pcm_hw_params_t *params,
				 snd_pcm_hw_param_t var,
				 unsigned int min, int mindir,
				 unsigned int max, int maxdir);
int _snd_pcm_hw_param_refine(snd_pcm_hw_params_t *params,
			     snd_pcm_hw_param_t var,
			     const snd_pcm_hw_params_t *src);
int _snd_pcm_hw_params_refine(snd_pcm_hw_params_t *params,
			      unsigned int vars,
			      const snd_pcm_hw_params_t *src);
int snd_pcm_hw_param_refine_near(snd_pcm_t *pcm,
				 snd_pcm_hw_params_t *params,
				 snd_pcm_hw_param_t var,
				 const snd_pcm_hw_params_t *src);
int snd_pcm_hw_param_refine_multiple(snd_pcm_t *pcm,
				     snd_pcm_hw_params_t *params,
				     snd_pcm_hw_param_t var,
				     const snd_pcm_hw_params_t *src);
int snd_pcm_hw_param_empty(const snd_pcm_hw_params_t *params,
			   snd_pcm_hw_param_t var);
int snd_pcm_hw_param_always_eq(const snd_pcm_hw_params_t *params,
			       snd_pcm_hw_param_t var,
			       const snd_pcm_hw_params_t *params1);
int snd_pcm_hw_param_never_eq(const snd_pcm_hw_params_t *params,
			      snd_pcm_hw_param_t var,
			      const snd_pcm_hw_params_t *params1);
const snd_mask_t *snd_pcm_hw_param_get_mask(const snd_pcm_hw_params_t *params,
					      snd_pcm_hw_param_t var);
const snd_interval_t *snd_pcm_hw_param_get_interval(const snd_pcm_hw_params_t *params,
						      snd_pcm_hw_param_t var);

int snd_pcm_hw_param_any(snd_pcm_t *pcm, snd_pcm_hw_params_t *params,
			 snd_pcm_hw_param_t var);
int snd_pcm_hw_param_set_integer(snd_pcm_t *pcm, snd_pcm_hw_params_t *params,
				 snd_set_mode_t mode,
				 snd_pcm_hw_param_t var);
int snd_pcm_hw_param_set_first(snd_pcm_t *pcm, snd_pcm_hw_params_t *params,
			       snd_pcm_hw_param_t var, unsigned int *rval, int *dir);
int snd_pcm_hw_param_set_last(snd_pcm_t *pcm, snd_pcm_hw_params_t *params,
			      snd_pcm_hw_param_t var, unsigned int *rval, int *dir);
int snd_pcm_hw_param_set_near(snd_pcm_t *pcm, snd_pcm_hw_params_t *params,
			      snd_pcm_hw_param_t var, unsigned int *val, int *dir);
int snd_pcm_hw_param_set_min(snd_pcm_t *pcm, snd_pcm_hw_params_t *params,
			     snd_set_mode_t mode,
			     snd_pcm_hw_param_t var,
			     unsigned int *val, int *dir);
int snd_pcm_hw_param_set_max(snd_pcm_t *pcm, snd_pcm_hw_params_t *params,
			     snd_set_mode_t mode,
			     snd_pcm_hw_param_t var, unsigned int *val, int *dir);
int snd_pcm_hw_param_set_minmax(snd_pcm_t *pcm, snd_pcm_hw_params_t *params,
				snd_set_mode_t mode,
				snd_pcm_hw_param_t var,
				unsigned int *min, int *mindir,
				unsigned int *max, int *maxdir);
int snd_pcm_hw_param_set(snd_pcm_t *pcm, snd_pcm_hw_params_t *params,
			 snd_set_mode_t mode,
			 snd_pcm_hw_param_t var, unsigned int val, int dir);
int snd_pcm_hw_param_set_mask(snd_pcm_t *pcm, snd_pcm_hw_params_t *params,
			      snd_set_mode_t mode,
			      snd_pcm_hw_param_t var, const snd_mask_t *mask);
int snd_pcm_hw_param_get(const snd_pcm_hw_params_t *params, snd_pcm_hw_param_t var,
			 unsigned int *val, int *dir);
int snd_pcm_hw_param_get_min(const snd_pcm_hw_params_t *params,
			     snd_pcm_hw_param_t var,
			     unsigned int *val, int *dir);
int snd_pcm_hw_param_get_max(const snd_pcm_hw_params_t *params,
			     snd_pcm_hw_param_t var,
			     unsigned int *val, int *dir);

#ifdef INTERNAL
snd_pcm_sframes_t INTERNAL(snd_pcm_forward)(snd_pcm_t *pcm, snd_pcm_uframes_t frames);

int INTERNAL(snd_pcm_hw_params_get_access)(const snd_pcm_hw_params_t *params, snd_pcm_access_t *access);
int snd_pcm_hw_params_test_access(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, snd_pcm_access_t access);
int snd_pcm_hw_params_set_access(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, snd_pcm_access_t access);
int INTERNAL(snd_pcm_hw_params_set_access_first)(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, snd_pcm_access_t *access);
int INTERNAL(snd_pcm_hw_params_set_access_last)(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, snd_pcm_access_t *access);
int snd_pcm_hw_params_set_access_mask(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, snd_pcm_access_mask_t *mask);
int snd_pcm_hw_params_get_access_mask(snd_pcm_hw_params_t *params, snd_pcm_access_mask_t *mask);

int INTERNAL(snd_pcm_hw_params_get_format)(const snd_pcm_hw_params_t *params, snd_pcm_format_t *val);
int snd_pcm_hw_params_test_format(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, snd_pcm_format_t val);
int snd_pcm_hw_params_set_format(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, snd_pcm_format_t val);
int INTERNAL(snd_pcm_hw_params_set_format_first)(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, snd_pcm_format_t *format);
int INTERNAL(snd_pcm_hw_params_set_format_last)(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, snd_pcm_format_t *format);
int snd_pcm_hw_params_set_format_mask(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, snd_pcm_format_mask_t *mask);
void snd_pcm_hw_params_get_format_mask(snd_pcm_hw_params_t *params, snd_pcm_format_mask_t *mask);

int INTERNAL(snd_pcm_hw_params_get_subformat)(const snd_pcm_hw_params_t *params, snd_pcm_subformat_t *subformat);
int snd_pcm_hw_params_test_subformat(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, snd_pcm_subformat_t subformat);
int snd_pcm_hw_params_set_subformat(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, snd_pcm_subformat_t subformat);
int INTERNAL(snd_pcm_hw_params_set_subformat_first)(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, snd_pcm_subformat_t *subformat);
int INTERNAL(snd_pcm_hw_params_set_subformat_last)(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, snd_pcm_subformat_t *subformat);
int snd_pcm_hw_params_set_subformat_mask(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, snd_pcm_subformat_mask_t *mask);
void snd_pcm_hw_params_get_subformat_mask(snd_pcm_hw_params_t *params, snd_pcm_subformat_mask_t *mask);

int INTERNAL(snd_pcm_hw_params_get_channels)(const snd_pcm_hw_params_t *params, unsigned int *val);
int INTERNAL(snd_pcm_hw_params_get_channels_min)(const snd_pcm_hw_params_t *params, unsigned int *val);
int INTERNAL(snd_pcm_hw_params_get_channels_max)(const snd_pcm_hw_params_t *params, unsigned int *val);
int snd_pcm_hw_params_test_channels(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, unsigned int val);
int snd_pcm_hw_params_set_channels(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, unsigned int val);
int snd_pcm_hw_params_set_channels_min(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, unsigned int *val);
int snd_pcm_hw_params_set_channels_max(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, unsigned int *val);
int snd_pcm_hw_params_set_channels_minmax(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, unsigned int *min, unsigned int *max);
int INTERNAL(snd_pcm_hw_params_set_channels_near)(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, unsigned int *val);
int INTERNAL(snd_pcm_hw_params_set_channels_first)(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, unsigned int *val);
int INTERNAL(snd_pcm_hw_params_set_channels_last)(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, unsigned int *val);

int INTERNAL(snd_pcm_hw_params_get_rate)(const snd_pcm_hw_params_t *params, unsigned int *val, int *dir);
int INTERNAL(snd_pcm_hw_params_get_rate_min)(const snd_pcm_hw_params_t *params, unsigned int *val, int *dir);
int INTERNAL(snd_pcm_hw_params_get_rate_max)(const snd_pcm_hw_params_t *params, unsigned int *val, int *dir);
int snd_pcm_hw_params_test_rate(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, unsigned int val, int dir);
int snd_pcm_hw_params_set_rate(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, unsigned int val, int dir);
int snd_pcm_hw_params_set_rate_min(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, unsigned int *val, int *dir);
int snd_pcm_hw_params_set_rate_max(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, unsigned int *val, int *dir);
int snd_pcm_hw_params_set_rate_minmax(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, unsigned int *min, int *mindir, unsigned int *max, int *maxdir);
int INTERNAL(snd_pcm_hw_params_set_rate_near)(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, unsigned int *val, int *dir);
int INTERNAL(snd_pcm_hw_params_set_rate_first)(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, unsigned int *val, int *dir);
int INTERNAL(snd_pcm_hw_params_set_rate_last)(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, unsigned int *val, int *dir);

int INTERNAL(snd_pcm_hw_params_get_period_time)(const snd_pcm_hw_params_t *params, unsigned int *val, int *dir);
int INTERNAL(snd_pcm_hw_params_get_period_time_min)(const snd_pcm_hw_params_t *params, unsigned int *val, int *dir);
int INTERNAL(snd_pcm_hw_params_get_period_time_max)(const snd_pcm_hw_params_t *params, unsigned int *val, int *dir);
int snd_pcm_hw_params_test_period_time(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, unsigned int val, int dir);
int snd_pcm_hw_params_set_period_time(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, unsigned int val, int dir);
int snd_pcm_hw_params_set_period_time_min(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, unsigned int *val, int *dir);
int snd_pcm_hw_params_set_period_time_max(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, unsigned int *val, int *dir);
int snd_pcm_hw_params_set_period_time_minmax(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, unsigned int *min, int *mindir, unsigned int *max, int *maxdir);
int INTERNAL(snd_pcm_hw_params_set_period_time_near)(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, unsigned int *val, int *dir);
int INTERNAL(snd_pcm_hw_params_set_period_time_first)(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, unsigned int *val, int *dir);
int INTERNAL(snd_pcm_hw_params_set_period_time_last)(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, unsigned int *val, int *dir);

int INTERNAL(snd_pcm_hw_params_get_period_size)(const snd_pcm_hw_params_t *params, snd_pcm_uframes_t *frames, int *dir);
int INTERNAL(snd_pcm_hw_params_get_period_size_min)(const snd_pcm_hw_params_t *params, snd_pcm_uframes_t *frames, int *dir);
int INTERNAL(snd_pcm_hw_params_get_period_size_max)(const snd_pcm_hw_params_t *params, snd_pcm_uframes_t *frames, int *dir);
int snd_pcm_hw_params_test_period_size(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, snd_pcm_uframes_t val, int dir);
int snd_pcm_hw_params_set_period_size(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, snd_pcm_uframes_t val, int dir);
int snd_pcm_hw_params_set_period_size_min(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, snd_pcm_uframes_t *val, int *dir);
int snd_pcm_hw_params_set_period_size_max(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, snd_pcm_uframes_t *val, int *dir);
int snd_pcm_hw_params_set_period_size_minmax(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, snd_pcm_uframes_t *min, int *mindir, snd_pcm_uframes_t *max, int *maxdir);
int INTERNAL(snd_pcm_hw_params_set_period_size_near)(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, snd_pcm_uframes_t *val, int *dir);
int INTERNAL(snd_pcm_hw_params_set_period_size_first)(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, snd_pcm_uframes_t *val, int *dir);
int INTERNAL(snd_pcm_hw_params_set_period_size_last)(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, snd_pcm_uframes_t *val, int *dir);
int snd_pcm_hw_params_set_period_size_integer(snd_pcm_t *pcm, snd_pcm_hw_params_t *params);

int INTERNAL(snd_pcm_hw_params_get_periods)(const snd_pcm_hw_params_t *params, unsigned int *val, int *dir);
int INTERNAL(snd_pcm_hw_params_get_periods_min)(const snd_pcm_hw_params_t *params, unsigned int *val, int *dir);
int INTERNAL(snd_pcm_hw_params_get_periods_max)(const snd_pcm_hw_params_t *params, unsigned int *val, int *dir);
int snd_pcm_hw_params_test_periods(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, unsigned int val, int dir);
int snd_pcm_hw_params_set_periods(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, unsigned int val, int dir);
int snd_pcm_hw_params_set_periods_min(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, unsigned int *val, int *dir);
int snd_pcm_hw_params_set_periods_max(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, unsigned int *val, int *dir);
int snd_pcm_hw_params_set_periods_minmax(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, unsigned int *min, int *mindir, unsigned int *max, int *maxdir);
int INTERNAL(snd_pcm_hw_params_set_periods_near)(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, unsigned int *val, int *dir);
int INTERNAL(snd_pcm_hw_params_set_periods_first)(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, unsigned int *val, int *dir);
int INTERNAL(snd_pcm_hw_params_set_periods_last)(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, unsigned int *val, int *dir);
int snd_pcm_hw_params_set_periods_integer(snd_pcm_t *pcm, snd_pcm_hw_params_t *params);

int INTERNAL(snd_pcm_hw_params_get_buffer_time)(const snd_pcm_hw_params_t *params, unsigned int *val, int *dir);
int INTERNAL(snd_pcm_hw_params_get_buffer_time_min)(const snd_pcm_hw_params_t *params, unsigned int *val, int *dir);
int INTERNAL(snd_pcm_hw_params_get_buffer_time_max)(const snd_pcm_hw_params_t *params, unsigned int *val, int *dir);
int snd_pcm_hw_params_test_buffer_time(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, unsigned int val, int dir);
int snd_pcm_hw_params_set_buffer_time(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, unsigned int val, int dir);
int snd_pcm_hw_params_set_buffer_time_min(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, unsigned int *val, int *dir);
int snd_pcm_hw_params_set_buffer_time_max(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, unsigned int *val, int *dir);
int snd_pcm_hw_params_set_buffer_time_minmax(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, unsigned int *min, int *mindir, unsigned int *max, int *maxdir);
int INTERNAL(snd_pcm_hw_params_set_buffer_time_near)(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, unsigned int *val, int *dir);
int INTERNAL(snd_pcm_hw_params_set_buffer_time_first)(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, unsigned int *val, int *dir);
int INTERNAL(snd_pcm_hw_params_set_buffer_time_last)(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, unsigned int *val, int *dir);

int INTERNAL(snd_pcm_hw_params_get_buffer_size)(const snd_pcm_hw_params_t *params, snd_pcm_uframes_t *val);
int INTERNAL(snd_pcm_hw_params_get_buffer_size_min)(const snd_pcm_hw_params_t *params, snd_pcm_uframes_t *val);
int INTERNAL(snd_pcm_hw_params_get_buffer_size_max)(const snd_pcm_hw_params_t *params, snd_pcm_uframes_t *val);
int snd_pcm_hw_params_test_buffer_size(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, snd_pcm_uframes_t val);
int snd_pcm_hw_params_set_buffer_size(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, snd_pcm_uframes_t val);
int snd_pcm_hw_params_set_buffer_size_min(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, snd_pcm_uframes_t *val);
int snd_pcm_hw_params_set_buffer_size_max(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, snd_pcm_uframes_t *val);
int snd_pcm_hw_params_set_buffer_size_minmax(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, snd_pcm_uframes_t *min, snd_pcm_uframes_t *max);
int INTERNAL(snd_pcm_hw_params_set_buffer_size_near)(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, snd_pcm_uframes_t *val);
int INTERNAL(snd_pcm_hw_params_set_buffer_size_first)(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, snd_pcm_uframes_t *val);
int INTERNAL(snd_pcm_hw_params_set_buffer_size_last)(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, snd_pcm_uframes_t *val);

int snd_pcm_sw_params_set_tstamp_mode(snd_pcm_t *pcm, snd_pcm_sw_params_t *params, snd_pcm_tstamp_t val);
int INTERNAL(snd_pcm_sw_params_get_tstamp_mode)(const snd_pcm_sw_params_t *params, snd_pcm_tstamp_t *val);
int snd_pcm_sw_params_set_tstamp_type(snd_pcm_t *pcm, snd_pcm_sw_params_t *params, snd_pcm_tstamp_type_t val);
int snd_pcm_sw_params_get_tstamp_type(const snd_pcm_sw_params_t *params, snd_pcm_tstamp_type_t *val);
int snd_pcm_sw_params_set_avail_min(snd_pcm_t *pcm, snd_pcm_sw_params_t *params, snd_pcm_uframes_t val);
int INTERNAL(snd_pcm_sw_params_get_avail_min)(const snd_pcm_sw_params_t *params, snd_pcm_uframes_t *val);
int snd_pcm_sw_params_set_start_threshold(snd_pcm_t *pcm, snd_pcm_sw_params_t *params, snd_pcm_uframes_t val);
int INTERNAL(snd_pcm_sw_params_get_start_threshold)(const snd_pcm_sw_params_t *paramsm, snd_pcm_uframes_t *val);
int snd_pcm_sw_params_set_stop_threshold(snd_pcm_t *pcm, snd_pcm_sw_params_t *params, snd_pcm_uframes_t val);
int INTERNAL(snd_pcm_sw_params_get_stop_threshold)(const snd_pcm_sw_params_t *params, snd_pcm_uframes_t *val);
int snd_pcm_sw_params_set_silence_threshold(snd_pcm_t *pcm, snd_pcm_sw_params_t *params, snd_pcm_uframes_t val);
int INTERNAL(snd_pcm_sw_params_get_silence_threshold)(const snd_pcm_sw_params_t *params, snd_pcm_uframes_t *val);
int snd_pcm_sw_params_set_silence_size(snd_pcm_t *pcm, snd_pcm_sw_params_t *params, snd_pcm_uframes_t val);
int INTERNAL(snd_pcm_sw_params_get_silence_size)(const snd_pcm_sw_params_t *params, snd_pcm_uframes_t *val);
#endif /* INTERNAL */

const char *snd_pcm_hw_param_name(snd_pcm_hw_param_t param);
void snd_pcm_hw_param_dump(const snd_pcm_hw_params_t *params,
			   snd_pcm_hw_param_t var, snd_output_t *out);
#if 0
int snd_pcm_hw_strategy_simple_near(snd_pcm_hw_strategy_t *strategy, int order,
				    snd_pcm_hw_param_t var,
				    unsigned int best,
				    unsigned int mul);
int snd_pcm_hw_strategy_simple_choices(snd_pcm_hw_strategy_t *strategy, int order,
				       snd_pcm_hw_param_t var,
				       unsigned int count,
				       snd_pcm_hw_strategy_simple_choices_list_t *choices);
#endif

#define SCONF_MANDATORY	1
#define SCONF_UNCHANGED	2

int snd_pcm_slave_conf(snd_config_t *root, snd_config_t *conf,
		       snd_config_t **pcm_conf, unsigned int count, ...);

#define SND_PCM_APPEND	(1<<8)

int snd_pcm_open_named_slave(snd_pcm_t **pcmp, const char *name,
			     snd_config_t *root,
			     snd_config_t *conf, snd_pcm_stream_t stream,
			     int mode, snd_config_t *parent_conf);
static inline int
snd_pcm_open_slave(snd_pcm_t **pcmp, snd_config_t *root,
		   snd_config_t *conf, snd_pcm_stream_t stream,
		   int mode, snd_config_t *parent_conf)
{
	return snd_pcm_open_named_slave(pcmp, NULL, root, conf, stream,
					mode, parent_conf);
}

#define snd_pcm_conf_generic_id(id) _snd_conf_generic_id(id)

int snd_pcm_hw_open_fd(snd_pcm_t **pcmp, const char *name, int fd,
		       int sync_ptr_ioctl);
int __snd_pcm_mmap_emul_open(snd_pcm_t **pcmp, const char *name,
			     snd_pcm_t *slave, int close_slave);

int snd_pcm_wait_nocheck(snd_pcm_t *pcm, int timeout);

const snd_config_t *snd_pcm_rate_get_default_converter(snd_config_t *root);

#define SND_PCM_HW_PARBIT_ACCESS	(1U << SND_PCM_HW_PARAM_ACCESS)
#define SND_PCM_HW_PARBIT_FORMAT	(1U << SND_PCM_HW_PARAM_FORMAT)
#define SND_PCM_HW_PARBIT_SUBFORMAT	(1U << SND_PCM_HW_PARAM_SUBFORMAT)
#define SND_PCM_HW_PARBIT_CHANNELS	(1U << SND_PCM_HW_PARAM_CHANNELS)
#define SND_PCM_HW_PARBIT_RATE		(1U << SND_PCM_HW_PARAM_RATE)
#define SND_PCM_HW_PARBIT_PERIOD_TIME	(1U << SND_PCM_HW_PARAM_PERIOD_TIME)
#define SND_PCM_HW_PARBIT_PERIOD_SIZE	(1U << SND_PCM_HW_PARAM_PERIOD_SIZE)
#define SND_PCM_HW_PARBIT_PERIODS	(1U << SND_PCM_HW_PARAM_PERIODS)
#define SND_PCM_HW_PARBIT_BUFFER_TIME	(1U << SND_PCM_HW_PARAM_BUFFER_TIME)
#define SND_PCM_HW_PARBIT_BUFFER_SIZE	(1U << SND_PCM_HW_PARAM_BUFFER_SIZE)
#define SND_PCM_HW_PARBIT_SAMPLE_BITS	(1U << SND_PCM_HW_PARAM_SAMPLE_BITS)
#define SND_PCM_HW_PARBIT_FRAME_BITS	(1U << SND_PCM_HW_PARAM_FRAME_BITS)
#define SND_PCM_HW_PARBIT_PERIOD_BYTES	(1U << SND_PCM_HW_PARAM_PERIOD_BYTES)
#define SND_PCM_HW_PARBIT_BUFFER_BYTES	(1U << SND_PCM_HW_PARAM_BUFFER_BYTES)
#define SND_PCM_HW_PARBIT_TICK_TIME	(1U << SND_PCM_HW_PARAM_TICK_TIME)


#define SND_PCM_ACCBIT_MMAP { ((1U << SND_PCM_ACCESS_MMAP_INTERLEAVED) | \
			     (1U << SND_PCM_ACCESS_MMAP_NONINTERLEAVED) | \
			     (1U << SND_PCM_ACCESS_MMAP_COMPLEX)) }
#define SND_PCM_ACCBIT_MMAPI { (1U << SND_PCM_ACCESS_MMAP_INTERLEAVED) }
#define SND_PCM_ACCBIT_MMAPN { (1U << SND_PCM_ACCESS_MMAP_NONINTERLEAVED) }
#define SND_PCM_ACCBIT_MMAPC { (1U << SND_PCM_ACCESS_MMAP_COMPLEX) }

#define SND_PCM_ACCBIT_SHM { ((1U << SND_PCM_ACCESS_MMAP_INTERLEAVED) | \
			    (1U << SND_PCM_ACCESS_RW_INTERLEAVED) | \
			    (1U << SND_PCM_ACCESS_MMAP_NONINTERLEAVED) | \
			    (1U << SND_PCM_ACCESS_RW_NONINTERLEAVED)) }
#define SND_PCM_ACCBIT_SHMI { ((1U << SND_PCM_ACCESS_MMAP_INTERLEAVED) | \
			     (1U << SND_PCM_ACCESS_RW_INTERLEAVED)) }
#define SND_PCM_ACCBIT_SHMN { ((1U << SND_PCM_ACCESS_MMAP_NONINTERLEAVED) | \
			     (1U << SND_PCM_ACCESS_RW_NONINTERLEAVED)) }

#define SND_PCM_FMTBIT_LINEAR \
	{ ((1U << SND_PCM_FORMAT_S8) | \
	 (1U << SND_PCM_FORMAT_U8) | \
	 (1U << SND_PCM_FORMAT_S16_LE) | \
	 (1U << SND_PCM_FORMAT_S16_BE) | \
	 (1U << SND_PCM_FORMAT_U16_LE) | \
	 (1U << SND_PCM_FORMAT_U16_BE) | \
	 (1U << SND_PCM_FORMAT_S20_LE) | \
	 (1U << SND_PCM_FORMAT_S20_BE) | \
	 (1U << SND_PCM_FORMAT_U20_LE) | \
	 (1U << SND_PCM_FORMAT_U20_BE) | \
	 (1U << SND_PCM_FORMAT_S24_LE) | \
	 (1U << SND_PCM_FORMAT_S24_BE) | \
	 (1U << SND_PCM_FORMAT_U24_LE) | \
	 (1U << SND_PCM_FORMAT_U24_BE) | \
	 (1U << SND_PCM_FORMAT_S32_LE) | \
	 (1U << SND_PCM_FORMAT_S32_BE) | \
	 (1U << SND_PCM_FORMAT_U32_LE) | \
	 (1U << SND_PCM_FORMAT_U32_BE)), \
	((1U << (SND_PCM_FORMAT_S24_3LE - 32)) | \
	 (1U << (SND_PCM_FORMAT_U24_3LE - 32)) | \
	 (1U << (SND_PCM_FORMAT_S24_3BE - 32)) | \
	 (1U << (SND_PCM_FORMAT_U24_3BE - 32)) | \
	 (1U << (SND_PCM_FORMAT_S20_3LE - 32)) | \
	 (1U << (SND_PCM_FORMAT_U20_3LE - 32)) | \
	 (1U << (SND_PCM_FORMAT_S20_3BE - 32)) | \
	 (1U << (SND_PCM_FORMAT_U20_3BE - 32)) | \
	 (1U << (SND_PCM_FORMAT_S18_3LE - 32)) | \
	 (1U << (SND_PCM_FORMAT_U18_3LE - 32)) | \
	 (1U << (SND_PCM_FORMAT_S18_3BE - 32)) | \
	 (1U << (SND_PCM_FORMAT_U18_3BE - 32))) }
	

#define SND_PCM_FMTBIT_FLOAT \
	{ ((1U << SND_PCM_FORMAT_FLOAT_LE) | \
	 (1U << SND_PCM_FORMAT_FLOAT_BE) | \
	 (1U << SND_PCM_FORMAT_FLOAT64_LE) | \
	 (1U << SND_PCM_FORMAT_FLOAT64_BE)) }


typedef union snd_tmp_float {
	float f;
	int32_t i;
} snd_tmp_float_t;

typedef union snd_tmp_double {
	double d;
	int64_t l;
} snd_tmp_double_t;

/* get the current timestamp */
#ifdef HAVE_CLOCK_GETTIME
static inline void gettimestamp(snd_htimestamp_t *tstamp,
				snd_pcm_tstamp_type_t tstamp_type)
{
	clockid_t id;

	switch (tstamp_type) {
#ifdef CLOCK_MONOTONIC_RAW
	case SND_PCM_TSTAMP_TYPE_MONOTONIC_RAW:
		id = CLOCK_MONOTONIC_RAW;
		break;
#endif
#ifdef CLOCK_MONOTONIC
	case SND_PCM_TSTAMP_TYPE_MONOTONIC:
		id = CLOCK_MONOTONIC;
		break;
#endif
	default:
		id = CLOCK_REALTIME;
		break;
	}
	clock_gettime(id, tstamp);
}
#else /* HAVE_CLOCK_GETTIME */
static inline void gettimestamp(snd_htimestamp_t *tstamp,
				snd_pcm_tstamp_type_t tstamp_type)
{
	struct timeval tv;

	gettimeofday(&tv, 0);
	tstamp->tv_sec = tv.tv_sec;
	tstamp->tv_nsec = tv.tv_usec * 1000L;
}
#endif /* HAVE_CLOCK_GETTIME */

snd_pcm_chmap_query_t **
_snd_pcm_make_single_query_chmaps(const snd_pcm_chmap_t *src);
snd_pcm_chmap_t *_snd_pcm_copy_chmap(const snd_pcm_chmap_t *src);
snd_pcm_chmap_query_t **
_snd_pcm_copy_chmap_query(snd_pcm_chmap_query_t * const *src);
snd_pcm_chmap_query_t **
_snd_pcm_parse_config_chmaps(snd_config_t *conf);
snd_pcm_chmap_t *
_snd_pcm_choose_fixed_chmap(snd_pcm_t *pcm, snd_pcm_chmap_query_t * const *maps);

/* return true if the PCM stream may wait to get avail_min space */
static inline int snd_pcm_may_wait_for_avail_min(snd_pcm_t *pcm, snd_pcm_uframes_t avail)
{
	if (avail >= pcm->avail_min)
		return 0;
	if (pcm->fast_ops->may_wait_for_avail_min)
		return pcm->fast_ops->may_wait_for_avail_min(pcm->fast_op_arg, avail);
	return 1;
}

/* hack to access to internal period_event in snd_pcm_sw_parmams */
static inline int sw_get_period_event(const snd_pcm_sw_params_t *params)
{
	return params->reserved[sizeof(params->reserved) / sizeof(params->reserved[0])- 1];
}

static inline void sw_set_period_event(snd_pcm_sw_params_t *params, int val)
{
	params->reserved[sizeof(params->reserved) / sizeof(params->reserved[0]) - 1] = val;
}

#define PCMINABORT(pcm) (((pcm)->mode & SND_PCM_ABORT) != 0)

static inline snd_pcm_sframes_t pcm_frame_diff(snd_pcm_uframes_t ptr1,
					       snd_pcm_uframes_t ptr2,
					       snd_pcm_uframes_t boundary)
{
	if (ptr1 < ptr2)
		return ptr1 + (boundary - ptr2);
	else
		return ptr1 - ptr2;
}

static inline snd_pcm_sframes_t pcm_frame_diff2(snd_pcm_uframes_t ptr1,
						snd_pcm_uframes_t ptr2,
						snd_pcm_uframes_t boundary)
{
	snd_pcm_sframes_t r = ptr1 - ptr2;
	if (r >= (snd_pcm_sframes_t)boundary / 2)
		return boundary - r;
	return r;
}

#ifdef THREAD_SAFE_API
/*
 * __snd_pcm_lock() and __snd_pcm_unlock() are used to lock/unlock the plugin
 * forcibly even if it's declared as thread-safe.  It's needed only for some
 * codes that are thread-unsafe per design (e.g. snd_pcm_nonblock()).
 *
 * OTOH, snd_pcm_lock() and snd_pcm_unlock() are used to lock/unlock the plugin
 * in normal situations.  They do lock/unlock only when the plugin is
 * thread-unsafe.
 *
 * Both __snd_pcm_lock() and snd_pcm_lock() (and their unlocks) wouldn't do
 * any action when the whole locking is disabled via $LIBASOUND_THREAD_SAFE=0.
 */
static inline void __snd_pcm_lock(snd_pcm_t *pcm)
{
	if (pcm->lock_enabled)
		pthread_mutex_lock(&pcm->lock);
}
static inline void __snd_pcm_unlock(snd_pcm_t *pcm)
{
	if (pcm->lock_enabled)
		pthread_mutex_unlock(&pcm->lock);
}
static inline void snd_pcm_lock(snd_pcm_t *pcm)
{
	if (pcm->lock_enabled && pcm->need_lock)
		pthread_mutex_lock(&pcm->lock);
}
static inline void snd_pcm_unlock(snd_pcm_t *pcm)
{
	if (pcm->lock_enabled && pcm->need_lock)
		pthread_mutex_unlock(&pcm->lock);
}
#else /* THREAD_SAFE_API */
#define __snd_pcm_lock(pcm)		do {} while (0)
#define __snd_pcm_unlock(pcm)		do {} while (0)
#define snd_pcm_lock(pcm)		do {} while (0)
#define snd_pcm_unlock(pcm)		do {} while (0)
#endif /* THREAD_SAFE_API */

#endif /* __PCM_LOCAL_H */
