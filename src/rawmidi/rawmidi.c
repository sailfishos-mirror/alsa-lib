/**
 * \file rawmidi/rawmidi.c
 * \brief RawMidi Interface
 * \author Jaroslav Kysela <perex@perex.cz>
 * \author Abramo Bagnara <abramo@alsa-project.org>
 * \date 2000-2001
 *
 * See the \ref rawmidi page for more details.
 */
/*
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

/*! \page rawmidi RawMidi interface

<P>RawMidi Interface is designed to write or read raw (unchanged) MIDI
data over the MIDI line without any timestamps defined in interface. MIDI
stands Musical Instrument Digital Interface and more information about
this standard can be found at http://www.midi.org.

\section rawmidi_general_overview General overview

The rawmidi implementation uses ring buffers to store outgoing and incoming
MIDI stream. The buffer size is tunable and drivers report underruns for incoming
stream as well.

\section rawmidi_open Open handling

RawMidi devices are opened exclusively for a selected direction.
While more than one process may not open a given MIDI device in the same
direction simultaneously, separate processes may open a single MIDI device
in different directions (i.e. process one opens a MIDI device in write
direction and process two opens the same device in read direction).

\subsection rawmidi_open_nonblock Nonblocking open (flag)

Using #SND_RAWMIDI_NONBLOCK flag for snd_rawmidi_open() or snd_rawmidi_open_lconf()
instruct device driver to return the -EBUSY error when device is already occupied
with another application. This flag also changes behaviour of snd_rawmidi_write()
and snd_rawmidi_read() returning -EAGAIN when no more bytes can be processed.

Note: In opposite (default) behaviour, application is blocked until device resources
are free.

\subsection rawmidi_open_append Append open (flag)

Using #SND_RAWMIDI_APPEND flag (output only) instruct device driver to append
contents of written buffer - passed by snd_rawmidi_write() - atomically
to output ring buffer in the kernel space. This flag also means that device
is not opened exclusively, so more applications can share given rawmidi device.
Note that applications must send the whole MIDI message including the running status,
because another writing application might break the MIDI message in the output
buffer.

\subsection rawmidi_open_sync Sync open (flag)

Using #SND_RAWMIDI_SYNC flag (output only) assures that the contents of output
buffer specified using snd_rawmidi_write() is always drained before the function
exits. This behaviour is same like 'snd_rawmidi_write() followed by
snd_rawmidi_drain() immediately'.

\subsection rawmidi_io I/O handling

There is only standard read/write access to device internal ring buffer. Use
snd_rawmidi_read() and snd_rawmidi_write() functions to obtain / write MIDI bytes.

\subsection rawmidi_dev_names RawMidi naming conventions

The ALSA library uses a generic string representation for names of devices.
The devices might be virtual, physical or a mix of both. The generic string
is passed to \link ::snd_rawmidi_open() \endlink or \link ::snd_rawmidi_open_lconf() \endlink.
It contains two parts: device name and arguments. Devices and arguments are described
in configuration files. The usual place for default definitions is at /usr/share/alsa/alsa.conf.

\subsection rawmidi_dev_names_default 

The default device is equal to hw device. The defaults are used:

defaults.rawmidi.card 0
defaults.rawmidi.device 0
defaults.rawmidi.subdevice -1

These defaults can be freely overwritten in local configuration files.

Example:

\code
default
\endcode

\subsection rawmidi_dev_names_hw HW device

The hw device description uses the hw plugin. The three arguments (in order: CARD,DEV,SUBDEV)
specify card number or identifier, device number and subdevice number (-1 means any).

Example:

\code
hw
hw:0
hw:0,0
hw:supersonic,1
hw:soundwave,1,2
hw:DEV=1,CARD=soundwave,SUBDEV=2
\endcode

\section read_mode Read mode

Optionally, incoming rawmidi bytes can be marked with timestamps. The library hides
the kernel implementation (linux kernel 5.14+) and exports
the \link ::snd_rawmidi_tread() \endlink  function which returns the
midi bytes marked with the identical timestamp in one iteration.

The timestamping is available only on input streams.

\section rawmidi_examples Examples

The full featured examples with cross-links:

\par Simple input/output test program
\link example_test_rawmidi example code \endlink
\par
This example shows open and read/write rawmidi operations.

*/

/**
 * \example ../test/rawmidi.c
 * \anchor example_test_rawmidi
 * Shows open and read/write rawmidi operations.
 */
 
#include "rawmidi_local.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>

/**
 * \brief setup the default parameters
 * \param rawmidi RawMidi handle
 * \param params pointer to a snd_rawmidi_params_t structure
 * \return 0 on success otherwise a negative error code
 */
static int snd_rawmidi_params_default(snd_rawmidi_t *rawmidi, snd_rawmidi_params_t *params)
{
	assert(rawmidi);
	assert(params);
	params->buffer_size = page_size();
	params->avail_min = 1;
	params->no_active_sensing = 1;
	params->mode = 0;
	memset(params->reserved, 0, sizeof(params->reserved));
	return 0;
}

