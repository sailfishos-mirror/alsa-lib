/**
 * \file control/control.c
 * \author Abramo Bagnara <abramo@alsa-project.org>
 * \date 2000
 *
 * CTL interface is designed to access primitive controls.
 */
/*
 *  Control Interface - main file
 *  Copyright (c) 2000 by Abramo Bagnara <abramo@alsa-project.org>
 *
 *
 *   This library is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2 of
 *   the License, or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Library General Public License for more details.
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this library; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <signal.h>
#include <sys/poll.h>
#include "control_local.h"

/**
 * \brief get identifier of CTL handle
 * \param ctl CTL handle
 * \return ascii identifier of CTL handle
 *
 * Returns the ASCII identifier of given CTL handle. It's the same
 * identifier specified in snd_ctl_open().
 */
const char *snd_ctl_name(snd_ctl_t *ctl)
{
	assert(ctl);
	return ctl->name;
}

/**
 * \brief get type of CTL handle
 * \param ctl CTL handle
 * \return type of CTL handle
 *
 * Returns the type #snd_ctl_type_t of given CTL handle.
 */
snd_ctl_type_t snd_ctl_type(snd_ctl_t *ctl)
{
	assert(ctl);
	return ctl->type;
}

/**
 * \brief close CTL handle
 * \param ctl CTL handle
 * \return 0 on success otherwise a negative error code
 *
 * Closes the specified CTL handle and frees all associated
 * resources.
 */
int snd_ctl_close(snd_ctl_t *ctl)
{
	int res;
	res = ctl->ops->close(ctl);
	if (ctl->name)
		free(ctl->name);
	free(ctl);
	return res;
}

/**
 * \brief set nonblock mode
 * \param ctl CTL handle
 * \param nonblock 0 = block, 1 = nonblock mode
 * \return 0 on success otherwise a negative error code
 */
int snd_ctl_nonblock(snd_ctl_t *ctl, int nonblock)
{
	int err;
	assert(ctl);
	err = ctl->ops->nonblock(ctl, nonblock);
	if (err < 0)
		return err;
	ctl->nonblock = nonblock;
	return 0;
}

/**
 * \brief set async mode
 * \param ctl CTL handle
 * \param sig Signal to raise: < 0 disable, 0 default (SIGIO)
 * \param pid Process ID to signal: 0 current
 * \return 0 on success otherwise a negative error code
 *
 * A signal is raised when a change happens.
 */
int snd_ctl_async(snd_ctl_t *ctl, int sig, pid_t pid)
{
	int err;
	assert(ctl);
	err = ctl->ops->async(ctl, sig, pid);
	if (err < 0)
		return err;
	if (sig)
		ctl->async_sig = sig;
	else
		ctl->async_sig = SIGIO;
	if (pid)
		ctl->async_pid = pid;
	else
		ctl->async_pid = getpid();
	return 0;
}

/**
 * \brief get count of poll descriptors for CTL handle
 * \param ctl CTL handle
 * \return count of poll descriptors
 */
int snd_ctl_poll_descriptors_count(snd_ctl_t *ctl)
{
	assert(ctl);
	return 1;
}

/**
 * \brief get poll descriptors
 * \param ctl CTL handle
 * \param pfds array of poll descriptors
 * \param space space in the poll descriptor array
 * \return count of filled descriptors
 */
int snd_ctl_poll_descriptors(snd_ctl_t *ctl, struct pollfd *pfds, unsigned int space)
{
	assert(ctl);
	if (space > 0) {
		pfds->fd = ctl->ops->poll_descriptor(ctl);
		pfds->events = POLLIN;
		return 1;
	}
	return 0;
}

/**
 * \brief Ask to be informed about events (poll, #snd_ctl_async, #snd_ctl_read)
 * \param ctl CTL handle
 * \param subscribe 0 = unsubscribe, 1 = subscribe
 * \return 0 on success otherwise a negative error code
 */
int snd_ctl_subscribe_events(snd_ctl_t *ctl, int subscribe)
{
	assert(ctl);
	return ctl->ops->subscribe_events(ctl, subscribe);
}


/**
 * \brief Get card related information
 * \param ctl CTL handle
 * \param info Card info pointer
 * \return 0 on success otherwise a negative error code
 */
int snd_ctl_card_info(snd_ctl_t *ctl, snd_ctl_card_info_t *info)
{
	assert(ctl && info);
	return ctl->ops->card_info(ctl, info);
}

/**
 * \brief Get a list of element identificators
 * \param ctl CTL handle
 * \param list CTL element identificators list pointer
 * \return 0 on success otherwise a negative error code
 */
int snd_ctl_elem_list(snd_ctl_t *ctl, snd_ctl_elem_list_t *list)
{
	assert(ctl && list);
	assert(list->space == 0 || list->pids);
	return ctl->ops->element_list(ctl, list);
}

/**
 * \brief Get CTL element information
 * \param ctl CTL handle
 * \param info CTL element id/information pointer
 * \return 0 on success otherwise a negative error code
 */
int snd_ctl_elem_info(snd_ctl_t *ctl, snd_ctl_elem_info_t *info)
{
	assert(ctl && info && (info->id.name[0] || info->id.numid));
	return ctl->ops->element_info(ctl, info);
}

/**
 * \brief Get CTL element value
 * \param ctl CTL handle
 * \param control CTL element id/value pointer
 * \return 0 on success otherwise a negative error code
 */
int snd_ctl_elem_read(snd_ctl_t *ctl, snd_ctl_elem_value_t *control)
{
	assert(ctl && control && (control->id.name[0] || control->id.numid));
	return ctl->ops->element_read(ctl, control);
}

/**
 * \brief Set CTL element value
 * \param ctl CTL handle
 * \param control CTL element id/value pointer
 * \return 0 on success otherwise a negative error code
 */
int snd_ctl_elem_write(snd_ctl_t *ctl, snd_ctl_elem_value_t *control)
{
	assert(ctl && control && (control->id.name[0] || control->id.numid));
	return ctl->ops->element_write(ctl, control);
}

/**
 * \brief Lock CTL element
 * \param ctl CTL handle
 * \param control CTL element id pointer
 * \return 0 on success otherwise a negative error code
 */
int snd_ctl_elem_lock(snd_ctl_t *ctl, snd_ctl_elem_id_t *id)
{
	assert(ctl && id);
	return ctl->ops->element_lock(ctl, id);
}

/**
 * \brief Unlock CTL element
 * \param ctl CTL handle
 * \param control CTL element id pointer
 * \return 0 on success otherwise a negative error code
 */
int snd_ctl_elem_unlock(snd_ctl_t *ctl, snd_ctl_elem_id_t *id)
{
	assert(ctl && id);
	return ctl->ops->element_unlock(ctl, id);
}

/**
 * \brief Get next hardware dependent device number
 * \param ctl CTL handle
 * \param device current device on entry and next device on return
 * \return 0 on success otherwise a negative error code
 */
int snd_ctl_hwdep_next_device(snd_ctl_t *ctl, int *device)
{
	assert(ctl && device);
	return ctl->ops->hwdep_next_device(ctl, device);
}

/**
 * \brief Get info about a hardware dependent device
 * \param ctl CTL handle
 * \param info Hardware dependent device id/info pointer
 * \return 0 on success otherwise a negative error code
 */