static int snd_rawmidi_open_conf(snd_rawmidi_t **inputp, snd_rawmidi_t **outputp,
				 const char *name, snd_config_t *rawmidi_root,
				 snd_config_t *rawmidi_conf, int mode)
{
	const char *str;
	char buf[256];
	int err;
	snd_config_t *conf, *type_conf = NULL;
	snd_config_iterator_t i, next;
	snd_rawmidi_params_t params;
	const char *id;
	const char *lib = NULL, *open_name = NULL;
	int (*open_func)(snd_rawmidi_t **, snd_rawmidi_t **,
			 const char *, snd_config_t *, snd_config_t *, int) = NULL;
#ifndef PIC
	extern void *snd_rawmidi_open_symbols(void);
#endif
	if (snd_config_get_type(rawmidi_conf) != SND_CONFIG_TYPE_COMPOUND) {
		if (name)
			SNDERR("Invalid type for RAWMIDI %s definition", name);
		else
			SNDERR("Invalid type for RAWMIDI definition");
		return -EINVAL;
	}
	err = snd_config_search(rawmidi_conf, "type", &conf);
	if (err < 0) {
		SNDERR("type is not defined");
		return err;
	}
	err = snd_config_get_id(conf, &id);
	if (err < 0) {
		SNDERR("unable to get id");
		return err;
	}
	err = snd_config_get_string(conf, &str);
	if (err < 0) {
		SNDERR("Invalid type for %s", id);
		return err;
	}
	err = snd_config_search_definition(rawmidi_root, "rawmidi_type", str, &type_conf);
	if (err >= 0) {
		if (snd_config_get_type(type_conf) != SND_CONFIG_TYPE_COMPOUND) {
			SNDERR("Invalid type for RAWMIDI type %s definition", str);
			err = -EINVAL;
			goto _err;
		}
		snd_config_for_each(i, next, type_conf) {
			snd_config_t *n = snd_config_iterator_entry(i);
			const char *id;
			if (snd_config_get_id(n, &id) < 0)
				continue;
			if (strcmp(id, "comment") == 0)
				continue;
			if (strcmp(id, "lib") == 0) {
				err = snd_config_get_string(n, &lib);
				if (err < 0) {
					SNDERR("Invalid type for %s", id);
					goto _err;
				}
				continue;
			}
			if (strcmp(id, "open") == 0) {
				err = snd_config_get_string(n, &open_name);
				if (err < 0) {
					SNDERR("Invalid type for %s", id);
					goto _err;
				}
				continue;
			}
			SNDERR("Unknown field %s", id);
			err = -EINVAL;
			goto _err;
		}
	}
	if (!open_name) {
		open_name = buf;
		snprintf(buf, sizeof(buf), "_snd_rawmidi_%s_open", str);
	}
#ifndef PIC
	snd_rawmidi_open_symbols();
#endif
	open_func = snd_dlobj_cache_get2(lib, open_name,
			SND_DLSYM_VERSION(SND_RAWMIDI_DLSYM_VERSION), 1);
	if (!open_func) {
		err = -ENXIO;
		goto _err;
	}
	if (type_conf)
		snd_config_delete(type_conf);
	err = open_func(inputp, outputp, name, rawmidi_root, rawmidi_conf, mode);
	if (err < 0)
		goto _err;
	if (inputp) {
		(*inputp)->open_func = open_func;
		snd_rawmidi_params_default(*inputp, &params);
		err = snd_rawmidi_params(*inputp, &params);
		assert(err >= 0);
	}
	if (outputp) {
		(*outputp)->open_func = open_func;
		snd_rawmidi_params_default(*outputp, &params);
		err = snd_rawmidi_params(*outputp, &params);
		assert(err >= 0);
	}
	return 0;

       _err:
	if (open_func)
		snd_dlobj_cache_put(open_func);
	if (type_conf)
		snd_config_delete(type_conf);
	return err;
}

static int snd_rawmidi_open_noupdate(snd_rawmidi_t **inputp, snd_rawmidi_t **outputp,
				     snd_config_t *root, const char *name, int mode)
{
	int err;
	snd_config_t *rawmidi_conf;
	err = snd_config_search_definition(root, "rawmidi", name, &rawmidi_conf);
	if (err < 0) {
		SNDERR("Unknown RawMidi %s", name);
		return err;
	}
	err = snd_rawmidi_open_conf(inputp, outputp, name, root, rawmidi_conf, mode);
	snd_config_delete(rawmidi_conf);
	return err;
}

/**
 * \brief Opens a new connection to the RawMidi interface.
 * \param inputp Returned input handle (NULL if not wanted)
 * \param outputp Returned output handle (NULL if not wanted)
 * \param name ASCII identifier of the RawMidi handle
 * \param mode Open mode
 * \return 0 on success otherwise a negative error code
 *
 * Opens a new connection to the RawMidi interface specified with
 * an ASCII identifier and mode.
 */
int snd_rawmidi_open(snd_rawmidi_t **inputp, snd_rawmidi_t **outputp,
		     const char *name, int mode)
{
	snd_config_t *top;
	int err;

	assert((inputp || outputp) && name);
	if (_snd_is_ucm_device(name)) {
		name = uc_mgr_alibcfg_by_device(&top, name);
		if (name == NULL)
			return -ENODEV;
	} else {
		err = snd_config_update_ref(&top);
		if (err < 0)
			return err;
	}
	err = snd_rawmidi_open_noupdate(inputp, outputp, top, name, mode);
	snd_config_unref(top);
	return err;
}

/**
 * \brief Opens a new connection to the RawMidi interface using local configuration
 * \param inputp Returned input handle (NULL if not wanted)
 * \param outputp Returned output handle (NULL if not wanted)
 * \param name ASCII identifier of the RawMidi handle
 * \param mode Open mode
 * \param lconf Local configuration
 * \return 0 on success otherwise a negative error code
 *
 * Opens a new connection to the RawMidi interface specified with
 * an ASCII identifier and mode.
 */
int snd_rawmidi_open_lconf(snd_rawmidi_t **inputp, snd_rawmidi_t **outputp,
			   const char *name, int mode, snd_config_t *lconf)
{
	assert((inputp || outputp) && name && lconf);
	return snd_rawmidi_open_noupdate(inputp, outputp, lconf, name, mode);
}

/**
 * \brief close RawMidi handle
 * \param rawmidi RawMidi handle
 * \return 0 on success otherwise a negative error code
 *
 * Closes the specified RawMidi handle and frees all associated
 * resources.
 */
int snd_rawmidi_close(snd_rawmidi_t *rawmidi)
{
	int err;
  	assert(rawmidi);
	err = rawmidi->ops->close(rawmidi);
	free(rawmidi->name);
	if (rawmidi->open_func)
		snd_dlobj_cache_put(rawmidi->open_func);
	free(rawmidi);
	return err;
}

/**
 * \brief get identifier of RawMidi handle
 * \param rawmidi a RawMidi handle
 * \return ascii identifier of RawMidi handle
 *
 * Returns the ASCII identifier of given RawMidi handle. It's the same
 * identifier specified in snd_rawmidi_open().
 */
const char *snd_rawmidi_name(snd_rawmidi_t *rawmidi)
{
	assert(rawmidi);
	return rawmidi->name;
}

/**
 * \brief get type of RawMidi handle
 * \param rawmidi a RawMidi handle
 * \return type of RawMidi handle
 *
 * Returns the type #snd_rawmidi_type_t of given RawMidi handle.
 */
snd_rawmidi_type_t snd_rawmidi_type(snd_rawmidi_t *rawmidi)
{
	assert(rawmidi);
	return rawmidi->type;
}

/**
 * \brief get stream (direction) of RawMidi handle
 * \param rawmidi a RawMidi handle
 * \return stream of RawMidi handle
 *
 * Returns the stream #snd_rawmidi_stream_t of given RawMidi handle.
 */
snd_rawmidi_stream_t snd_rawmidi_stream(snd_rawmidi_t *rawmidi)
{
	assert(rawmidi);
	return rawmidi->stream;
}

/**
 * \brief get count of poll descriptors for RawMidi handle
 * \param rawmidi RawMidi handle
 * \return count of poll descriptors
 */
int snd_rawmidi_poll_descriptors_count(snd_rawmidi_t *rawmidi)
{
	assert(rawmidi);
	return 1;
}

/**
 * \brief get poll descriptors
 * \param rawmidi RawMidi handle
 * \param pfds array of poll descriptors
 * \param space space in the poll descriptor array
 * \return count of filled descriptors
 */
int snd_rawmidi_poll_descriptors(snd_rawmidi_t *rawmidi, struct pollfd *pfds, unsigned int space)
{
	assert(rawmidi);
	if (space >= 1) {
		pfds->fd = rawmidi->poll_fd;
		pfds->events = rawmidi->stream == SND_RAWMIDI_STREAM_OUTPUT ? (POLLOUT|POLLERR|POLLNVAL) : (POLLIN|POLLERR|POLLNVAL);
		return 1;
	}
	return 0;
}

/**
 * \brief get returned events from poll descriptors
 * \param rawmidi rawmidi RawMidi handle
 * \param pfds array of poll descriptors
 * \param nfds count of poll descriptors
 * \param revents returned events
 * \return zero if success, otherwise a negative error code
 */
int snd_rawmidi_poll_descriptors_revents(snd_rawmidi_t *rawmidi, struct pollfd *pfds, unsigned int nfds, unsigned short *revents)
{
        assert(rawmidi && pfds && revents);
        if (nfds == 1) {
                *revents = pfds->revents;
                return 0;
        }
        return -EINVAL;
}