int snd_ctl_hwdep_info(snd_ctl_t *ctl, snd_hwdep_info_t * info)
{
	assert(ctl && info);
	return ctl->ops->hwdep_info(ctl, info);
}

/**
 * \brief Get next PCM device number
 * \param ctl CTL handle
 * \param device current device on entry and next device on return
 * \return 0 on success otherwise a negative error code
 */
int snd_ctl_pcm_next_device(snd_ctl_t *ctl, int * device)
{
	assert(ctl && device);
	return ctl->ops->pcm_next_device(ctl, device);
}

/**
 * \brief Get next PCM surround device number
 * \param ctl CTL handle
 * \param type surround type
 * \param device current device on entry and next device on return
 * \return 0 on success otherwise a negative error code
 */
int snd_ctl_pcm_surround_next_device(snd_ctl_t *ctl, snd_pcm_surround_type_t type, int * device)
{
	assert(ctl && device);
	return ctl->ops->pcm_surround_next_device(ctl, type, device);
}

/**
 * \brief Get info about a PCM device
 * \param ctl CTL handle
 * \param info PCM device id/info pointer
 * \return 0 on success otherwise a negative error code
 */
int snd_ctl_pcm_info(snd_ctl_t *ctl, snd_pcm_info_t * info)
{
	assert(ctl && info);
	return ctl->ops->pcm_info(ctl, info);
}

/**
 * \brief Set preferred PCM subdevice number of successive PCM open
 * \param ctl CTL handle
 * \param subdev Preferred PCM subdevice number
 * \return 0 on success otherwise a negative error code
 */
int snd_ctl_pcm_prefer_subdevice(snd_ctl_t *ctl, int subdev)
{
	assert(ctl);
	return ctl->ops->pcm_prefer_subdevice(ctl, subdev);
}

/**
 * \brief Get next RawMidi device number
 * \param ctl CTL handle
 * \param device current device on entry and next device on return
 * \return 0 on success otherwise a negative error code
 */
int snd_ctl_rawmidi_next_device(snd_ctl_t *ctl, int * device)
{
	assert(ctl && device);
	return ctl->ops->rawmidi_next_device(ctl, device);
}

/**
 * \brief Get info about a RawMidi device
 * \param ctl CTL handle
 * \param info RawMidi device id/info pointer
 * \return 0 on success otherwise a negative error code
 */
int snd_ctl_rawmidi_info(snd_ctl_t *ctl, snd_rawmidi_info_t * info)
{
	assert(ctl && info);
	return ctl->ops->rawmidi_info(ctl, info);
}

/**
 * \brief Set preferred RawMidi subdevice number of successive RawMidi open
 * \param ctl CTL handle
 * \param subdev Preferred RawMidi subdevice number
 * \return 0 on success otherwise a negative error code
 */
int snd_ctl_rawmidi_prefer_subdevice(snd_ctl_t *ctl, int subdev)
{
	assert(ctl);
	return ctl->ops->rawmidi_prefer_subdevice(ctl, subdev);
}


/**
 * \brief Read an event
 * \param ctl CTL handle
 * \param event Event pointer
 * \return number of events read otherwise a negative error code on failure
 */
int snd_ctl_read(snd_ctl_t *ctl, snd_ctl_event_t *event)
{
	assert(ctl && event);
	return ctl->ops->read(ctl, event);
}

/**
 * \brief Wait for a CTL to become ready (i.e. at least one event pending)
 * \param ctl CTL handle
 * \param timeout maximum time in milliseconds to wait
 * \return 0 otherwise a negative error code on failure
 */
int snd_ctl_wait(snd_ctl_t *ctl, int timeout)
{
	struct pollfd pfd;
	int err;
	err = snd_ctl_poll_descriptors(ctl, &pfd, 1);
	assert(err == 1);
	err = poll(&pfd, 1, timeout);
	if (err < 0)
		return -errno;
	return 0;
}

/**
 * \brief Opens a CTL
 * \param ctlp Returned CTL handle
 * \param name ASCII identifier of the CTL handle
 * \param mode Open mode (see #SND_CTL_NONBLOCK, #SND_CTL_ASYNC)
 * \return 0 on success otherwise a negative error code
 */
int snd_ctl_open(snd_ctl_t **ctlp, const char *name, int mode)
{
	const char *str;
	char buf[256];
	int err;
	snd_config_t *ctl_conf, *conf, *type_conf;
	snd_config_iterator_t i, next;
	const char *lib = NULL, *open_name = NULL;
	int (*open_func)(snd_ctl_t **, const char *, snd_config_t *, int);
	void *h;
	const char *name1;
	assert(ctlp && name);
	err = snd_config_update();
	if (err < 0)
		return err;

	err = snd_config_search_alias(snd_config, "ctl", name, &ctl_conf);
	name1 = name;
	if (err < 0 || snd_config_get_string(ctl_conf, &name1) >= 0) {
		int card;
		char socket[256], sname[256];
		err = sscanf(name1, "hw:%d", &card);
		if (err == 1)
			return snd_ctl_hw_open(ctlp, name, card, mode);
		err = sscanf(name1, "shm:%256[^,],%256[^,]", socket, sname);
		if (err == 2)
			return snd_ctl_shm_open(ctlp, name, socket, sname, mode);
		SNDERR("Unknown ctl %s", name1);
		return -ENOENT;
	}
	if (snd_config_get_type(ctl_conf) != SND_CONFIG_TYPE_COMPOUND) {
		SNDERR("Invalid type for %s", snd_config_get_id(ctl_conf));
		return -EINVAL;
	}
	err = snd_config_search(ctl_conf, "type", &conf);
	if (err < 0)
		return err;
	err = snd_config_get_string(conf, &str);
	if (err < 0)
		return err;
	err = snd_config_search_alias(snd_config, "ctl_type", str, &type_conf);
	if (err >= 0) {
		if (snd_config_get_type(type_conf) != SND_CONFIG_TYPE_COMPOUND) {
			SNDERR("Invalid type for ctl type %s definition", str);
			return -EINVAL;
		}
		snd_config_for_each(i, next, type_conf) {
			snd_config_t *n = snd_config_iterator_entry(i);
			const char *id = snd_config_get_id(n);
			if (strcmp(id, "comment") == 0)
				continue;
			if (strcmp(id, "lib") == 0) {
				err = snd_config_get_string(n, &lib);
				if (err < 0)
					return -EINVAL;
				continue;
			}
			if (strcmp(id, "open") == 0) {
				err = snd_config_get_string(n, &open_name);
				if (err < 0)
					return -EINVAL;
				continue;
			}
			SNDERR("Unknown field %s", id);
			return -EINVAL;
		}
	}
	if (!open_name) {
		open_name = buf;
		snprintf(buf, sizeof(buf), "_snd_ctl_%s_open", str);
	}
	if (!lib)
		lib = "libasound.so";
	h = dlopen(lib, RTLD_NOW);
	if (!h) {
		SNDERR("Cannot open shared library %s", lib);
		return -ENOENT;
	}
	open_func = dlsym(h, open_name);
	if (!open_func) {
		SNDERR("symbol %s is not defined inside %s", open_name, lib);
		dlclose(h);
		return -ENXIO;
	}
	return open_func(ctlp, name, ctl_conf, mode);
}