/**
 * \brief set nonblock mode
 * \param rawmidi RawMidi handle
 * \param nonblock 0 = block, 1 = nonblock mode
 * \return 0 on success otherwise a negative error code
 *
 * The nonblock mode cannot be used when the stream is in
 * #SND_RAWMIDI_APPEND state.
 */
int snd_rawmidi_nonblock(snd_rawmidi_t *rawmidi, int nonblock)
{
	int err;
	assert(rawmidi);
	assert(!(rawmidi->mode & SND_RAWMIDI_APPEND));
	if ((err = rawmidi->ops->nonblock(rawmidi, nonblock)) < 0)
		return err;
	if (nonblock)
		rawmidi->mode |= SND_RAWMIDI_NONBLOCK;
	else
		rawmidi->mode &= ~SND_RAWMIDI_NONBLOCK;
	return 0;
}

/**
 * \brief get size of the snd_rawmidi_info_t structure in bytes
 * \return size of the snd_rawmidi_info_t structure in bytes
 */
size_t snd_rawmidi_info_sizeof()
{
	return sizeof(snd_rawmidi_info_t);
}

/**
 * \brief allocate a new snd_rawmidi_info_t structure
 * \param info returned pointer
 * \return 0 on success otherwise a negative error code if fails
 *
 * Allocates a new snd_rawmidi_params_t structure using the standard
 * malloc C library function.
 */
int snd_rawmidi_info_malloc(snd_rawmidi_info_t **info)
{
	assert(info);
	*info = calloc(1, sizeof(snd_rawmidi_info_t));
	if (!*info)
		return -ENOMEM;
	return 0;
}

/**
 * \brief frees the snd_rawmidi_info_t structure
 * \param info pointer to the snd_rawmidi_info_t structure to free
 *
 * Frees the given snd_rawmidi_params_t structure using the standard
 * free C library function.
 */
void snd_rawmidi_info_free(snd_rawmidi_info_t *info)
{
	assert(info);
	free(info);
}

/**
 * \brief copy one snd_rawmidi_info_t structure to another
 * \param dst destination snd_rawmidi_info_t structure
 * \param src source snd_rawmidi_info_t structure
 */
void snd_rawmidi_info_copy(snd_rawmidi_info_t *dst, const snd_rawmidi_info_t *src)
{
	assert(dst && src);
	*dst = *src;
}

/**
 * \brief get rawmidi device number
 * \param info pointer to a snd_rawmidi_info_t structure
 * \return rawmidi device number
 */
unsigned int snd_rawmidi_info_get_device(const snd_rawmidi_info_t *info)
{
	assert(info);
	return info->device;
}

/**
 * \brief get rawmidi subdevice number
 * \param info pointer to a snd_rawmidi_info_t structure
 * \return rawmidi subdevice number
 */
unsigned int snd_rawmidi_info_get_subdevice(const snd_rawmidi_info_t *info)
{
	assert(info);
	return info->subdevice;
}

/**
 * \brief get rawmidi stream identification
 * \param info pointer to a snd_rawmidi_info_t structure
 * \return rawmidi stream identification
 */
snd_rawmidi_stream_t snd_rawmidi_info_get_stream(const snd_rawmidi_info_t *info)
{
	assert(info);
	return info->stream;
}

/**
 * \brief get rawmidi card number
 * \param info pointer to a snd_rawmidi_info_t structure
 * \return rawmidi card number
 */
int snd_rawmidi_info_get_card(const snd_rawmidi_info_t *info)
{
	assert(info);
	return info->card;
}

/**
 * \brief get rawmidi flags
 * \param info pointer to a snd_rawmidi_info_t structure
 * \return rawmidi flags
 */
unsigned int snd_rawmidi_info_get_flags(const snd_rawmidi_info_t *info)
{
	assert(info);
	return info->flags;
}

/**
 * \brief get rawmidi hardware driver identifier
 * \param info pointer to a snd_rawmidi_info_t structure
 * \return rawmidi hardware driver identifier
 */
const char *snd_rawmidi_info_get_id(const snd_rawmidi_info_t *info)
{
	assert(info);
	return (const char *)info->id;
}

/**
 * \brief get rawmidi hardware driver name
 * \param info pointer to a snd_rawmidi_info_t structure
 * \return rawmidi hardware driver name
 */
const char *snd_rawmidi_info_get_name(const snd_rawmidi_info_t *info)
{
	assert(info);
	return (const char *)info->name;
}

/**
 * \brief get rawmidi subdevice name
 * \param info pointer to a snd_rawmidi_info_t structure
 * \return rawmidi subdevice name
 */
const char *snd_rawmidi_info_get_subdevice_name(const snd_rawmidi_info_t *info)
{
	assert(info);
	return (const char *)info->subname;
}

/**
 * \brief get rawmidi count of subdevices
 * \param info pointer to a snd_rawmidi_info_t structure
 * \return rawmidi count of subdevices
 */
unsigned int snd_rawmidi_info_get_subdevices_count(const snd_rawmidi_info_t *info)
{
	assert(info);
	return info->subdevices_count;
}

/**
 * \brief get rawmidi available count of subdevices
 * \param info pointer to a snd_rawmidi_info_t structure
 * \return rawmidi available count of subdevices
 */
unsigned int snd_rawmidi_info_get_subdevices_avail(const snd_rawmidi_info_t *info)
{
	assert(info);
	return info->subdevices_avail;
}

/**
 * \brief get the tied device number for the given rawmidi device
 * \param info pointer to a snd_rawmidi_info_t structure
 * \return the device number for the tied device, or -1 if untied / unknown.
 *
 * This function is useful for UMP rawmidi devices where each of them may
 * have the mirroring legacy rawmidi device. Those are shown as "tied".
 */
int snd_rawmidi_info_get_tied_device(const snd_rawmidi_info_t *info)
{
	assert(info);
	if (info->tied_device > 0)
		return info->tied_device - 1;
	return -1;
}

/**
 * \brief set rawmidi device number
 * \param info pointer to a snd_rawmidi_info_t structure
 * \param val device number
 */
void snd_rawmidi_info_set_device(snd_rawmidi_info_t *info, unsigned int val)
{
	assert(info);
	info->device = val;
}

/**
 * \brief set rawmidi subdevice number
 * \param info pointer to a snd_rawmidi_info_t structure
 * \param val subdevice number
 */
void snd_rawmidi_info_set_subdevice(snd_rawmidi_info_t *info, unsigned int val)
{
	assert(info);
	info->subdevice = val;
}

/**
 * \brief set rawmidi stream identifier
 * \param info pointer to a snd_rawmidi_info_t structure
 * \param val rawmidi stream identifier
 */
void snd_rawmidi_info_set_stream(snd_rawmidi_info_t *info, snd_rawmidi_stream_t val)
{
	assert(info);
	info->stream = val;
}

/**
 * \brief get information about RawMidi handle
 * \param rawmidi RawMidi handle
 * \param info pointer to a snd_rawmidi_info_t structure to be filled
 * \return 0 on success otherwise a negative error code
 */
int snd_rawmidi_info(snd_rawmidi_t *rawmidi, snd_rawmidi_info_t * info)
{
	assert(rawmidi);
	assert(info);
	return rawmidi->ops->info(rawmidi, info);
}

/**
 * \brief get size of the snd_rawmidi_params_t structure in bytes
 * \return size of the snd_rawmidi_params_t structure in bytes
 */
size_t snd_rawmidi_params_sizeof()
{
	return sizeof(snd_rawmidi_params_t);
}

/**
 * \brief allocate the snd_rawmidi_params_t structure
 * \param params returned pointer
 * \return 0 on success otherwise a negative error code if fails
 *
 * Allocates a new snd_rawmidi_params_t structure using the standard
 * malloc C library function.
 */
int snd_rawmidi_params_malloc(snd_rawmidi_params_t **params)
{
	assert(params);
	*params = calloc(1, sizeof(snd_rawmidi_params_t));
	if (!*params)
		return -ENOMEM;
	return 0;
}

/**
 * \brief frees the snd_rawmidi_params_t structure
 * \param params pointer to the #snd_rawmidi_params_t structure to free
 *
 * Frees the given snd_rawmidi_params_t structure using the standard
 * free C library function.
 */
void snd_rawmidi_params_free(snd_rawmidi_params_t *params)
{
	assert(params);
	free(params);
}

/**
 * \brief copy one snd_rawmidi_params_t structure to another
 * \param dst destination snd_rawmidi_params_t structure
 * \param src source snd_rawmidi_params_t structure
 */
void snd_rawmidi_params_copy(snd_rawmidi_params_t *dst, const snd_rawmidi_params_t *src)
{
	assert(dst && src);
	*dst = *src;
}

/**
 * \brief set rawmidi I/O ring buffer size
 * \param rawmidi RawMidi handle
 * \param params pointer to a snd_rawmidi_params_t structure
 * \param val size in bytes
 * \return 0 on success otherwise a negative error code
 */
#ifndef DOXYGEN
int snd_rawmidi_params_set_buffer_size(snd_rawmidi_t *rawmidi ATTRIBUTE_UNUSED, snd_rawmidi_params_t *params, size_t val)
#else
int snd_rawmidi_params_set_buffer_size(snd_rawmidi_t *rawmidi, snd_rawmidi_params_t *params, size_t val)
#endif
{
	assert(rawmidi && params);
	assert(val > params->avail_min);
	params->buffer_size = val;
	return 0;
}

/**
 * \brief get rawmidi I/O ring buffer size
 * \param params pointer to a snd_rawmidi_params_t structure
 * \return size of rawmidi I/O ring buffer in bytes
 */