/**
 * \brief Set CTL element #SND_CTL_ELEM_TYPE_BYTES value
 * \param ctl CTL handle
 * \param data Bytes value
 * \param size Size in bytes
 */
void snd_ctl_elem_set_bytes(snd_ctl_elem_value_t *obj, void *data, size_t size)
{
	assert(obj);
	if (size >= sizeof(obj->value.bytes.data)) {
		assert(0);
		return;
	}
	memcpy(obj->value.bytes.data, data, size);
}

#ifndef DOC_HIDDEN
#define TYPE(v) [SND_CTL_ELEM_TYPE_##v] = #v
#define IFACE(v) [SND_CTL_ELEM_IFACE_##v] = #v
#define EVENT(v) [SND_CTL_EVENT_##v] = #v

static const char *snd_ctl_elem_type_names[] = {
	TYPE(NONE),
	TYPE(BOOLEAN),
	TYPE(INTEGER),
	TYPE(ENUMERATED),
	TYPE(BYTES),
	TYPE(IEC958),
};

static const char *snd_ctl_elem_iface_names[] = {
	IFACE(CARD),
	IFACE(HWDEP),
	IFACE(MIXER),
	IFACE(PCM),
	IFACE(RAWMIDI),
	IFACE(TIMER),
	IFACE(SEQUENCER),
};

static const char *snd_ctl_event_type_names[] = {
	EVENT(ELEM),
};
#endif

/**
 * \brief get name of a CTL element type
 * \param type CTL element type
 * \return ascii name of CTL element type
 */
const char *snd_ctl_elem_type_name(snd_ctl_elem_type_t type)
{
	assert(type <= SND_CTL_ELEM_TYPE_LAST);
	return snd_ctl_elem_type_names[snd_enum_to_int(type)];
}

/**
 * \brief get name of a CTL element related interface
 * \param iface CTL element related interface
 * \return ascii name of CTL element related interface
 */
const char *snd_ctl_elem_iface_name(snd_ctl_elem_iface_t iface)
{
	assert(iface <= SND_CTL_ELEM_IFACE_LAST);
	return snd_ctl_elem_iface_names[snd_enum_to_int(iface)];
}

/**
 * \brief get name of a CTL event type
 * \param type CTL event type
 * \return ascii name of CTL event type
 */
const char *snd_ctl_event_type_name(snd_ctl_event_type_t type)
{
	assert(type <= SND_CTL_EVENT_LAST);
	return snd_ctl_event_type_names[snd_enum_to_int(type)];
}

/**
 * \brief allocate space for CTL element identificators list
 * \param obj CTL element identificators list
 * \param entries Entries to allocate
 * \return 0 on success otherwise a negative error code
 */
int snd_ctl_elem_list_alloc_space(snd_ctl_elem_list_t *obj, unsigned int entries)
{
	if (obj->pids)
		free(obj->pids);
	obj->pids = calloc(entries, sizeof(*obj->pids));
	if (!obj->pids) {
		obj->space = 0;
		return -ENOMEM;
	}
	obj->space = entries;
	return 0;
}  

/**
 * \brief free previously allocated space for CTL element identificators list
 * \param obj CTL element identificators list
 */
void snd_ctl_elem_list_free_space(snd_ctl_elem_list_t *obj)
{
	free(obj->pids);
	obj->pids = NULL;
}

/**
 * \brief Get event mask for an element related event
 * \param obj CTL event
 * \return event mask for element related event
 */
unsigned int snd_ctl_event_elem_get_mask(const snd_ctl_event_t *obj)
{
	assert(obj);
	assert(obj->type == SND_CTL_EVENT_ELEM);
	return obj->data.elem.mask;
}

/**
 * \brief Get CTL element identificator for an element related event
 * \param obj CTL event
 * \param ptr Pointer to returned CTL element identificator
 */
void snd_ctl_event_elem_get_id(const snd_ctl_event_t *obj, snd_ctl_elem_id_t *ptr)
{
	assert(obj && ptr);
	assert(obj->type == SND_CTL_EVENT_ELEM);
	*ptr = obj->data.elem.id;
}

/**
 * \brief Get element numeric identificator for an element related event
 * \param obj CTL event
 * \return element numeric identificator
 */
unsigned int snd_ctl_event_elem_get_numid(const snd_ctl_event_t *obj)
{
	assert(obj);
	assert(obj->type == SND_CTL_EVENT_ELEM);
	return obj->data.elem.id.numid;
}

/**
 * \brief Get interface part of CTL element identificator for an element related event
 * \param obj CTL event
 * \return interface part of element identificator
 */
snd_ctl_elem_iface_t snd_ctl_event_elem_get_interface(const snd_ctl_event_t *obj)
{
	assert(obj);
	assert(obj->type == SND_CTL_EVENT_ELEM);
	return snd_int_to_enum(obj->data.elem.id.iface);
}

/**
 * \brief Get device part of CTL element identificator for an element related event
 * \param obj CTL event
 * \return device part of element identificator
 */
unsigned int snd_ctl_event_elem_get_device(const snd_ctl_event_t *obj)
{
	assert(obj);
	assert(obj->type == SND_CTL_EVENT_ELEM);
	return obj->data.elem.id.device;
}

/**
 * \brief Get subdevice part of CTL element identificator for an element related event
 * \param obj CTL event
 * \return subdevice part of element identificator
 */
unsigned int snd_ctl_event_elem_get_subdevice(const snd_ctl_event_t *obj)
{
	assert(obj);
	assert(obj->type == SND_CTL_EVENT_ELEM);
	return obj->data.elem.id.subdevice;
}

/**
 * \brief Get name part of CTL element identificator for an element related event
 * \param obj CTL event
 * \return name part of element identificator
 */
const char *snd_ctl_event_elem_get_name(const snd_ctl_event_t *obj)
{
	assert(obj);
	assert(obj->type == SND_CTL_EVENT_ELEM);
	return obj->data.elem.id.name;
}

/**
 * \brief Get index part of CTL element identificator for an element related event
 * \param obj CTL event
 * \return index part of element identificator
 */
unsigned int snd_ctl_event_elem_get_index(const snd_ctl_event_t *obj)
{
	assert(obj);
	assert(obj->type == SND_CTL_EVENT_ELEM);
	return obj->data.elem.id.index;
}

#ifndef DOC_HIDDEN
int _snd_ctl_poll_descriptor(snd_ctl_t *ctl)
{
	assert(ctl);
	return ctl->ops->poll_descriptor(ctl);
}
#endif

/**
 * \brief get size of #snd_ctl_elem_id_t
 * \return size in bytes
 */
size_t snd_ctl_elem_id_sizeof()
{
	return sizeof(snd_ctl_elem_id_t);
}

/**
 * \brief allocate an invalid #snd_ctl_elem_id_t using standard malloc
 * \param ptr returned pointer
 * \return 0 on success otherwise negative error code
 */
int snd_ctl_elem_id_malloc(snd_ctl_elem_id_t **ptr)
{
	assert(ptr);
	*ptr = calloc(1, sizeof(snd_ctl_elem_id_t));
	if (!*ptr)
		return -ENOMEM;
	return 0;
}

/**
 * \brief frees a previously allocated #snd_ctl_elem_id_t
 * \param pointer to object to free
 */
void snd_ctl_elem_id_free(snd_ctl_elem_id_t *obj)
{
	free(obj);
}

/**
 * \brief copy one #snd_ctl_elem_id_t to another
 * \param dst pointer to destination
 * \param src pointer to source
 */
void snd_ctl_elem_id_copy(snd_ctl_elem_id_t *dst, const snd_ctl_elem_id_t *src)
{
	assert(dst && src);
	*dst = *src;
}

/**
 * \brief Get numeric identificator from a CTL element identificator
 * \param obj CTL element identificator
 * \return CTL element numeric identificator
 */
unsigned int snd_ctl_elem_id_get_numid(const snd_ctl_elem_id_t *obj)
{
	assert(obj);
	return obj->numid;
}

/**
 * \brief Get interface part of a CTL element identificator
 * \param obj CTL element identificator
 * \return CTL element related interface
 */
snd_ctl_elem_iface_t snd_ctl_elem_id_get_interface(const snd_ctl_elem_id_t *obj)
{
	assert(obj);
	return snd_int_to_enum(obj->iface);
}

/**
 * \brief Get device part of a CTL element identificator
 * \param obj CTL element identificator
 * \return CTL element related device
 */
unsigned int snd_ctl_elem_id_get_device(const snd_ctl_elem_id_t *obj)
{
	assert(obj);
	return obj->device;
}

/**
 * \brief Get subdevice part of a CTL element identificator
 * \param obj CTL element identificator
 * \return CTL element related subdevice
 */
unsigned int snd_ctl_elem_id_get_subdevice(const snd_ctl_elem_id_t *obj)
{
	assert(obj);
	return obj->subdevice;
}

/**
 * \brief Get name part of a CTL element identificator
 * \param obj CTL element identificator
 * \return CTL element name
 */
const char *snd_ctl_elem_id_get_name(const snd_ctl_elem_id_t *obj)
{
	assert(obj);
	return obj->name;
}

/**
 * \brief Get index part of a CTL element identificator
 * \param obj CTL element identificator
 * \return CTL element index
 */
unsigned int snd_ctl_elem_id_get_index(const snd_ctl_elem_id_t *obj)
{
	assert(obj);
	return obj->index;
}

/**
 * \brief Set numeric identificator for a CTL element identificator
 * \param obj CTL element identificator
 * \param val CTL element numeric identificator
 */
void snd_ctl_elem_id_set_numid(snd_ctl_elem_id_t *obj, unsigned int val)
{
	assert(obj);
	obj->numid = val;
}

/**
 * \brief Set interface part for a CTL element identificator
 * \param obj CTL element identificator
 * \param val CTL element related interface
 */
void snd_ctl_elem_id_set_interface(snd_ctl_elem_id_t *obj, snd_ctl_elem_iface_t val)
{
	assert(obj);
	obj->iface = snd_enum_to_int(val);
}

/**
 * \brief Set device part for a CTL element identificator
 * \param obj CTL element identificator
 * \param val CTL element related device
 */
void snd_ctl_elem_id_set_device(snd_ctl_elem_id_t *obj, unsigned int val)
{
	assert(obj);
	obj->device = val;
}

/**
 * \brief Set subdevice part for a CTL element identificator
 * \param obj CTL element identificator
 * \param val CTL element related subdevice
 */
void snd_ctl_elem_id_set_subdevice(snd_ctl_elem_id_t *obj, unsigned int val)
{
	assert(obj);
	obj->subdevice = val;
}

/**
 * \brief Set name part for a CTL element identificator
 * \param obj CTL element identificator
 * \param val CTL element name
 */
void snd_ctl_elem_id_set_name(snd_ctl_elem_id_t *obj, const char *val)
{
	assert(obj);
	strncpy(obj->name, val, sizeof(obj->name));
}

/**
 * \brief Set index part for a CTL element identificator
 * \param obj CTL element identificator
 * \param val CTL element index
 */
void snd_ctl_elem_id_set_index(snd_ctl_elem_id_t *obj, unsigned int val)
{
	assert(obj);
	obj->index = val;
}

/**
 * \brief get size of #snd_ctl_card_info_t
 * \return size in bytes
 */
size_t snd_ctl_card_info_sizeof()
{
	return sizeof(snd_ctl_card_info_t);
}

/**
 * \brief allocate an invalid #snd_ctl_card_info_t using standard malloc
 * \param ptr returned pointer
 * \return 0 on success otherwise negative error code
 */
int snd_ctl_card_info_malloc(snd_ctl_card_info_t **ptr)
{
	assert(ptr);
	*ptr = calloc(1, sizeof(snd_ctl_card_info_t));
	if (!*ptr)
		return -ENOMEM;
	return 0;
}

/**
 * \brief frees a previously allocated #snd_ctl_card_info_t
 * \param pointer to object to free
 */
void snd_ctl_card_info_free(snd_ctl_card_info_t *obj)
{
	free(obj);
}

/**
 * \brief copy one #snd_ctl_card_info_t to another
 * \param dst pointer to destination
 * \param src pointer to source
 */
void snd_ctl_card_info_copy(snd_ctl_card_info_t *dst, const snd_ctl_card_info_t *src)
{
	assert(dst && src);
	*dst = *src;
}

/**
 * \brief Get card number from a CTL card info
 * \param obj CTL card info
 * \return card number
 */
int snd_ctl_card_info_get_card(const snd_ctl_card_info_t *obj)
{
	assert(obj);
	return obj->card;
}

/**
 * \brief Get card type from a CTL card info
 * \param obj CTL card info
 * \return card type
 */
snd_card_type_t snd_ctl_card_info_get_type(const snd_ctl_card_info_t *obj)
{
	assert(obj);
	return snd_int_to_enum(obj->type);
}

/**
 * \brief Get card identificator from a CTL card info
 * \param obj CTL card info
 * \return card identificator
 */
const char *snd_ctl_card_info_get_id(const snd_ctl_card_info_t *obj)
{
	assert(obj);
	return obj->id;
}

/**
 * \brief Get card abbreviation from a CTL card info
 * \param obj CTL card info
 * \return card abbreviation
 */
const char *snd_ctl_card_info_get_abbreviation(const snd_ctl_card_info_t *obj)
{
	assert(obj);
	return obj->abbreviation;
}

/**
 * \brief Get card name from a CTL card info
 * \param obj CTL card info
 * \return card name
 */
const char *snd_ctl_card_info_get_name(const snd_ctl_card_info_t *obj)
{
	assert(obj);
	return obj->name;
}

/**
 * \brief Get card long name from a CTL card info
 * \param obj CTL card info
 * \return card long name
 */
const char *snd_ctl_card_info_get_longname(const snd_ctl_card_info_t *obj)
{
	assert(obj);
	return obj->longname;
}

/**
 * \brief Get card mixer identificator from a CTL card info
 * \param obj CTL card info
 * \return card mixer identificator
 */
const char *snd_ctl_card_info_get_mixerid(const snd_ctl_card_info_t *obj)
{
	assert(obj);
	return obj->mixerid;
}

/**
 * \brief Get card mixer name from a CTL card info
 * \param obj CTL card info
 * \return card mixer name
 */
const char *snd_ctl_card_info_get_mixername(const snd_ctl_card_info_t *obj)
{
	assert(obj);
	return obj->mixername;
}