size_t snd_rawmidi_params_get_buffer_size(const snd_rawmidi_params_t *params)
{
	assert(params);
	return params->buffer_size;
}

/**
 * \brief set minimum available bytes in rawmidi I/O ring buffer for wakeup
 * \param rawmidi RawMidi handle
 * \param params pointer to a snd_rawmidi_params_t structure
 * \param val desired value
 */
#ifndef DOXYGEN
int snd_rawmidi_params_set_avail_min(snd_rawmidi_t *rawmidi ATTRIBUTE_UNUSED, snd_rawmidi_params_t *params, size_t val)
#else
int snd_rawmidi_params_set_avail_min(snd_rawmidi_t *rawmidi, snd_rawmidi_params_t *params, size_t val)
#endif
{
	assert(rawmidi && params);
	assert(val < params->buffer_size);
	params->avail_min = val;
	return 0;
}

/**
 * \brief get minimum available bytes in rawmidi I/O ring buffer for wakeup
 * \param params pointer to snd_rawmidi_params_t structure
 * \return minimum available bytes
 */
size_t snd_rawmidi_params_get_avail_min(const snd_rawmidi_params_t *params)
{
	assert(params);
	return params->avail_min;
}

/**
 * \brief set no-active-sensing action on snd_rawmidi_close()
 * \param rawmidi RawMidi handle
 * \param params pointer to snd_rawmidi_params_t structure
 * \param val value: 0 = enable to send the active sensing message, 1 = disable
 * \return 0 on success otherwise a negative error code
 */
#ifndef DOXYGEN
int snd_rawmidi_params_set_no_active_sensing(snd_rawmidi_t *rawmidi ATTRIBUTE_UNUSED, snd_rawmidi_params_t *params, int val)
#else
int snd_rawmidi_params_set_no_active_sensing(snd_rawmidi_t *rawmidi, snd_rawmidi_params_t *params, int val)
#endif
{
	assert(rawmidi && params);
	params->no_active_sensing = val;
	return 0;
}

/**
 * \brief get no-active-sensing action status
 * \param params pointer to snd_rawmidi_params_t structure
 * \return the current status (0 = enable, 1 = disable the active sensing message)
 */
int snd_rawmidi_params_get_no_active_sensing(const snd_rawmidi_params_t *params)
{
	assert(params);
	return params->no_active_sensing;
}

/**
 * \brief set read mode
 * \param rawmidi RawMidi handle
 * \param params pointer to snd_rawmidi_params_t structure
 * \param val type of read_mode
 * \return 0 on success, otherwise a negative error code.
 *
 * Notable error codes:
 * -EINVAL - "val" is invalid
 * -ENOTSUP - mode is not supported
 *
 */
int snd_rawmidi_params_set_read_mode(const snd_rawmidi_t *rawmidi, snd_rawmidi_params_t *params, snd_rawmidi_read_mode_t val)
{
	unsigned int framing;
	assert(rawmidi && params);

	switch (val) {
	case SND_RAWMIDI_READ_STANDARD:
		framing = SNDRV_RAWMIDI_MODE_FRAMING_NONE;
		break;
	case SND_RAWMIDI_READ_TSTAMP:
		if (rawmidi->ops->tread == NULL)
			return -ENOTSUP;
		framing = SNDRV_RAWMIDI_MODE_FRAMING_TSTAMP;
		break;
	default:
		return -EINVAL;
	}

	if (framing != SNDRV_RAWMIDI_MODE_FRAMING_NONE &&
		(rawmidi->version < SNDRV_PROTOCOL_VERSION(2, 0, 2) || rawmidi->stream != SND_RAWMIDI_STREAM_INPUT))
		return -ENOTSUP;
	params->mode = (params->mode & ~SNDRV_RAWMIDI_MODE_FRAMING_MASK) | framing;
	return 0;
}

/**
 * \brief get current read mode
 * \param params pointer to snd_rawmidi_params_t structure
 * \return the current read mode (see enum)
 */
snd_rawmidi_read_mode_t snd_rawmidi_params_get_read_mode(const snd_rawmidi_params_t *params)
{
	unsigned int framing;

	assert(params);
	framing = params->mode & SNDRV_RAWMIDI_MODE_FRAMING_MASK;
	if (framing == SNDRV_RAWMIDI_MODE_FRAMING_TSTAMP)
		return SND_RAWMIDI_READ_TSTAMP;
	return SND_RAWMIDI_READ_STANDARD;
}

/**
 * \brief sets clock type for tstamp type framing
 * \param rawmidi RawMidi handle
 * \param params pointer to snd_rawmidi_params_t structure
 * \param val one of the SND_RAWMIDI_CLOCK_* constants
 * \return 0 on success, otherwise a negative error code.
 *
 * Notable error codes:
 * -EINVAL - "val" is invalid
 * -ENOTSUP - Kernel is too old to support framing.
 *
 */