/**
 * \brief get size of #snd_ctl_event_t
 * \return size in bytes
 */
size_t snd_ctl_event_sizeof()
{
	return sizeof(snd_ctl_event_t);
}

/**
 * \brief allocate an invalid #snd_ctl_event_t using standard malloc
 * \param ptr returned pointer
 * \return 0 on success otherwise negative error code
 */
int snd_ctl_event_malloc(snd_ctl_event_t **ptr)
{
	assert(ptr);
	*ptr = calloc(1, sizeof(snd_ctl_event_t));
	if (!*ptr)
		return -ENOMEM;
	return 0;
}

/**
 * \brief frees a previously allocated #snd_ctl_event_t
 * \param pointer to object to free
 */
void snd_ctl_event_free(snd_ctl_event_t *obj)
{
	free(obj);
}

/**
 * \brief copy one #snd_ctl_event_t to another
 * \param dst pointer to destination
 * \param src pointer to source
 */
void snd_ctl_event_copy(snd_ctl_event_t *dst, const snd_ctl_event_t *src)
{
	assert(dst && src);
	*dst = *src;
}

/**
 * \brief Get type of a CTL event
 * \param obj CTL event
 * \return CTL event type
 */
snd_ctl_event_type_t snd_ctl_event_get_type(const snd_ctl_event_t *obj)
{
	assert(obj);
	return snd_int_to_enum(obj->type);
}

/**
 * \brief get size of #snd_ctl_elem_list_t
 * \return size in bytes
 */
size_t snd_ctl_elem_list_sizeof()
{
	return sizeof(snd_ctl_elem_list_t);
}

/**
 * \brief allocate an invalid #snd_ctl_elem_list_t using standard malloc
 * \param ptr returned pointer
 * \return 0 on success otherwise negative error code
 */
int snd_ctl_elem_list_malloc(snd_ctl_elem_list_t **ptr)
{
	assert(ptr);
	*ptr = calloc(1, sizeof(snd_ctl_elem_list_t));
	if (!*ptr)
		return -ENOMEM;
	return 0;
}

/**
 * \brief frees a previously allocated #snd_ctl_elem_list_t
 * \param pointer to object to free
 */
void snd_ctl_elem_list_free(snd_ctl_elem_list_t *obj)
{
	free(obj);
}

/**
 * \brief copy one #snd_ctl_elem_list_t to another
 * \param dst pointer to destination
 * \param src pointer to source
 */
void snd_ctl_elem_list_copy(snd_ctl_elem_list_t *dst, const snd_ctl_elem_list_t *src)
{
	assert(dst && src);
	*dst = *src;
}

/**
 * \brief Set index of first wanted CTL element identificator in a CTL element identificators list
 * \param obj CTL element identificators list
 * \param val index of CTL element to put at position 0 of list
 */
void snd_ctl_elem_list_set_offset(snd_ctl_elem_list_t *obj, unsigned int val)
{
	assert(obj);
	obj->offset = val;
}

/**
 * \brief Get number of used entries in CTL element identificators list
 * \param obj CTL element identificator list
 * \return number of used entries
 */
unsigned int snd_ctl_elem_list_get_used(const snd_ctl_elem_list_t *obj)
{
	assert(obj);
	return obj->used;
}

/**
 * \brief Get total count of elements present in CTL device (information present in every filled CTL element identificators list)
 * \param obj CTL element identificator list
 * \return total number of elements
 */
unsigned int snd_ctl_elem_list_get_count(const snd_ctl_elem_list_t *obj)
{
	assert(obj);
	return obj->count;
}

/**
 * \brief Get CTL element identificator for an entry of a CTL element identificators list
 * \param obj CTL element identificator list
 * \param idx Index of entry
 * \param ptr Pointer to returned CTL element identificator
 */
void snd_ctl_elem_list_get_id(const snd_ctl_elem_list_t *obj, unsigned int idx, snd_ctl_elem_id_t *ptr)
{
	assert(obj && ptr);
	assert(idx < obj->used);
	*ptr = obj->pids[idx];
}

/**
 * \brief Get CTL element numeric identificator for an entry of a CTL element identificators list
 * \param obj CTL element identificator list
 * \param idx Index of entry
 * \return CTL element numeric identificator
 */
unsigned int snd_ctl_elem_list_get_numid(const snd_ctl_elem_list_t *obj, unsigned int idx)
{
	assert(obj);
	assert(idx < obj->used);
	return obj->pids[idx].numid;
}

/**
 * \brief Get interface part of CTL element identificator for an entry of a CTL element identificators list
 * \param obj CTL element identificator list
 * \param idx Index of entry
 * \return CTL element related interface
 */
snd_ctl_elem_iface_t snd_ctl_elem_list_get_interface(const snd_ctl_elem_list_t *obj, unsigned int idx)
{
	assert(obj);
	assert(idx < obj->used);
	return snd_int_to_enum(obj->pids[idx].iface);
}

/**
 * \brief Get device part of CTL element identificator for an entry of a CTL element identificators list
 * \param obj CTL element identificator list
 * \param idx Index of entry
 * \return CTL element related device
 */
unsigned int snd_ctl_elem_list_get_device(const snd_ctl_elem_list_t *obj, unsigned int idx)
{
	assert(obj);
	assert(idx < obj->used);
	return obj->pids[idx].device;
}

/**
 * \brief Get subdevice part of CTL element identificator for an entry of a CTL element identificators list
 * \param obj CTL element identificator list
 * \param idx Index of entry
 * \return CTL element related subdevice
 */
unsigned int snd_ctl_elem_list_get_subdevice(const snd_ctl_elem_list_t *obj, unsigned int idx)
{
	assert(obj);
	assert(idx < obj->used);
	return obj->pids[idx].subdevice;
}

/**
 * \brief Get name part of CTL element identificator for an entry of a CTL element identificators list
 * \param obj CTL element identificator list
 * \param idx Index of entry
 * \return CTL element name
 */
const char *snd_ctl_elem_list_get_name(const snd_ctl_elem_list_t *obj, unsigned int idx)
{
	assert(obj);
	assert(idx < obj->used);
	return obj->pids[idx].name;
}

/**
 * \brief Get index part of CTL element identificator for an entry of a CTL element identificators list
 * \param obj CTL element identificator list
 * \param idx Index of entry
 * \return CTL element index
 */
unsigned int snd_ctl_elem_list_get_index(const snd_ctl_elem_list_t *obj, unsigned int idx)
{
	assert(obj);
	assert(idx < obj->used);
	return obj->pids[idx].index;
}

/**
 * \brief get size of #snd_ctl_elem_info_t
 * \return size in bytes
 */
size_t snd_ctl_elem_info_sizeof()
{
	return sizeof(snd_ctl_elem_info_t);
}

/**
 * \brief allocate an invalid #snd_ctl_elem_info_t using standard malloc
 * \param ptr returned pointer
 * \return 0 on success otherwise negative error code
 */
int snd_ctl_elem_info_malloc(snd_ctl_elem_info_t **ptr)
{
	assert(ptr);
	*ptr = calloc(1, sizeof(snd_ctl_elem_info_t));
	if (!*ptr)
		return -ENOMEM;
	return 0;
}

/**
 * \brief frees a previously allocated #snd_ctl_elem_info_t
 * \param pointer to object to free
 */