int snd_rawmidi_params_set_clock_type(const snd_rawmidi_t *rawmidi, snd_rawmidi_params_t *params, snd_rawmidi_clock_t val)
{
	assert(rawmidi && params);
	if (val > SNDRV_RAWMIDI_MODE_CLOCK_MASK >> SNDRV_RAWMIDI_MODE_CLOCK_SHIFT)
		return -EINVAL;
	if (val != SNDRV_RAWMIDI_MODE_CLOCK_NONE &&
		(rawmidi->version < SNDRV_PROTOCOL_VERSION(2, 0, 2) || rawmidi->stream != SND_RAWMIDI_STREAM_INPUT))
		return -ENOTSUP;
	params->mode = (params->mode & ~SNDRV_RAWMIDI_MODE_CLOCK_MASK) + (val << SNDRV_RAWMIDI_MODE_CLOCK_SHIFT);
	return 0;
}

/**
 * \brief get current clock type (for tstamp type framing)
 * \param params pointer to snd_rawmidi_params_t structure
 * \return the current clock type (one of the SND_RAWMIDI_CLOCK_* constants)
 */
snd_rawmidi_clock_t snd_rawmidi_params_get_clock_type(const snd_rawmidi_params_t *params)
{
	assert(params);
	return (params->mode & SNDRV_RAWMIDI_MODE_CLOCK_MASK) >> SNDRV_RAWMIDI_MODE_CLOCK_SHIFT;
}


/**
 * \brief set parameters about rawmidi stream
 * \param rawmidi RawMidi handle
 * \param params pointer to a snd_rawmidi_params_t structure to be filled
 * \return 0 on success otherwise a negative error code
 */
int snd_rawmidi_params(snd_rawmidi_t *rawmidi, snd_rawmidi_params_t * params)
{
	int err;
	assert(rawmidi);
	assert(params);
	err = rawmidi->ops->params(rawmidi, params);
	if (err < 0)
		return err;
	rawmidi->buffer_size = params->buffer_size;
	rawmidi->avail_min = params->avail_min;
	rawmidi->no_active_sensing = params->no_active_sensing;
	rawmidi->params_mode = rawmidi->version < SNDRV_PROTOCOL_VERSION(2, 0, 2) ? 0 : params->mode;
	return 0;
}

/**
 * \brief get current parameters about rawmidi stream
 * \param rawmidi RawMidi handle
 * \param params pointer to a snd_rawmidi_params_t structure to be filled
 * \return 0 on success otherwise a negative error code
 */
int snd_rawmidi_params_current(snd_rawmidi_t *rawmidi, snd_rawmidi_params_t *params)
{
	assert(rawmidi);
	assert(params);
	params->buffer_size = rawmidi->buffer_size;
	params->avail_min = rawmidi->avail_min;
	params->no_active_sensing = rawmidi->no_active_sensing;
	params->mode = rawmidi->params_mode;
	return 0;
}

/**
 * \brief get size of the snd_rawmidi_status_t structure in bytes
 * \return size of the snd_rawmidi_status_t structure in bytes
 */
size_t snd_rawmidi_status_sizeof()
{
	return sizeof(snd_rawmidi_status_t);
}

/**
 * \brief allocate the snd_rawmidi_status_t structure
 * \param ptr returned pointer
 * \return 0 on success otherwise a negative error code if fails
 *
 * Allocates a new snd_rawmidi_status_t structure using the standard
 * malloc C library function.
 */
int snd_rawmidi_status_malloc(snd_rawmidi_status_t **ptr)
{
	assert(ptr);
	*ptr = calloc(1, sizeof(snd_rawmidi_status_t));
	if (!*ptr)
		return -ENOMEM;
	return 0;
}

/**
 * \brief frees the snd_rawmidi_status_t structure
 * \param status pointer to the snd_rawmidi_status_t structure to free
 *
 * Frees the given snd_rawmidi_status_t structure using the standard
 * free C library function.
 */
void snd_rawmidi_status_free(snd_rawmidi_status_t *status)
{
	assert(status);
	free(status);
}

/**
 * \brief copy one snd_rawmidi_status_t structure to another
 * \param dst destination snd_rawmidi_status_t structure
 * \param src source snd_rawmidi_status_t structure
 */
void snd_rawmidi_status_copy(snd_rawmidi_status_t *dst, const snd_rawmidi_status_t *src)
{
	assert(dst && src);
	*dst = *src;
}

/**
 * \brief get the start timestamp
 * \param status pointer to a snd_rawmidi_status_t structure
 * \param tstamp returned timestamp value
 */
void snd_rawmidi_status_get_tstamp(const snd_rawmidi_status_t *status, snd_htimestamp_t *tstamp)
{
	assert(status && tstamp);
	*tstamp = status->tstamp;
}