void snd_ctl_elem_info_free(snd_ctl_elem_info_t *obj)
{
	free(obj);
}

/**
 * \brief copy one #snd_ctl_elem_info_t to another
 * \param dst pointer to destination
 * \param src pointer to source
 */
void snd_ctl_elem_info_copy(snd_ctl_elem_info_t *dst, const snd_ctl_elem_info_t *src)
{
	assert(dst && src);
	*dst = *src;
}

/**
 * \brief Get type from a CTL element id/info
 * \param obj CTL element id/info
 * \return CTL element content type
 */
snd_ctl_elem_type_t snd_ctl_elem_info_get_type(const snd_ctl_elem_info_t *obj)
{
	assert(obj);
	return snd_int_to_enum(obj->type);
}

/**
 * \brief Get info about readability from a CTL element id/info
 * \param obj CTL element id/info
 * \return 0 if element is not redable, 1 if element is readable
 */
int snd_ctl_elem_info_is_readable(const snd_ctl_elem_info_t *obj)
{
	assert(obj);
	return !!(obj->access & SNDRV_CTL_ELEM_ACCESS_READ);
}

/**
 * \brief Get info about writability from a CTL element id/info
 * \param obj CTL element id/info
 * \return 0 if element is not writable, 1 if element is not writable
 */
int snd_ctl_elem_info_is_writable(const snd_ctl_elem_info_t *obj)
{
	assert(obj);
	return !!(obj->access & SNDRV_CTL_ELEM_ACCESS_WRITE);
}

/**
 * \brief Get info about notification feasibility from a CTL element id/info
 * \param obj CTL element id/info
 * \return 0 if all element value changes are notified to subscribed applications, 1 otherwise
 */
int snd_ctl_elem_info_is_volatile(const snd_ctl_elem_info_t *obj)
{
	assert(obj);
	return !!(obj->access & SNDRV_CTL_ELEM_ACCESS_VOLATILE);
}

/**
 * \brief Get info about status from a CTL element id/info
 * \param obj CTL element id/info
 * \return 0 if element value is not active, 1 if is active
 */
int snd_ctl_elem_info_is_inactive(const snd_ctl_elem_info_t *obj)
{
	assert(obj);
	return !!(obj->access & SNDRV_CTL_ELEM_ACCESS_INACTIVE);
}

/**
 * \brief Get info whether an element is locked
 * \param obj CTL element id/info
 * \return 0 if element value is currently changeable, 1 if it's locked by another application
 */
int snd_ctl_elem_info_is_locked(const snd_ctl_elem_info_t *obj)
{
	assert(obj);
	return !!(obj->access & SNDRV_CTL_ELEM_ACCESS_LOCK);
}

/**
 * \brief Get info if I own an element
 * \param obj CTL element id/info
 * \return 0 if element value is currently changeable, 1 if it's locked by another application
 */
int snd_ctl_elem_info_is_owner(const snd_ctl_elem_info_t *obj)
{
	assert(obj);
	return !!(obj->access & SNDRV_CTL_ELEM_ACCESS_OWNER);
}

/**
 * \brief Get info about values passing policy from a CTL element id/info
 * \param obj CTL element id/info
 * \return 0 if element value need to be passed by contents, 1 if need to be passed with a pointer
 */
int snd_ctl_elem_info_is_indirect(const snd_ctl_elem_info_t *obj)
{
	assert(obj);
	return !!(obj->access & SNDRV_CTL_ELEM_ACCESS_INDIRECT);
}

/**
 * \brief Get owner of a locked element
 * \param obj CTL element id/info
 * \return value entries count
 */
pid_t snd_ctl_elem_info_get_owner(const snd_ctl_elem_info_t *obj)
{
	assert(obj);
	return obj->owner;
}

/**
 * \brief Get number of value entries from a CTL element id/info
 * \param obj CTL element id/info
 * \return value entries count
 */
unsigned int snd_ctl_elem_info_get_count(const snd_ctl_elem_info_t *obj)
{
	assert(obj);
	return obj->count;
}

/**
 * \brief Get minimum value from a #SND_CTL_ELEM_TYPE_INTEGER CTL element id/info
 * \param obj CTL element id/info
 * \return Minimum value
 */
long snd_ctl_elem_info_get_min(const snd_ctl_elem_info_t *obj)
{
	assert(obj);
	assert(obj->type == SND_CTL_ELEM_TYPE_INTEGER);
	return obj->value.integer.min;
}

/**
 * \brief Get maximum value from a #SND_CTL_ELEM_TYPE_INTEGER CTL element id/info
 * \param obj CTL element id/info
 * \return Maximum value
 */
long snd_ctl_elem_info_get_max(const snd_ctl_elem_info_t *obj)
{
	assert(obj);
	assert(obj->type == SND_CTL_ELEM_TYPE_INTEGER);
	return obj->value.integer.max;
}

/**
 * \brief Get value step from a #SND_CTL_ELEM_TYPE_INTEGER CTL element id/info
 * \param obj CTL element id/info
 * \return Step
 */
long snd_ctl_elem_info_get_step(const snd_ctl_elem_info_t *obj)
{
	assert(obj);
	assert(obj->type == SND_CTL_ELEM_TYPE_INTEGER);
	return obj->value.integer.step;
}

/**
 * \brief Get number of items available from a #SND_CTL_ELEM_TYPE_ENUMERATED CTL element id/info
 * \param obj CTL element id/info
 * \return items count
 */
unsigned int snd_ctl_elem_info_get_items(const snd_ctl_elem_info_t *obj)
{
	assert(obj);
	assert(obj->type == SND_CTL_ELEM_TYPE_ENUMERATED);
	return obj->value.enumerated.items;
}

/**
 * \brief Select item in a #SND_CTL_ELEM_TYPE_ENUMERATED CTL element id/info
 * \param obj CTL element id/info
 * \param val item number
 */
void snd_ctl_elem_info_set_item(snd_ctl_elem_info_t *obj, unsigned int val)
{
	assert(obj);
	obj->value.enumerated.item = val;
}

/**
 * \brief Get name for selected item in a #SND_CTL_ELEM_TYPE_ENUMERATED CTL element id/info
 * \param obj CTL element id/info
 * \return name of choosen item
 */
const char *snd_ctl_elem_info_get_item_name(const snd_ctl_elem_info_t *obj)
{
	assert(obj);
	assert(obj->type == SND_CTL_ELEM_TYPE_ENUMERATED);
	return obj->value.enumerated.name;
}

/**
 * \brief Get CTL element identificator of a CTL element id/info
 * \param obj CTL element id/info
 * \param ptr Pointer to returned CTL element identificator
 */
void snd_ctl_elem_info_get_id(const snd_ctl_elem_info_t *obj, snd_ctl_elem_id_t *ptr)
{
	assert(obj && ptr);
	*ptr = obj->id;
}

/**
 * \brief Get element numeric identificator of a CTL element id/info
 * \param obj CTL element id/info
 * \return element numeric identificator
 */
unsigned int snd_ctl_elem_info_get_numid(const snd_ctl_elem_info_t *obj)
{
	assert(obj);
	return obj->id.numid;
}

/**
 * \brief Get interface part of CTL element identificator of a CTL element id/info
 * \param obj CTL element id/info
 * \return interface part of element identificator
 */
snd_ctl_elem_iface_t snd_ctl_elem_info_get_interface(const snd_ctl_elem_info_t *obj)
{
	assert(obj);
	return snd_int_to_enum(obj->id.iface);
}

/**
 * \brief Get device part of CTL element identificator of a CTL element id/info
 * \param obj CTL element id/info
 * \return device part of element identificator
 */
unsigned int snd_ctl_elem_info_get_device(const snd_ctl_elem_info_t *obj)
{
	assert(obj);
	return obj->id.device;
}

/**
 * \brief Get subdevice part of CTL element identificator of a CTL element id/info
 * \param obj CTL element id/info
 * \return subdevice part of element identificator
 */
unsigned int snd_ctl_elem_info_get_subdevice(const snd_ctl_elem_info_t *obj)
{
	assert(obj);
	return obj->id.subdevice;
}

/**
 * \brief Get name part of CTL element identificator of a CTL element id/info
 * \param obj CTL element id/info
 * \return name part of element identificator
 */
const char *snd_ctl_elem_info_get_name(const snd_ctl_elem_info_t *obj)
{
	assert(obj);
	return obj->id.name;
}

/**
 * \brief Get index part of CTL element identificator of a CTL element id/info
 * \param obj CTL element id/info
 * \return index part of element identificator
 */
unsigned int snd_ctl_elem_info_get_index(const snd_ctl_elem_info_t *obj)
{
	assert(obj);
	return obj->id.index;
}

/**
 * \brief Set CTL element identificator of a CTL element id/info
 * \param obj CTL element id/info
 * \param ptr CTL element identificator
 */
void snd_ctl_elem_info_set_id(snd_ctl_elem_info_t *obj, const snd_ctl_elem_id_t *ptr)
{
	assert(obj && ptr);
	obj->id = *ptr;
}

/**
 * \brief Set element numeric identificator of a CTL element id/info
 * \param obj CTL element id/info
 * \param val element numeric identificator
 */
void snd_ctl_elem_info_set_numid(snd_ctl_elem_info_t *obj, unsigned int val)
{
	assert(obj);
	obj->id.numid = val;
}

/**
 * \brief Set interface part of CTL element identificator of a CTL element id/info
 * \param obj CTL element id/info
 * \param val interface part of element identificator
 */
void snd_ctl_elem_info_set_interface(snd_ctl_elem_info_t *obj, snd_ctl_elem_iface_t val)
{
	assert(obj);
	obj->id.iface = snd_enum_to_int(val);
}

/**
 * \brief Set device part of CTL element identificator of a CTL element id/info
 * \param obj CTL element id/info
 * \param val device part of element identificator
 */
void snd_ctl_elem_info_set_device(snd_ctl_elem_info_t *obj, unsigned int val)
{
	assert(obj);
	obj->id.device = val;
}

/**
 * \brief Set subdevice part of CTL element identificator of a CTL element id/info
 * \param obj CTL element id/info
 * \param val subdevice part of element identificator
 */
void snd_ctl_elem_info_set_subdevice(snd_ctl_elem_info_t *obj, unsigned int val)
{
	assert(obj);
	obj->id.subdevice = val;
}

/**
 * \brief Set name part of CTL element identificator of a CTL element id/info
 * \param obj CTL element id/info
 * \param val name part of element identificator
 */
void snd_ctl_elem_info_set_name(snd_ctl_elem_info_t *obj, const char *val)
{
	assert(obj);
	strncpy(obj->id.name, val, sizeof(obj->id.name));
}

/**
 * \brief Set index part of CTL element identificator of a CTL element id/info
 * \param obj CTL element id/info
 * \param val index part of element identificator
 */
void snd_ctl_elem_info_set_index(snd_ctl_elem_info_t *obj, unsigned int val)
{
	assert(obj);
	obj->id.index = val;
}

/**
 * \brief get size of #snd_ctl_elem_value_t
 * \return size in bytes
 */
size_t snd_ctl_elem_value_sizeof()
{
	return sizeof(snd_ctl_elem_value_t);
}

/**
 * \brief allocate an invalid #snd_ctl_elem_value_t using standard malloc
 * \param ptr returned pointer
 * \return 0 on success otherwise negative error code
 */
int snd_ctl_elem_value_malloc(snd_ctl_elem_value_t **ptr)
{
	assert(ptr);
	*ptr = calloc(1, sizeof(snd_ctl_elem_value_t));
	if (!*ptr)
		return -ENOMEM;
	return 0;
}

/**
 * \brief frees a previously allocated #snd_ctl_elem_value_t
 * \param pointer to object to free
 */
void snd_ctl_elem_value_free(snd_ctl_elem_value_t *obj)
{
	free(obj);
}

/**
 * \brief copy one #snd_ctl_elem_value_t to another
 * \param dst pointer to destination
 * \param src pointer to source
 */
void snd_ctl_elem_value_copy(snd_ctl_elem_value_t *dst, const snd_ctl_elem_value_t *src)
{
	assert(dst && src);
	*dst = *src;
}

/**
 * \brief Get CTL element identificator of a CTL element id/value
 * \param obj CTL element id/value
 * \param ptr Pointer to returned CTL element identificator
 */
void snd_ctl_elem_value_get_id(const snd_ctl_elem_value_t *obj, snd_ctl_elem_id_t *ptr)
{
	assert(obj && ptr);
	*ptr = obj->id;
}

/**
 * \brief Get element numeric identificator of a CTL element id/value
 * \param obj CTL element id/value
 * \return element numeric identificator
 */
unsigned int snd_ctl_elem_value_get_numid(const snd_ctl_elem_value_t *obj)
{
	assert(obj);
	return obj->id.numid;
}

/**
 * \brief Get interface part of CTL element identificator of a CTL element id/value
 * \param obj CTL element id/value
 * \return interface part of element identificator
 */
snd_ctl_elem_iface_t snd_ctl_elem_value_get_interface(const snd_ctl_elem_value_t *obj)
{
	assert(obj);
	return snd_int_to_enum(obj->id.iface);
}

/**
 * \brief Get device part of CTL element identificator of a CTL element id/value
 * \param obj CTL element id/value
 * \return device part of element identificator
 */
unsigned int snd_ctl_elem_value_get_device(const snd_ctl_elem_value_t *obj)
{
	assert(obj);
	return obj->id.device;
}

/**
 * \brief Get subdevice part of CTL element identificator of a CTL element id/value
 * \param obj CTL element id/value
 * \return subdevice part of element identificator
 */
unsigned int snd_ctl_elem_value_get_subdevice(const snd_ctl_elem_value_t *obj)
{
	assert(obj);
	return obj->id.subdevice;
}

/**
 * \brief Get name part of CTL element identificator of a CTL element id/value
 * \param obj CTL element id/value
 * \return name part of element identificator
 */
const char *snd_ctl_elem_value_get_name(const snd_ctl_elem_value_t *obj)
{
	assert(obj);
	return obj->id.name;
}

/**
 * \brief Get index part of CTL element identificator of a CTL element id/value
 * \param obj CTL element id/value
 * \return index part of element identificator
 */
unsigned int snd_ctl_elem_value_get_index(const snd_ctl_elem_value_t *obj)
{
	assert(obj);
	return obj->id.index;
}