/**
 * \brief get current available bytes in the rawmidi I/O ring buffer
 * \param status pointer to a snd_rawmidi_status_t structure
 * \return current available bytes in the rawmidi I/O ring buffer
 */
size_t snd_rawmidi_status_get_avail(const snd_rawmidi_status_t *status)
{
	assert(status);
	return status->avail;
}

/**
 * \brief get count of xruns
 * \param status pointer to a snd_rawmidi_status_t structure
 * \return count of xruns
 */
size_t snd_rawmidi_status_get_xruns(const snd_rawmidi_status_t *status)
{
	assert(status);
	return status->xruns;
}

/**
 * \brief get status of rawmidi stream
 * \param rawmidi RawMidi handle
 * \param status pointer to a snd_rawmidi_status_t structure to be filled
 * \return 0 on success otherwise a negative error code
 */
int snd_rawmidi_status(snd_rawmidi_t *rawmidi, snd_rawmidi_status_t * status)
{
	assert(rawmidi);
	assert(status);
	return rawmidi->ops->status(rawmidi, status);
}

/**
 * \brief drop all bytes in the rawmidi I/O ring buffer immediately
 * \param rawmidi RawMidi handle
 * \return 0 on success otherwise a negative error code
 */
int snd_rawmidi_drop(snd_rawmidi_t *rawmidi)
{
	assert(rawmidi);
	return rawmidi->ops->drop(rawmidi);
}

/**
 * \brief drain all bytes in the rawmidi I/O ring buffer
 * \param rawmidi RawMidi handle
 * \return 0 on success otherwise a negative error code
 *
 * Waits until all MIDI bytes are not drained (sent) to the
 * hardware device.
 */
int snd_rawmidi_drain(snd_rawmidi_t *rawmidi)
{
	assert(rawmidi);
	return rawmidi->ops->drain(rawmidi);
}

/**
 * \brief write MIDI bytes to MIDI stream
 * \param rawmidi RawMidi handle
 * \param buffer buffer containing MIDI bytes
 * \param size output buffer size in bytes
 */
ssize_t snd_rawmidi_write(snd_rawmidi_t *rawmidi, const void *buffer, size_t size)
{
	assert(rawmidi);
	assert(rawmidi->stream == SND_RAWMIDI_STREAM_OUTPUT);
	assert(buffer || size == 0);
	return rawmidi->ops->write(rawmidi, buffer, size);
}

/**
 * \brief read MIDI bytes from MIDI stream
 * \param rawmidi RawMidi handle
 * \param buffer buffer to store the input MIDI bytes
 * \param size input buffer size in bytes
 * \retval count of MIDI bytes otherwise a negative error code
 */
ssize_t snd_rawmidi_read(snd_rawmidi_t *rawmidi, void *buffer, size_t size)
{
	assert(rawmidi);
	assert(rawmidi->stream == SND_RAWMIDI_STREAM_INPUT);
	if ((rawmidi->params_mode & SNDRV_RAWMIDI_MODE_FRAMING_MASK) == SNDRV_RAWMIDI_MODE_FRAMING_TSTAMP)
		size &= ~(sizeof(struct snd_rawmidi_framing_tstamp) - 1);
	assert(buffer || size == 0);
	return (rawmidi->ops->read)(rawmidi, buffer, size);
}

/**
 * \brief read MIDI bytes from MIDI stream with timestamp
 * \param rawmidi RawMidi handle
 * \param[out] tstamp timestamp for the returned MIDI bytes
 * \param buffer buffer to store the input MIDI bytes
 * \param size input buffer size in bytes
 * \retval count of MIDI bytes otherwise a negative error code
 */
ssize_t snd_rawmidi_tread(snd_rawmidi_t *rawmidi, struct timespec *tstamp, void *buffer, size_t size)
{
	assert(rawmidi);
	assert(rawmidi->stream == SND_RAWMIDI_STREAM_INPUT);
	assert(buffer || size == 0);
	if ((rawmidi->params_mode & SNDRV_RAWMIDI_MODE_FRAMING_MASK) != SNDRV_RAWMIDI_MODE_FRAMING_TSTAMP)
		return -EINVAL;
	if (rawmidi->ops->tread == NULL)
		return -ENOTSUP;
	return (rawmidi->ops->tread)(rawmidi, tstamp, buffer, size);
}

#ifndef DOXYGEN
/*
 * internal API functions for obtaining UMP info from rawmidi instance
 */
int _snd_rawmidi_ump_endpoint_info(snd_rawmidi_t *rmidi, void *info)
{
	if (!rmidi->ops->ump_endpoint_info)
		return -ENXIO;
	return rmidi->ops->ump_endpoint_info(rmidi, info);
}

int _snd_rawmidi_ump_block_info(snd_rawmidi_t *rmidi, void *info)
{
	if (!rmidi->ops->ump_block_info)
		return -ENXIO;
	return rmidi->ops->ump_block_info(rmidi, info);
}
#endif /* DOXYGEN */