/**
 * \brief Set CTL element identificator of a CTL element id/value
 * \param obj CTL element id/value
 * \param ptr CTL element identificator
 */
void snd_ctl_elem_value_set_id(snd_ctl_elem_value_t *obj, const snd_ctl_elem_id_t *ptr)
{
	assert(obj && ptr);
	obj->id = *ptr;
}

/**
 * \brief Set element numeric identificator of a CTL element id/value
 * \param obj CTL element id/value
 * \param val element numeric identificator
 */
void snd_ctl_elem_value_set_numid(snd_ctl_elem_value_t *obj, unsigned int val)
{
	assert(obj);
	obj->id.numid = val;
}

/**
 * \brief Set interface part of CTL element identificator of a CTL element id/value
 * \param obj CTL element id/value
 * \param val interface part of element identificator
 */
void snd_ctl_elem_value_set_interface(snd_ctl_elem_value_t *obj, snd_ctl_elem_iface_t val)
{
	assert(obj);
	obj->id.iface = snd_enum_to_int(val);
}

/**
 * \brief Set device part of CTL element identificator of a CTL element id/value
 * \param obj CTL element id/value
 * \param val device part of element identificator
 */
void snd_ctl_elem_value_set_device(snd_ctl_elem_value_t *obj, unsigned int val)
{
	assert(obj);
	obj->id.device = val;
}

/**
 * \brief Set subdevice part of CTL element identificator of a CTL element id/value
 * \param obj CTL element id/value
 * \param val subdevice part of element identificator
 */
void snd_ctl_elem_value_set_subdevice(snd_ctl_elem_value_t *obj, unsigned int val)
{
	assert(obj);
	obj->id.subdevice = val;
}

/**
 * \brief Set name part of CTL element identificator of a CTL element id/value
 * \param obj CTL element id/value
 * \param val name part of element identificator
 */
void snd_ctl_elem_value_set_name(snd_ctl_elem_value_t *obj, const char *val)
{
	assert(obj);
	strncpy(obj->id.name, val, sizeof(obj->id.name));
}

/**
 * \brief Set index part of CTL element identificator of a CTL element id/value
 * \param obj CTL element id/value
 * \param val index part of element identificator
 */
void snd_ctl_elem_value_set_index(snd_ctl_elem_value_t *obj, unsigned int val)
{
	assert(obj);
	obj->id.index = val;
}

/**
 * \brief Get value for an entry of a #SND_CTL_ELEM_TYPE_BOOLEAN CTL element id/value 
 * \param obj CTL element id/value
 * \param idx Entry index
 * \return value for the entry
 */ 
int snd_ctl_elem_value_get_boolean(const snd_ctl_elem_value_t *obj, unsigned int idx)
{
	assert(obj);
	assert(idx < sizeof(obj->value.integer.value) / sizeof(obj->value.integer.value[0]));
	return obj->value.integer.value[idx];
}

/**
 * \brief Get value for an entry of a #SND_CTL_ELEM_TYPE_INTEGER CTL element id/value 
 * \param obj CTL element id/value
 * \param idx Entry index
 * \return value for the entry
 */ 
long snd_ctl_elem_value_get_integer(const snd_ctl_elem_value_t *obj, unsigned int idx)
{
	assert(obj);
	assert(idx < sizeof(obj->value.integer.value) / sizeof(obj->value.integer.value[0]));
	return obj->value.integer.value[idx];
}

/**
 * \brief Get value for an entry of a #SND_CTL_ELEM_TYPE_ENUMERATED CTL element id/value 
 * \param obj CTL element id/value
 * \param idx Entry index
 * \return value for the entry
 */ 
unsigned int snd_ctl_elem_value_get_enumerated(const snd_ctl_elem_value_t *obj, unsigned int idx)
{
	assert(obj);
	assert(idx < sizeof(obj->value.enumerated.item) / sizeof(obj->value.enumerated.item[0]));
	return obj->value.enumerated.item[idx];
}

/**
 * \brief Get value for an entry of a #SND_CTL_ELEM_TYPE_BYTES CTL element id/value 
 * \param obj CTL element id/value
 * \param idx Entry index
 * \return value for the entry
 */ 
unsigned char snd_ctl_elem_value_get_byte(const snd_ctl_elem_value_t *obj, unsigned int idx)
{
	assert(obj);
	assert(idx < sizeof(obj->value.bytes.data));
	return obj->value.bytes.data[idx];
}

/**
 * \brief Set value for an entry of a #SND_CTL_ELEM_TYPE_BOOLEAN CTL element id/value 
 * \param obj CTL element id/value
 * \param idx Entry index
 * \param val value for the entry
 */ 
void snd_ctl_elem_value_set_boolean(snd_ctl_elem_value_t *obj, unsigned int idx, long val)
{
	assert(obj);
	obj->value.integer.value[idx] = val;
}

/**
 * \brief Set value for an entry of a #SND_CTL_ELEM_TYPE_INTEGER CTL element id/value 
 * \param obj CTL element id/value
 * \param idx Entry index
 * \param val value for the entry
 */ 
void snd_ctl_elem_value_set_integer(snd_ctl_elem_value_t *obj, unsigned int idx, long val)
{
	assert(obj);
	obj->value.integer.value[idx] = val;
}

/**
 * \brief Set value for an entry of a #SND_CTL_ELEM_TYPE_ENUMERATED CTL element id/value 
 * \param obj CTL element id/value
 * \param idx Entry index
 * \param val value for the entry
 */ 
void snd_ctl_elem_value_set_enumerated(snd_ctl_elem_value_t *obj, unsigned int idx, unsigned int val)
{
	assert(obj);
	obj->value.enumerated.item[idx] = val;
}

/**
 * \brief Set value for an entry of a #SND_CTL_ELEM_TYPE_BYTES CTL element id/value 
 * \param obj CTL element id/value
 * \param idx Entry index
 * \param val value for the entry
 */ 
void snd_ctl_elem_value_set_byte(snd_ctl_elem_value_t *obj, unsigned int idx, unsigned char val)
{
	assert(obj);
	obj->value.bytes.data[idx] = val;
}

/**
 * \brief Get value for a #SND_CTL_ELEM_TYPE_BYTES CTL element id/value 
 * \param obj CTL element id/value
 * \return Pointer to CTL element value
 */ 
const void * snd_ctl_elem_value_get_bytes(const snd_ctl_elem_value_t *obj)
{
	assert(obj);
	return obj->value.bytes.data;
}

/**
 * \brief Get value for a #SND_CTL_ELEM_TYPE_IEC958 CTL element id/value 
 * \param obj CTL element id/value
 * \param Pointer to returned CTL element value
 */ 
void snd_ctl_elem_value_get_iec958(const snd_ctl_elem_value_t *obj, snd_aes_iec958_t *ptr)
{
	assert(obj && ptr);
	*ptr = obj->value.iec958;
}

/**
 * \brief Set value for a #SND_CTL_ELEM_TYPE_IEC958 CTL element id/value 
 * \param obj CTL element id/value
 * \param Pointer to CTL element value
 */ 
void snd_ctl_elem_value_set_iec958(snd_ctl_elem_value_t *obj, const snd_aes_iec958_t *ptr)
{
	assert(obj && ptr);
	obj->value.iec958 = *ptr;
}

