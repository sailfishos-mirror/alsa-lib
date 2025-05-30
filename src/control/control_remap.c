/**
 * \file control/control_remap.c
 * \brief CTL Remap Plugin Interface
 * \author Jaroslav Kysela <perex@perex.cz>
 * \date 2021-2025
 */
/*
 *  Control - Remap Controls
 *  Copyright (c) 2021-2025 by Jaroslav Kysela <perex@perex.cz>
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

#include "control_local.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>

#ifndef DOC_HIDDEN
#if 0
#define REMAP_DEBUG 1
#define debug(format, args...) fprintf(stderr, format, ##args)
#define debug_id(id, format, args...) do { \
	char *s = snd_ctl_ascii_elem_id_get(id); \
	fprintf(stderr, "%s: ", s); free(s); \
	fprintf(stderr, format, ##args); \
} while (0)
#else
#define REMAP_DEBUG 0
#define debug(format, args...) do { } while (0)
#define debug_id(id, format, args...) do { } while (0)
#endif

#define EREMAPNOTFOUND (888899)
#endif /* DOC_HIDDEN */

#ifndef PIC
/* entry for static linking */
const char *_snd_module_control_remap = "";
#endif

#ifndef DOC_HIDDEN
typedef struct {
	unsigned int numid_child;
	unsigned int numid_app;
} snd_ctl_numid_t;

typedef struct {
	snd_ctl_elem_id_t id_child;
	snd_ctl_elem_id_t id_app;
} snd_ctl_remap_id_t;

typedef struct {
	snd_ctl_elem_id_t map_id;
	snd_ctl_elem_type_t type;
	size_t controls_items;
	size_t controls_alloc;
	struct snd_ctl_map_ctl {
		snd_ctl_elem_id_t id_child;
		size_t channel_map_items;
		size_t channel_map_alloc;
		unsigned int src_channels;
		long *channel_map;
	} *controls;
} snd_ctl_map_t;

typedef struct {
	snd_ctl_elem_id_t *id;
	unsigned int numid_app;
	unsigned int event_mask;
} snd_ctl_map_event_t;

typedef struct {
	size_t control_items;
	snd_ctl_elem_id_t *control_ids;
	snd_ctl_elem_id_t switch_id;
	bool switch_state;
} snd_ctl_sync_t;

typedef struct {
	snd_ctl_t *child;
	int numid_remap_active;
	unsigned int numid_app_last;

	size_t numid_items;
	size_t numid_alloc;
	snd_ctl_numid_t *numid;
	snd_ctl_numid_t numid_temp;

	size_t remap_items;
	size_t remap_alloc;
	snd_ctl_remap_id_t *remap;

	size_t map_items;
	size_t map_alloc;
	snd_ctl_map_t *map;

	size_t sync_items;
	size_t sync_alloc;
	snd_ctl_sync_t *sync;
	size_t sync_switch_items;

	size_t event_items;
	size_t event_queue_head;
	size_t event_queue_tail;
	snd_ctl_map_event_t *event_queue;
} snd_ctl_remap_t;
#endif

static snd_ctl_numid_t *remap_numid_temp(snd_ctl_remap_t *priv, unsigned int numid)
{
	priv->numid_temp.numid_child = numid;
	priv->numid_temp.numid_app = numid;
	return &priv->numid_temp;
}

static snd_ctl_numid_t *remap_find_numid_app(snd_ctl_remap_t *priv, unsigned int numid_app)
{
	snd_ctl_numid_t *numid;
	size_t count;

	if (!priv->numid_remap_active)
		return remap_numid_temp(priv, numid_app);
	numid = priv->numid;
	for (count = priv->numid_items; count > 0; count--, numid++)
		if (numid_app == numid->numid_app)
			return numid;
	return NULL;
}

static snd_ctl_numid_t *remap_numid_new(snd_ctl_remap_t *priv, unsigned int numid_child,
					unsigned int numid_app)
{
	snd_ctl_numid_t *numid;

	if (priv->numid_alloc == priv->numid_items) {
		numid = realloc(priv->numid, (priv->numid_alloc + 16) * sizeof(*numid));
		if (numid == NULL)
			return NULL;
		memset(numid + priv->numid_alloc, 0, sizeof(*numid) * 16);
		priv->numid_alloc += 16;
		priv->numid = numid;
	}
	numid = &priv->numid[priv->numid_items++];
	numid->numid_child = numid_child;
	numid->numid_app = numid_app;
	debug("new numid: child %u app %u\n", numid->numid_child, numid->numid_app);
	return numid;
}

static snd_ctl_numid_t *remap_numid_child_new(snd_ctl_remap_t *priv, unsigned int numid_child)
{
	unsigned int numid_app;

	if (numid_child == 0)
		return NULL;
	if (priv->numid_remap_active && remap_find_numid_app(priv, numid_child)) {
		while (remap_find_numid_app(priv, priv->numid_app_last))
			priv->numid_app_last++;
		numid_app = priv->numid_app_last;
	} else {
		numid_app = numid_child;
	}
	return remap_numid_new(priv, numid_child, numid_app);
}

static snd_ctl_numid_t *remap_find_numid_child(snd_ctl_remap_t *priv, unsigned int numid_child)
{
	snd_ctl_numid_t *numid;
	size_t count;

	if (!priv->numid_remap_active)
		return remap_numid_temp(priv, numid_child);
	numid = priv->numid;
	for (count = priv->numid_items; count > 0; count--, numid++)
		if (numid_child == numid->numid_child)
			return numid;
	return remap_numid_child_new(priv, numid_child);
}

static void remap_forget_numid_child(snd_ctl_remap_t *priv, unsigned int numid_child)
{
	snd_ctl_numid_t *numid;
	size_t index;

	if (!priv->numid_remap_active)
		return;
	numid = priv->numid;
	for (index = 0; index < priv->numid_items; index++) {
		if (numid[index].numid_child != numid_child)
			continue;
		memcpy(&priv->numid[index], &priv->numid[index + 1],
				(priv->numid_items - 1 - index) * sizeof(*numid));
		priv->numid_items++;
	}
}

static snd_ctl_remap_id_t *remap_find_id_child(snd_ctl_remap_t *priv, snd_ctl_elem_id_t *id)
{
	size_t count;
	snd_ctl_remap_id_t *rid;

	if (id->numid > 0) {
		rid = priv->remap;
		for (count = priv->remap_items; count > 0; count--, rid++)
			if (id->numid == rid->id_child.numid)
				return rid;
	}
	rid = priv->remap;
	for (count = priv->remap_items; count > 0; count--, rid++)
		if (snd_ctl_elem_id_compare_set(id, &rid->id_child) == 0)
			return rid;
	return NULL;
}

static snd_ctl_remap_id_t *remap_find_id_app(snd_ctl_remap_t *priv, snd_ctl_elem_id_t *id)
{
	size_t count;
	snd_ctl_remap_id_t *rid;

	if (id->numid > 0) {
		rid = priv->remap;
		for (count = priv->remap_items; count > 0; count--, rid++)
			if (id->numid == rid->id_app.numid)
				return rid;
	}
	rid = priv->remap;
	for (count = priv->remap_items; count > 0; count--, rid++)
		if (snd_ctl_elem_id_compare_set(id, &rid->id_app) == 0)
			return rid;
	return NULL;
}

static snd_ctl_map_t *remap_find_map_numid(snd_ctl_remap_t *priv, unsigned int numid)
{
	size_t count;
	snd_ctl_map_t *map;

	if (numid == 0)
		return NULL;
	map = priv->map;
	for (count = priv->map_items; count > 0; count--, map++) {
		if (numid == map->map_id.numid)
			return map;
	}
	return NULL;
}

static snd_ctl_map_t *remap_find_map_id(snd_ctl_remap_t *priv, snd_ctl_elem_id_t *id)
{
	size_t count;
	snd_ctl_map_t *map;

	if (id->numid > 0)
		return remap_find_map_numid(priv, id->numid);
	map = priv->map;
	for (count = priv->map_items; count > 0; count--, map++)
		if (snd_ctl_elem_id_compare_set(id, &map->map_id) == 0)
			return map;
	return NULL;
}

static int remap_id_to_child(snd_ctl_remap_t *priv, snd_ctl_elem_id_t *id, snd_ctl_remap_id_t **_rid)
{
	snd_ctl_remap_id_t *rid;
	snd_ctl_numid_t *numid;

	debug_id(id, "%s enter\n", __func__);
	rid = remap_find_id_app(priv, id);
	if (rid) {
		if (rid->id_app.numid == 0) {
			numid = remap_find_numid_app(priv, id->numid);
			if (numid) {
				rid->id_child.numid = numid->numid_child;
				rid->id_app.numid = numid->numid_app;
			}
		}
		*id = rid->id_child;
	} else {
		if (remap_find_id_child(priv, id))
			return -ENOENT;
		numid = remap_find_numid_app(priv, id->numid);
		if (numid)
			id->numid = numid->numid_child;
		else
			id->numid = 0;
	}
	*_rid = rid;
	debug_id(id, "%s leave\n", __func__);
	return 0;
}

static int remap_id_to_app(snd_ctl_remap_t *priv, snd_ctl_elem_id_t *id, snd_ctl_remap_id_t *rid, int err)
{
	snd_ctl_numid_t *numid;

	if (rid) {
		if (err >= 0 && rid->id_app.numid == 0) {
			numid = remap_numid_child_new(priv, id->numid);
			if (numid == NULL)
				return -EIO;
			rid->id_child.numid = numid->numid_child;
			rid->id_app.numid = numid->numid_app;
		}
		*id = rid->id_app;
	} else {
		if (err >= 0) {
			numid = remap_find_numid_child(priv, id->numid);
			if (numid == NULL)
				return -EIO;
			id->numid = numid->numid_app;
		}
	}
	return err;
}

static snd_ctl_sync_t *remap_find_sync_numid(snd_ctl_remap_t *priv, unsigned int numid)
{
	size_t count, index2;
	snd_ctl_sync_t *sync;

	if (numid == 0)
		return NULL;
	sync = priv->sync;
	for (count = priv->sync_items; count > 0; count--, sync++)
		for (index2 = 0; index2 < sync->control_items; index2++)
			if (numid == sync->control_ids[index2].numid)
				return sync;
	return NULL;
}

static snd_ctl_sync_t *remap_find_sync_id(snd_ctl_remap_t *priv, snd_ctl_elem_id_t *id)
{
	size_t count, index2;
	snd_ctl_sync_t *sync;

	if (id->numid > 0)
		return remap_find_sync_numid(priv, id->numid);
	sync = priv->sync;
	for (count = priv->sync_items; count > 0; count--, sync++)
		for (index2 = 0; index2 < sync->control_items; index2++)
			if (snd_ctl_elem_id_compare_set(id, &sync->control_ids[index2]) == 0)
				return sync;
	return NULL;
}

static void remap_update_sync_id(snd_ctl_remap_t *priv, snd_ctl_elem_id_t *id)
{
	size_t count, index2;
	snd_ctl_sync_t *sync;

	if (id->numid == 0)
		return;
	sync = priv->sync;
	for (count = priv->sync_items; count > 0; count--, sync++)
		for (index2 = 0; index2 < sync->control_items; index2++) {
			if (snd_ctl_elem_id_compare_set(id, &sync->control_ids[index2]) == 0) {
				sync->control_ids[index2].numid = id->numid;
				break;
			}
		}
}

static snd_ctl_sync_t *remap_find_sync_switch_numid(snd_ctl_remap_t *priv, unsigned int numid)
{
	size_t count;
	snd_ctl_sync_t *sync;

	if (numid == 0)
		return NULL;
	sync = priv->sync;
	for (count = priv->sync_items; count > 0; count--, sync++)
		if (numid == sync->switch_id.numid)
			return sync;
	return NULL;
}

static snd_ctl_sync_t *remap_find_sync_switch_id(snd_ctl_remap_t *priv, snd_ctl_elem_id_t *id)
{
	size_t count;
	snd_ctl_sync_t *sync;

	if (id->numid > 0)
		return remap_find_sync_switch_numid(priv, id->numid);
	sync = priv->sync;
	for (count = priv->sync_items; count > 0; count--, sync++) {
		if (sync->switch_id.numid == 0)
			continue;
		if (snd_ctl_elem_id_compare_set(id, &sync->switch_id) == 0)
			return sync;
	}
	return NULL;
}

static void remap_free(snd_ctl_remap_t *priv)
{
	size_t idx1, idx2;
	snd_ctl_sync_t *sync;
	snd_ctl_map_t *map;

	for (idx1 = 0; idx1 < priv->sync_items; idx1++) {
		sync = &priv->sync[idx1];
		free(sync->control_ids);
	}
	for (idx1 = 0; idx1 < priv->map_items; idx1++) {
		map = &priv->map[idx1];
		for (idx2 = 0; idx2 < map->controls_items; idx2++)
			free(map->controls[idx2].channel_map);
		free(map->controls);
	}
	free(priv->event_queue);
	free(priv->sync);
	free(priv->map);
	free(priv->remap);
	free(priv->numid);
	free(priv);
}

static int snd_ctl_remap_close(snd_ctl_t *ctl)
{
	snd_ctl_remap_t *priv = ctl->private_data;
	int err = snd_ctl_close(priv->child);
	remap_free(priv);
	return err;
}

static int snd_ctl_remap_nonblock(snd_ctl_t *ctl, int nonblock)
{
	snd_ctl_remap_t *priv = ctl->private_data;
	return snd_ctl_nonblock(priv->child, nonblock);
}

static int snd_ctl_remap_async(snd_ctl_t *ctl, int sig, pid_t pid)
{
	snd_ctl_remap_t *priv = ctl->private_data;
	return snd_ctl_async(priv->child, sig, pid);
}

static int snd_ctl_remap_subscribe_events(snd_ctl_t *ctl, int subscribe)
{
	snd_ctl_remap_t *priv = ctl->private_data;
	return snd_ctl_subscribe_events(priv->child, subscribe);
}

static int snd_ctl_remap_card_info(snd_ctl_t *ctl, snd_ctl_card_info_t *info)
{
	snd_ctl_remap_t *priv = ctl->private_data;
	return snd_ctl_card_info(priv->child, info);
}

static int snd_ctl_remap_elem_list(snd_ctl_t *ctl, snd_ctl_elem_list_t *list)
{
	snd_ctl_remap_t *priv = ctl->private_data;
	snd_ctl_elem_id_t *id;
	snd_ctl_remap_id_t *rid;
	snd_ctl_numid_t *numid;
	unsigned int index;
	size_t index2;
	int err;

	err = snd_ctl_elem_list(priv->child, list);
	if (err < 0)
		return err;
	for (index = 0; index < list->used; index++) {
		id = &list->pids[index];
		rid = remap_find_id_child(priv, id);
		if (rid) {
			rid->id_app.numid = id->numid;
			*id = rid->id_app;
		}
		numid = remap_find_numid_child(priv, id->numid);
		if (numid == NULL)
			return -EIO;
		id->numid = numid->numid_app;
	}
	if (list->offset >= list->count + priv->map_items + priv->sync_switch_items)
		return 0;
	index2 = 0;
	if (list->offset > list->count)
		index2 = list->offset - list->count;
	for ( ; index < list->space && index2 < priv->map_items; index2++, index++) {
		snd_ctl_map_t *map = &priv->map[index2];
		list->pids[index] = map->map_id;
		list->used++;
	}
	if (index2 >= priv->map_items) {
		index2 -= priv->map_items;
		for ( ; index < list->space && index2 < priv->sync_switch_items; index2++, index++) {
			snd_ctl_sync_t *sync = &priv->sync[index2];
			list->pids[index] = sync->switch_id;
			list->used++;
		}
	}
	list->count += priv->map_items + priv->sync_switch_items;
	return 0;
}

#ifndef DOC_HIDDEN
#define ACCESS_BITS(bits) \
	(bits & (SNDRV_CTL_ELEM_ACCESS_READWRITE|\
		 SNDRV_CTL_ELEM_ACCESS_VOLATILE|\
		 SNDRV_CTL_ELEM_ACCESS_TLV_READWRITE))
#endif /* DOC_HIDDEN */

static int remap_map_elem_info(snd_ctl_remap_t *priv, snd_ctl_elem_info_t *info)
{
	snd_ctl_map_t *map;
	snd_ctl_elem_info_t info2, info3;
	size_t item;
	unsigned int access;
	size_t count;
	int owner, err;

	map = remap_find_map_id(priv, &info->id);
	if (map == NULL)
		return -EREMAPNOTFOUND;
	debug_id(&info->id, "%s\n", __func__);
	assert(map->controls_items > 0);
	snd_ctl_elem_info_clear(&info2);
	info2.id = map->controls[0].id_child;
	debug_id(&info2.id, "%s controls[0]\n", __func__);
	err = snd_ctl_elem_info(priv->child, &info2);
	if (err < 0)
		return err;
	if (info2.type != SNDRV_CTL_ELEM_TYPE_BOOLEAN &&
	    info2.type != SNDRV_CTL_ELEM_TYPE_INTEGER &&
	    info2.type != SNDRV_CTL_ELEM_TYPE_INTEGER64 &&
	    info2.type != SNDRV_CTL_ELEM_TYPE_BYTES)
		return -EIO;
	map->controls[0].id_child.numid = info2.id.numid;
	map->type = info2.type;
	access = info2.access;
	owner = info2.owner;
	count = map->controls[0].channel_map_items;
	for (item = 1; item < map->controls_items; item++) {
		snd_ctl_elem_info_clear(&info3);
		info3.id = map->controls[item].id_child;
		debug_id(&info3.id, "%s controls[%zd]\n", __func__, item);
		err = snd_ctl_elem_info(priv->child, &info3);
		if (err < 0)
			return err;
		if (info2.type != info3.type)
			return -EIO;
		if (ACCESS_BITS(info2.access) != ACCESS_BITS(info3.access))
			return -EIO;
		if (info2.type == SNDRV_CTL_ELEM_TYPE_BOOLEAN ||
		    info2.type == SNDRV_CTL_ELEM_TYPE_INTEGER) {
			if (memcmp(&info2.value.integer, &info3.value.integer, sizeof(info2.value.integer)))
				return -EIO;
		} else if (info2.type == SNDRV_CTL_ELEM_TYPE_INTEGER64) {
			if (memcmp(&info2.value.integer64, &info3.value.integer64, sizeof(info2.value.integer64)))
				return -EIO;
		}
		access |= info3.access;
		if (owner == 0)
			owner = info3.owner;
		if (count < map->controls[item].channel_map_items)
			count = map->controls[item].channel_map_items;
	}
	snd_ctl_elem_info_clear(info);
	info->id = map->map_id;
	info->type = info2.type;
	info->access = access;
	info->count = count;
	if (info2.type == SNDRV_CTL_ELEM_TYPE_BOOLEAN ||
	    info2.type == SNDRV_CTL_ELEM_TYPE_INTEGER)
		info->value.integer = info2.value.integer;
	else if (info2.type == SNDRV_CTL_ELEM_TYPE_INTEGER64)
		info->value.integer64 = info2.value.integer64;
	if (access & SNDRV_CTL_ELEM_ACCESS_LOCK)
		info->owner = owner;
	return 0;
}

static int remap_sync_elem_info(snd_ctl_remap_t *priv, snd_ctl_elem_info_t *info)
{
	snd_ctl_sync_t *sync;

	sync = remap_find_sync_switch_id(priv, &info->id);
	if (!sync)
		return -EREMAPNOTFOUND;
	snd_ctl_elem_info_clear(info);
	info->id = sync->switch_id;
	info->type = SNDRV_CTL_ELEM_TYPE_BOOLEAN;
	info->access = SNDRV_CTL_ELEM_ACCESS_READ | SNDRV_CTL_ELEM_ACCESS_WRITE;
	info->count = 1;
	return 0;
}

static int snd_ctl_remap_elem_info(snd_ctl_t *ctl, snd_ctl_elem_info_t *info)
{
	snd_ctl_remap_t *priv = ctl->private_data;
	snd_ctl_remap_id_t *rid;
	int err;

	debug_id(&info->id, "%s\n", __func__);
	err = remap_map_elem_info(priv, info);
	if (err != -EREMAPNOTFOUND)
		return err;
	err = remap_sync_elem_info(priv, info);
	if (err != -EREMAPNOTFOUND)
		return err;
	err = remap_id_to_child(priv, &info->id, &rid);
	if (err < 0)
		return err;
	err = snd_ctl_elem_info(priv->child, info);
	if (err >= 0 && priv->sync_items > 0)
		remap_update_sync_id(priv, &info->id);
	return remap_id_to_app(priv, &info->id, rid, err);
}

static int remap_map_elem_read(snd_ctl_remap_t *priv, snd_ctl_elem_value_t *control)
{
	snd_ctl_map_t *map;
	struct snd_ctl_map_ctl *mctl;
	snd_ctl_elem_value_t control2;
	size_t item, index, src_index;
	int err;

	map = remap_find_map_id(priv, &control->id);
	if (map == NULL)
		return -EREMAPNOTFOUND;
	debug_id(&control->id, "%s\n", __func__);
	snd_ctl_elem_value_clear(control);
	control->id = map->map_id;
	for (item = 0; item < map->controls_items; item++) {
		mctl = &map->controls[item];
		snd_ctl_elem_value_clear(&control2);
		control2.id = mctl->id_child;
		debug_id(&control2.id, "%s controls[%zd]\n", __func__, item);
		err = snd_ctl_elem_read(priv->child, &control2);
		if (err < 0)
			return err;
		if (map->type == SNDRV_CTL_ELEM_TYPE_BOOLEAN ||
		    map->type == SNDRV_CTL_ELEM_TYPE_INTEGER) {
			for (index = 0; index < mctl->channel_map_items; index++) {
				long base_idx = mctl->src_channels * index;
				long src = mctl->channel_map[base_idx];
				if ((unsigned long)src < ARRAY_SIZE(control->value.integer.value))
					control->value.integer.value[index] = control2.value.integer.value[src];
				for (src_index = 1; src_index < mctl->src_channels; src_index++) {
					src = mctl->channel_map[base_idx + src_index];
					if ((unsigned long)src < ARRAY_SIZE(control->value.integer.value))
						if (control2.value.integer.value[src] < control->value.integer.value[index])
							control->value.integer.value[index] = control2.value.integer.value[src];
				}
			}
		} else if (map->type == SNDRV_CTL_ELEM_TYPE_INTEGER64) {
			for (index = 0; index < mctl->channel_map_items; index++) {
				long base_idx = mctl->src_channels * index;
				long src = mctl->channel_map[base_idx];
				if ((unsigned long)src < ARRAY_SIZE(control->value.integer64.value))
					control->value.integer64.value[index] = control2.value.integer64.value[src];
				for (src_index = 1; src_index < mctl->src_channels; src_index++) {
					src = mctl->channel_map[base_idx + src_index];
					if ((unsigned long)src < ARRAY_SIZE(control->value.integer64.value))
						if (control2.value.integer64.value[src] < control->value.integer64.value[index])
							control->value.integer64.value[index] = control2.value.integer64.value[src];
				}
			}
		} else if (map->type == SNDRV_CTL_ELEM_TYPE_BYTES) {
			/* does it make sense to merge bytes? */
			if (mctl->src_channels > 1)
				return -EINVAL;
			for (index = 0; index < mctl->channel_map_items; index++) {
				long src = mctl->channel_map[mctl->src_channels];
				if ((unsigned long)src < ARRAY_SIZE(control->value.bytes.data))
					control->value.bytes.data[index] = control2.value.bytes.data[src];
			}
		}
	}
	return 0;
}

static int remap_sync_elem_read(snd_ctl_remap_t *priv, snd_ctl_elem_value_t *control)
{
	snd_ctl_sync_t *sync;

	sync = remap_find_sync_switch_id(priv, &control->id);
	if (!sync)
		return -EREMAPNOTFOUND;
	control->value.integer.value[0] = sync->switch_state ? 1 : 0;
	return 0;
}

static int snd_ctl_remap_elem_read(snd_ctl_t *ctl, snd_ctl_elem_value_t *control)
{
	snd_ctl_remap_t *priv = ctl->private_data;
	snd_ctl_remap_id_t *rid;
	int err;

	debug_id(&control->id, "%s\n", __func__);
	err = remap_map_elem_read(priv, control);
	if (err != -EREMAPNOTFOUND)
		return err;
	err = remap_sync_elem_read(priv, control);
	if (err != -EREMAPNOTFOUND)
		return err;
	err = remap_id_to_child(priv, &control->id, &rid);
	if (err < 0)
		return err;
	err = snd_ctl_elem_read(priv->child, control);
	return remap_id_to_app(priv, &control->id, rid, err);
}

static int remap_map_elem_write(snd_ctl_remap_t *priv, snd_ctl_elem_value_t *control)
{
	snd_ctl_map_t *map;
	struct snd_ctl_map_ctl *mctl;
	snd_ctl_elem_value_t control2;
	size_t item, index, src_index;
	int err, changes;

	map = remap_find_map_id(priv, &control->id);
	if (map == NULL)
		return -EREMAPNOTFOUND;
	debug_id(&control->id, "%s\n", __func__);
	control->id = map->map_id;
	for (item = 0; item < map->controls_items; item++) {
		mctl = &map->controls[item];
		snd_ctl_elem_value_clear(&control2);
		control2.id = mctl->id_child;
		debug_id(&control2.id, "%s controls[%zd]\n", __func__, item);
		err = snd_ctl_elem_read(priv->child, &control2);
		if (err < 0)
			return err;
		changes = 0;
		if (map->type == SNDRV_CTL_ELEM_TYPE_BOOLEAN ||
		    map->type == SNDRV_CTL_ELEM_TYPE_INTEGER) {
			for (index = 0; index < mctl->channel_map_items; index++) {
				long base_idx = mctl->src_channels * index;
				long dst = mctl->channel_map[base_idx];
				if ((unsigned long)dst < ARRAY_SIZE(control->value.integer.value)) {
					changes |= control2.value.integer.value[dst] != control->value.integer.value[index];
					control2.value.integer.value[dst] = control->value.integer.value[index];
				}
				for (src_index = 1; src_index < mctl->src_channels; src_index++) {
					dst = mctl->channel_map[base_idx + src_index];
					if ((unsigned long)dst < ARRAY_SIZE(control->value.integer.value)) {
						changes |= control2.value.integer.value[dst] != control->value.integer.value[index];
						control2.value.integer.value[dst] = control->value.integer.value[index];
					}
				}
			}
		} else if (map->type == SNDRV_CTL_ELEM_TYPE_INTEGER64) {
			for (index = 0; index < mctl->channel_map_items; index++) {
				long base_idx = mctl->src_channels * index;
				long dst = mctl->channel_map[base_idx];
				if ((unsigned long)dst < ARRAY_SIZE(control->value.integer64.value)) {
					changes |= control2.value.integer64.value[dst] != control->value.integer64.value[index];
					control2.value.integer64.value[dst] = control->value.integer64.value[index];
				}
				for (src_index = 1; src_index < mctl->src_channels; src_index++) {
					dst = mctl->channel_map[base_idx + src_index];
					if ((unsigned long)dst < ARRAY_SIZE(control->value.integer.value)) {
						changes |= control2.value.integer64.value[dst] != control->value.integer64.value[index];
						control2.value.integer64.value[dst] = control->value.integer64.value[index];
					}
				}
			}
		} else if (map->type == SNDRV_CTL_ELEM_TYPE_BYTES) {
			/* does it make sense to merge bytes? */
			if (mctl->src_channels > 1)
				return -EINVAL;
			for (index = 0; index < mctl->channel_map_items; index++) {
				long dst = mctl->channel_map[index];
				if ((unsigned long)dst < ARRAY_SIZE(control->value.bytes.data)) {
					changes |= control2.value.bytes.data[dst] != control->value.bytes.data[index];
					control2.value.bytes.data[dst] = control->value.bytes.data[index];
				}
			}
		}
		debug_id(&control2.id, "%s changes %d\n", __func__, changes);
		if (changes > 0) {
			err = snd_ctl_elem_write(priv->child, &control2);
			if (err < 0)
				return err;
		}
	}
	return 0;
}

static int remap_sync_elem_write(snd_ctl_remap_t *priv, snd_ctl_elem_value_t *control)
{
	snd_ctl_sync_t *sync;
	snd_ctl_elem_value_t control2;
	size_t item;
	int err;

	sync = remap_find_sync_switch_id(priv, &control->id);
	if (sync) {
		err = sync->switch_state != control->value.integer.value[0];
		sync->switch_state = control->value.integer.value[0] != 0;
		return err;
	}
	sync = remap_find_sync_id(priv, &control->id);
	if (sync == NULL)
		return -EREMAPNOTFOUND;
	if (sync->switch_state == false)
		return -EREMAPNOTFOUND;
	debug_id(&control->id, "%s\n", __func__);
	control2 = *control;
	for (item = 0; item < sync->control_items; item++) {
		control2.id = sync->control_ids[item];
		debug_id(&control2.id, "%s sync[%zd]\n", __func__, item);
		/* TODO: it's a blind write - no checks if the values are in range for all controls */
		err = snd_ctl_elem_write(priv->child, &control2);
		if (err < 0)
			return err;
	}
	return remap_id_to_app(priv, &control->id, NULL, 0);
}

static int snd_ctl_remap_elem_write(snd_ctl_t *ctl, snd_ctl_elem_value_t *control)
{
	snd_ctl_remap_t *priv = ctl->private_data;
	snd_ctl_remap_id_t *rid;
	int err;

	debug_id(&control->id, "%s\n", __func__);
	err = remap_map_elem_write(priv, control);
	if (err != -EREMAPNOTFOUND)
		return err;
	err = remap_sync_elem_write(priv, control);
	if (err != -EREMAPNOTFOUND)
		return err;
	err = remap_id_to_child(priv, &control->id, &rid);
	if (err < 0)
		return err;
	err = snd_ctl_elem_write(priv->child, control);
	return remap_id_to_app(priv, &control->id, rid, err);
}

static int snd_ctl_remap_elem_lock(snd_ctl_t *ctl, snd_ctl_elem_id_t *id)
{
	snd_ctl_remap_t *priv = ctl->private_data;
	snd_ctl_remap_id_t *rid;
	int err;

	debug_id(id, "%s\n", __func__);
	err = remap_id_to_child(priv, id, &rid);
	if (err < 0)
		return err;
	err = snd_ctl_elem_lock(priv->child, id);
	return remap_id_to_app(priv, id, rid, err);
}

static int snd_ctl_remap_elem_unlock(snd_ctl_t *ctl, snd_ctl_elem_id_t *id)
{
	snd_ctl_remap_t *priv = ctl->private_data;
	snd_ctl_remap_id_t *rid;
	int err;

	debug_id(id, "%s\n", __func__);
	err = remap_id_to_child(priv, id, &rid);
	if (err < 0)
		return err;
	err = snd_ctl_elem_unlock(priv->child, id);
	return remap_id_to_app(priv, id, rid, err);
}

static int remap_get_map_numid(snd_ctl_remap_t *priv, struct snd_ctl_map_ctl *mctl)
{
	snd_ctl_elem_info_t info;
	snd_ctl_numid_t *numid;
	int err;

	if (mctl->id_child.numid > 0)
		return 0;
	debug_id(&mctl->id_child, "%s get numid\n", __func__);
	snd_ctl_elem_info_clear(&info);
	info.id = mctl->id_child;
	err = snd_ctl_elem_info(priv->child, &info);
	if (err < 0)
		return err;
	numid = remap_find_numid_child(priv, info.id.numid);
	if (numid == NULL)
		return -EIO;
	mctl->id_child.numid = info.id.numid;
	return 0;
}

static int remap_map_elem_tlv(snd_ctl_remap_t *priv, int op_flag, unsigned int numid,
			      unsigned int *tlv, unsigned int tlv_size)
{
	snd_ctl_map_t *map;
	struct snd_ctl_map_ctl *mctl;
	size_t item;
	unsigned int *tlv2;
	int err;

	map = remap_find_map_numid(priv, numid);
	if (map == NULL)
		return -EREMAPNOTFOUND;
	if (op_flag != 0)	/* read only */
		return -ENXIO;
	debug("%s numid %d\n", __func__, numid);
	mctl = &map->controls[0];
	err = remap_get_map_numid(priv, mctl);
	if (err < 0)
		return err;
	memset(tlv, 0, tlv_size);
	err = priv->child->ops->element_tlv(priv->child, op_flag, mctl->id_child.numid, tlv, tlv_size);
	if (err < 0)
		return err;
	tlv2 = malloc(tlv_size);
	if (tlv2 == NULL)
		return -ENOMEM;
	for (item = 1; item < map->controls_items; item++) {
		mctl = &map->controls[item];
		err = remap_get_map_numid(priv, mctl);
		if (err < 0) {
			free(tlv2);
			return err;
		}
		memset(tlv2, 0, tlv_size);
		err = priv->child->ops->element_tlv(priv->child, op_flag, mctl->id_child.numid, tlv2, tlv_size);
		if (err < 0) {
			free(tlv2);
			return err;
		}
		if (memcmp(tlv, tlv2, tlv_size) != 0) {
			free(tlv2);
			return -EIO;
		}
	}
	free(tlv2);
	return 0;
}

static int snd_ctl_remap_elem_tlv(snd_ctl_t *ctl, int op_flag,
				  unsigned int numid,
				  unsigned int *tlv, unsigned int tlv_size)
{
	snd_ctl_remap_t *priv = ctl->private_data;
	snd_ctl_numid_t *map_numid;
	int err;

	debug("%s: numid = %d, op_flag = %d\n", __func__, numid, op_flag);
	err = remap_map_elem_tlv(priv, op_flag, numid, tlv, tlv_size);
	if (err != -EREMAPNOTFOUND)
		return err;
	map_numid = remap_find_numid_app(priv, numid);
	if (map_numid == NULL)
		return -ENOENT;
	return priv->child->ops->element_tlv(priv->child, op_flag, map_numid->numid_child, tlv, tlv_size);
}

static int snd_ctl_remap_hwdep_next_device(snd_ctl_t *ctl, int * device)
{
	snd_ctl_remap_t *priv = ctl->private_data;
	return snd_ctl_hwdep_next_device(priv->child, device);
}

static int snd_ctl_remap_hwdep_info(snd_ctl_t *ctl, snd_hwdep_info_t * info)
{
	snd_ctl_remap_t *priv = ctl->private_data;
	return snd_ctl_hwdep_info(priv->child, info);
}

static int snd_ctl_remap_pcm_next_device(snd_ctl_t *ctl, int * device)
{
	snd_ctl_remap_t *priv = ctl->private_data;
	return snd_ctl_pcm_next_device(priv->child, device);
}

static int snd_ctl_remap_pcm_info(snd_ctl_t *ctl, snd_pcm_info_t * info)
{
	snd_ctl_remap_t *priv = ctl->private_data;
	return snd_ctl_pcm_info(priv->child, info);
}

static int snd_ctl_remap_pcm_prefer_subdevice(snd_ctl_t *ctl, int subdev)
{
	snd_ctl_remap_t *priv = ctl->private_data;
	return snd_ctl_pcm_prefer_subdevice(priv->child, subdev);
}

static int snd_ctl_remap_rawmidi_next_device(snd_ctl_t *ctl, int * device)
{
	snd_ctl_remap_t *priv = ctl->private_data;
	return snd_ctl_rawmidi_next_device(priv->child, device);
}

static int snd_ctl_remap_rawmidi_info(snd_ctl_t *ctl, snd_rawmidi_info_t * info)
{
	snd_ctl_remap_t *priv = ctl->private_data;
	return snd_ctl_rawmidi_info(priv->child, info);
}

static int snd_ctl_remap_rawmidi_prefer_subdevice(snd_ctl_t *ctl, int subdev)
{
	snd_ctl_remap_t *priv = ctl->private_data;
	return snd_ctl_rawmidi_prefer_subdevice(priv->child, subdev);
}

static int snd_ctl_remap_set_power_state(snd_ctl_t *ctl, unsigned int state)
{
	snd_ctl_remap_t *priv = ctl->private_data;
	return snd_ctl_set_power_state(priv->child, state);
}

static int snd_ctl_remap_get_power_state(snd_ctl_t *ctl, unsigned int *state)
{
	snd_ctl_remap_t *priv = ctl->private_data;
	return snd_ctl_get_power_state(priv->child, state);
}

static void _next_ptr(size_t *ptr, size_t count)
{
	*ptr = (*ptr + 1) % count;
}

static void event_add(snd_ctl_remap_t *priv, snd_ctl_elem_id_t *id,
		      unsigned int numid_app, unsigned int event_mask)
{
	snd_ctl_map_event_t *map_event;
	int found = 0;
	size_t head;

	for (head = priv->event_queue_head;
	     head != priv->event_queue_tail;
	     _next_ptr(&head, priv->event_items))
		if (priv->event_queue[head].numid_app == numid_app) {
			found = 1;
			priv->event_queue[head].event_mask |= event_mask;
			break;
		}
	debug_id(id, "%s marking for read (already %d)\n", __func__, found);
	if (found)
		return;
	map_event = &priv->event_queue[priv->event_queue_tail];
	map_event->id = id;
	map_event->numid_app = numid_app;
	map_event->event_mask = event_mask;
	_next_ptr(&priv->event_queue_tail, priv->event_items);
}

static void remap_event_for_all_map_controls(snd_ctl_remap_t *priv,
					     snd_ctl_elem_id_t *id,
					     unsigned int event_mask)
{
	size_t count, index;
	snd_ctl_map_t *map;
	struct snd_ctl_map_ctl *mctl;
	int changed = 0;

	if (event_mask == SNDRV_CTL_EVENT_MASK_REMOVE)
		event_mask = SNDRV_CTL_EVENT_MASK_INFO;
	map = priv->map;
	for (count = priv->map_items; count > 0; count--, map++) {
		changed = 0;
		for (index = 0; index < map->controls_items; index++) {
			mctl = &map->controls[index];
			if (mctl->id_child.numid == 0) {
				if (snd_ctl_elem_id_compare_set(id, &mctl->id_child))
					continue;
				mctl->id_child.numid = id->numid;
			}
			if (id->numid != mctl->id_child.numid)
				continue;
			debug_id(&map->map_id, "%s found (all)\n", __func__);
			changed = 1;
		}
		if (changed)
			event_add(priv, &map->map_id, map->map_id.numid, event_mask);
	}
}

static void remap_event_for_all_sync_controls(snd_ctl_remap_t *priv,
					      snd_ctl_elem_id_t *id,
					      unsigned int event_mask)
{
	size_t count, index;
	snd_ctl_sync_t *sync;
	snd_ctl_numid_t *numid;
	snd_ctl_elem_id_t *sid;
	int changed = 0;

	if (event_mask == SNDRV_CTL_EVENT_MASK_REMOVE)
		return;
	sync = priv->sync;
	for (count = priv->sync_items; count > 0; count--, sync++) {
		changed = 0;
		for (index = 0; index < sync->control_items; index++) {
			sid = &sync->control_ids[index];
			if (sid->numid == 0) {
				if (snd_ctl_elem_id_compare_set(id, sid))
					continue;
				sid->numid = id->numid;
			}
			if (id->numid != sid->numid)
				continue;
			debug_id(sid, "%s found (all)\n", __func__);
			changed = 1;
			break;
		}
		if (!changed)
			continue;
		for (index = 0; index < sync->control_items; index++) {
			sid = &sync->control_ids[index];
			/* skip double updates */
			if (sid->numid == id->numid)
				continue;
			numid = remap_find_numid_child(priv, sid->numid);
			if (numid)
				event_add(priv, sid, numid->numid_app, event_mask);
		}
	}
}

static int snd_ctl_remap_read(snd_ctl_t *ctl, snd_ctl_event_t *event)
{
	snd_ctl_remap_t *priv = ctl->private_data;
	snd_ctl_remap_id_t *rid;
	snd_ctl_numid_t *numid;
	snd_ctl_map_event_t *map_event;
	unsigned int numid_child;
	int err;

	if (priv->event_queue_head != priv->event_queue_tail) {
		map_event = &priv->event_queue[priv->event_queue_head];
		_next_ptr(&priv->event_queue_head, priv->event_items);
		memset(event, 0, sizeof(*event));
		event->type = SNDRV_CTL_EVENT_ELEM;
		event->data.elem.mask = map_event->event_mask;
		event->data.elem.id = *map_event->id;
		event->data.elem.id.numid = map_event->numid_app;
		debug_id(&event->data.elem.id, "%s queue read\n", __func__);
		return 1;
	}
	err = snd_ctl_read(priv->child, event);
	if (err < 0 || event->type != SNDRV_CTL_EVENT_ELEM)
		return err;
	if (event->data.elem.mask == SNDRV_CTL_EVENT_MASK_REMOVE ||
	    (event->data.elem.mask & (SNDRV_CTL_EVENT_MASK_VALUE | SNDRV_CTL_EVENT_MASK_INFO |
				      SNDRV_CTL_EVENT_MASK_ADD | SNDRV_CTL_EVENT_MASK_TLV)) != 0) {
		debug_id(&event->data.elem.id, "%s event mask 0x%x\n", __func__, event->data.elem.mask);
		numid_child = event->data.elem.id.numid;
		remap_event_for_all_map_controls(priv, &event->data.elem.id, event->data.elem.mask);
		remap_event_for_all_sync_controls(priv, &event->data.elem.id, event->data.elem.mask);
		rid = remap_find_id_child(priv, &event->data.elem.id);
		if (rid) {
			if (rid->id_child.numid == 0) {
				numid = remap_find_numid_child(priv, numid_child);
				if (numid == NULL)
					return -EIO;
				rid->id_child.numid = numid->numid_child;
				rid->id_app.numid = numid->numid_app;
			}
			event->data.elem.id = rid->id_app;
		} else {
			numid = remap_find_numid_child(priv, numid_child);
			if (numid == NULL)
				return -EIO;
			event->data.elem.id.numid = numid->numid_app;
		}
		if (event->data.elem.mask == SNDRV_CTL_EVENT_MASK_REMOVE)
			remap_forget_numid_child(priv, numid_child);
	}
	return err;
}

static const snd_ctl_ops_t snd_ctl_remap_ops = {
	.close = snd_ctl_remap_close,
	.nonblock = snd_ctl_remap_nonblock,
	.async = snd_ctl_remap_async,
	.subscribe_events = snd_ctl_remap_subscribe_events,
	.card_info = snd_ctl_remap_card_info,
	.element_list = snd_ctl_remap_elem_list,
	.element_info = snd_ctl_remap_elem_info,
	.element_read = snd_ctl_remap_elem_read,
	.element_write = snd_ctl_remap_elem_write,
	.element_lock = snd_ctl_remap_elem_lock,
	.element_unlock = snd_ctl_remap_elem_unlock,
	.element_tlv = snd_ctl_remap_elem_tlv,
	.hwdep_next_device = snd_ctl_remap_hwdep_next_device,
	.hwdep_info = snd_ctl_remap_hwdep_info,
	.pcm_next_device = snd_ctl_remap_pcm_next_device,
	.pcm_info = snd_ctl_remap_pcm_info,
	.pcm_prefer_subdevice = snd_ctl_remap_pcm_prefer_subdevice,
	.rawmidi_next_device = snd_ctl_remap_rawmidi_next_device,
	.rawmidi_info = snd_ctl_remap_rawmidi_info,
	.rawmidi_prefer_subdevice = snd_ctl_remap_rawmidi_prefer_subdevice,
	.set_power_state = snd_ctl_remap_set_power_state,
	.get_power_state = snd_ctl_remap_get_power_state,
	.read = snd_ctl_remap_read,
};

static int add_to_remap(snd_ctl_remap_t *priv,
			snd_ctl_elem_id_t *child,
			snd_ctl_elem_id_t *app)
{
	snd_ctl_remap_id_t *rid;


	if (priv->remap_alloc == priv->remap_items) {
		rid = realloc(priv->remap, (priv->remap_alloc + 16) * sizeof(*rid));
		if (rid == NULL)
			return -ENOMEM;
		memset(rid + priv->remap_alloc, 0, sizeof(*rid) * 16);
		priv->remap_alloc += 16;
		priv->remap = rid;
	}
	rid = &priv->remap[priv->remap_items++];
	rid->id_child = *child;
	rid->id_app = *app;
	debug_id(&rid->id_child, "%s remap child\n", __func__);
	debug_id(&rid->id_app, "%s remap app\n", __func__);
	return 0;
}

static int parse_remap(snd_ctl_remap_t *priv, snd_config_t *conf)
{
	snd_config_iterator_t i, next;
	snd_ctl_elem_id_t child, app;
	int err;

	if (conf == NULL)
		return 0;
	snd_config_for_each(i, next, conf) {
		snd_config_t *n = snd_config_iterator_entry(i);
		const char *id, *str;
		if (snd_config_get_id(n, &id) < 0)
			continue;
		if (snd_config_get_string(n, &str) < 0) {
			SNDERR("expected string with the target control id!");
			return -EINVAL;
		}
		snd_ctl_elem_id_clear(&app);
		err = snd_ctl_ascii_elem_id_parse(&app, str);
		if (err < 0) {
			SNDERR("unable to parse target id '%s'!", str);
			return -EINVAL;
		}
		if (remap_find_id_app(priv, &app)) {
			SNDERR("duplicate target id '%s'!", id);
			return -EINVAL;
		}
		snd_ctl_elem_id_clear(&child);
		err = snd_ctl_ascii_elem_id_parse(&child, id);
		if (err < 0) {
			SNDERR("unable to parse source id '%s'!", id);
			return -EINVAL;
		}
		if (remap_find_id_child(priv, &app)) {
			SNDERR("duplicate source id '%s'!", id);
			return -EINVAL;
		}
		err = add_to_remap(priv, &child, &app);
		if (err < 0)
			return err;
	}

	return 0;
}

static int new_map(snd_ctl_remap_t *priv, snd_ctl_map_t **_map, snd_ctl_elem_id_t *id)
{
	snd_ctl_map_t *map;
	snd_ctl_numid_t *numid;

	if (priv->map_alloc == priv->map_items) {
		map = realloc(priv->map, (priv->map_alloc + 16) * sizeof(*map));
		if (map == NULL)
			return -ENOMEM;
		memset(map + priv->map_alloc, 0, sizeof(*map) * 16);
		priv->map_alloc += 16;
		priv->map = map;
	}
	map = &priv->map[priv->map_items++];
	map->map_id = *id;
	numid = remap_numid_new(priv, 0, ++priv->numid_app_last);
	if (numid == NULL)
		return -ENOMEM;
	map->map_id.numid = numid->numid_app;
	debug_id(&map->map_id, "%s created\n", __func__);
	*_map = map;
	return 0;
}

static int add_ctl_to_map(snd_ctl_map_t *map, struct snd_ctl_map_ctl **_mctl, snd_ctl_elem_id_t *id)
{
	struct snd_ctl_map_ctl *mctl;

	if (map->controls_alloc == map->controls_items) {
		mctl = realloc(map->controls, (map->controls_alloc + 4) * sizeof(*mctl));
		if (mctl == NULL)
			return -ENOMEM;
		memset(mctl + map->controls_alloc, 0, sizeof(*mctl) * 4);
		map->controls_alloc += 4;
		map->controls = mctl;
	}
	mctl = &map->controls[map->controls_items++];
	mctl->id_child = *id;
	*_mctl = mctl;
	return 0;
}

static int add_chn_to_map(struct snd_ctl_map_ctl *mctl, long idx, long src_idx, long val)
{
	size_t off;
	long *map;

	if (src_idx >= mctl->src_channels) {
		SNDERR("Wrong channel mapping (extra source channel?)");
		return -EINVAL;
	}
	if (mctl->channel_map_alloc <= (size_t)idx) {
		map = realloc(mctl->channel_map, mctl->src_channels * (idx + 4) * sizeof(*map));
		if (map == NULL)
			return -ENOMEM;
		mctl->channel_map = map;
		off = mctl->channel_map_alloc;
		mctl->channel_map_alloc = idx + 4;
		for ( ; off < mctl->src_channels * mctl->channel_map_alloc; off++)
			map[off] = -1;
	}
	if ((size_t)idx >= mctl->channel_map_items)
		mctl->channel_map_items = idx + 1;
	mctl->channel_map[mctl->src_channels * idx + src_idx] = val;
	return 0;
}

static int add_chn_to_map_array(struct snd_ctl_map_ctl *mctl, const char *dst_id, snd_config_t *conf)
{
	snd_config_iterator_t i, next;
	long src_idx = 0;
	int err;

	snd_config_for_each(i, next, conf) {
		snd_config_t *n = snd_config_iterator_entry(i);
		long idx = -1, chn = -1;
		if (safe_strtol(dst_id, &idx) || snd_config_get_integer(n, &chn)) {
			SNDERR("Wrong channel mapping (%ld -> %ld)", idx, chn);
			return -EINVAL;
		}
		err = add_chn_to_map(mctl, idx, src_idx, chn);
		if (err < 0)
			return err;
		src_idx++;
	}
	return 0;
}

static int parse_map_vindex(struct snd_ctl_map_ctl *mctl, snd_config_t *conf)
{
	snd_config_iterator_t i, next;
	snd_config_t *n;
	int err;

	snd_config_for_each(i, next, conf) {
		n = snd_config_iterator_entry(i);
		err = snd_config_is_array(n);
		if (err > 0 && (unsigned int)err > mctl->src_channels)
			mctl->src_channels = err; /* count of array items */
	}
	snd_config_for_each(i, next, conf) {
		n = snd_config_iterator_entry(i);
		long idx = -1, chn = -1;
		const char *id;
		if (snd_config_get_id(n, &id) < 0)
			continue;
		if (snd_config_is_array(n) > 0) {
			err = add_chn_to_map_array(mctl, id, n);
		} else {
			if (safe_strtol(id, &idx) || snd_config_get_integer(n, &chn)) {
				SNDERR("Wrong channel mapping (%ld -> %ld)", idx, chn);
				return -EINVAL;
			}
			err = add_chn_to_map(mctl, idx, 0, chn);
		}
		if (err < 0)
			return err;
	}

	return 0;
}

static int parse_map_config(struct snd_ctl_map_ctl *mctl, snd_config_t *conf)
{
	snd_config_iterator_t i, next;
	int err;

	mctl->src_channels = 1;
	snd_config_for_each(i, next, conf) {
		snd_config_t *n = snd_config_iterator_entry(i);
		const char *id;
		if (snd_config_get_id(n, &id) < 0)
			continue;
		if (strcmp(id, "vindex") == 0) {
			err = parse_map_vindex(mctl, n);
			if (err < 0)
				return err;
		}
	}
	return 0;
}

static int parse_map1(snd_ctl_map_t *map, snd_config_t *conf)
{
	snd_config_iterator_t i, next;
	snd_ctl_elem_id_t cid;
	struct snd_ctl_map_ctl *mctl;
	int err;

	snd_config_for_each(i, next, conf) {
		snd_config_t *n = snd_config_iterator_entry(i);
		const char *id;
		if (snd_config_get_id(n, &id) < 0)
			continue;
		snd_ctl_elem_id_clear(&cid);
		err = snd_ctl_ascii_elem_id_parse(&cid, id);
		if (err < 0) {
			SNDERR("unable to parse control id '%s'!", id);
			return -EINVAL;
		}
		err = add_ctl_to_map(map, &mctl, &cid);
		if (err < 0)
			return err;
		err = parse_map_config(mctl, n);
		if (err < 0)
			return err;
	}

	return 0;
}

static int parse_map(snd_ctl_remap_t *priv, snd_config_t *conf)
{
	snd_config_iterator_t i, next;
	snd_ctl_elem_id_t eid;
	snd_ctl_map_t *map;
	int err;

	if (conf == NULL)
		return 0;
	snd_config_for_each(i, next, conf) {
		snd_config_t *n = snd_config_iterator_entry(i);
		const char *id;
		if (snd_config_get_id(n, &id) < 0)
			continue;
		snd_ctl_elem_id_clear(&eid);
		err = snd_ctl_ascii_elem_id_parse(&eid, id);
		if (err < 0) {
			SNDERR("unable to parse id '%s'!", id);
			return -EINVAL;
		}
		err = new_map(priv, &map, &eid);
		if (err < 0)
			return 0;
		err = parse_map1(map, n);
		if (err < 0)
			return err;
	}

	return 0;
}

static snd_ctl_sync_t *alloc_sync(snd_ctl_remap_t *priv)
{
	snd_ctl_sync_t *sync;

	if (priv->sync_alloc == priv->sync_items) {
		sync = realloc(priv->sync, (priv->sync_alloc + 16) * sizeof(*sync));
		if (sync == NULL)
			return NULL;
		memset(sync + priv->sync_alloc, 0, sizeof(*sync) * 16);
		priv->sync_alloc += 16;
		priv->sync = sync;
	}
	return &priv->sync[priv->sync_items++];
}

static int parse_sync1(snd_ctl_remap_t *priv, unsigned int count, snd_config_t *conf)
{
	snd_config_iterator_t i, next;
	snd_ctl_elem_id_t *eid;
	snd_ctl_sync_t *sync;
	const char *str;
	int err, index = 0;

	sync = alloc_sync(priv);
	if (sync == NULL)
		return -ENOMEM;
	sync->switch_state = true;
	sync->control_ids = calloc(count, sizeof(sync->control_ids[0]));
	if (sync->control_ids == NULL)
		return -ENOMEM;
	snd_config_for_each(i, next, conf) {
		snd_config_t *n = snd_config_iterator_entry(i);
		if (snd_config_get_string(n, &str) < 0) {
			SNDERR("strings are expected in sync array");
			return -EINVAL;
		}
		eid = &sync->control_ids[index];
		snd_ctl_elem_id_clear(eid);
		err = snd_ctl_ascii_elem_id_parse(eid, str);
		if (err < 0) {
			SNDERR("unable to parse control id '%s'!", str);
			return -EINVAL;
		}
		sync->control_items++;
		index++;
	}

	return 0;
}

static int parse_sync_compound(snd_ctl_remap_t *priv, snd_config_t *conf)
{
	snd_config_iterator_t i, next;
	snd_ctl_elem_id_t eid;
	snd_ctl_numid_t *numid;
	bool eid_found = false;
	bool controls_found = false;
	int count, err;

	snd_ctl_elem_id_clear(&eid);
	snd_config_for_each(i, next, conf) {
		snd_config_t *n = snd_config_iterator_entry(i);
		const char *id, *str;
		if (snd_config_get_id(n, &id) < 0)
			continue;
		if (strcmp(id, "switch") == 0) {
			if (snd_config_get_string(n, &str) < 0) {
				SNDERR("String is expected for switch");
				return -EINVAL;
			}
			err = snd_ctl_ascii_elem_id_parse(&eid, str);
			if (err < 0) {
				SNDERR("unable to parse id '%s'!", str);
				return -EINVAL;
			}
			eid_found = true;
		}
		if (strcmp(id, "controls") == 0) {
			count = snd_config_is_array(n);
			if (count <= 0) {
				SNDERR("Array is expected for sync!");
				return -EINVAL;
			}
			err = parse_sync1(priv, count, n);
			if (err < 0)
				return err;
			controls_found = true;
		}
	}

	if (!controls_found || !eid_found)
		return 0;

	numid = remap_numid_new(priv, 0, ++priv->numid_app_last);
	if (numid == NULL)
		return -ENOMEM;
	eid.numid = numid->numid_app;
	priv->sync[priv->sync_items - 1].switch_id = eid;
	priv->sync_switch_items++;
	return 0;
}

static int parse_sync(snd_ctl_remap_t *priv, snd_config_t *conf)
{
	snd_config_iterator_t i, next;
	int count, err;

	if (conf == NULL)
		return 0;
	snd_config_for_each(i, next, conf) {
		snd_config_t *n = snd_config_iterator_entry(i);
		if (snd_config_get_type(n) == SND_CONFIG_TYPE_COMPOUND) {
			err = parse_sync_compound(priv, n);
		} else {
			count = snd_config_is_array(n);
			if (count <= 0) {
				SNDERR("Array is expected for sync!");
				return -EINVAL;
			}
			err = parse_sync1(priv, count, n);
		}
		if (err < 0)
			return err;
	}

	return 0;
}

/**
 * \brief Creates a new remap/map/sync control handle
 * \param handlep Returns created control handle
 * \param name Name of control device
 * \param remap Remap configuration
 * \param map Map configuration
 * \param sync Sync configuration
 * \param child child configuration root
 * \param mode Control handle mode
 * \retval zero on success otherwise a negative error code
 * \warning Using of this function might be dangerous in the sense
 *          of compatibility reasons. The prototype might be freely
 *          changed in future.
 */
int snd_ctl_remap_open(snd_ctl_t **handlep, const char *name, snd_config_t *remap,
		       snd_config_t *map, snd_config_t *sync, snd_ctl_t *child, int mode)
{
	snd_ctl_remap_t *priv;
	snd_ctl_t *ctl;
	size_t index;
	int result, err;

	/* no-op, remove the plugin */
	if (!remap && !map && !sync)
		goto _noop;

	priv = calloc(1, sizeof(*priv));
	if (priv == NULL)
		return -ENOMEM;

	err = parse_remap(priv, remap);
	if (err < 0) {
		result = err;
		goto _err;
	}

	err = parse_map(priv, map);
	if (err < 0) {
		result = err;
		goto _err;
	}

	err = parse_sync(priv, sync);
	if (err < 0) {
		result = err;
		goto _err;
	}

	/* no-op check, remove the plugin */
	if (priv->map_items == 0 && priv->remap_items == 0 && priv->sync_items == 0) {
		remap_free(priv);
 _noop:
		free(child->name);
		child->name = name ? strdup(name) : NULL;
		if (name && !child->name)
			return -ENOMEM;
		*handlep = child;
		return 0;
	}

	priv->event_items = priv->map_items;
	for (index = 0; index < priv->sync_items; index++)
		priv->event_items += priv->sync[index].control_items;
	priv->event_queue = calloc(priv->event_items, sizeof(priv->event_queue[0]));
	if (priv->event_queue == NULL) {
		result = -ENOMEM;
		goto _err;
	}

	priv->numid_remap_active = priv->map_items > 0 || priv->sync_items;

	priv->child = child;
	err = snd_ctl_new(&ctl, SND_CTL_TYPE_REMAP, name, mode);
	if (err < 0) {
		result = err;
		goto _err;
	}
	ctl->ops = &snd_ctl_remap_ops;
	ctl->private_data = priv;
	ctl->poll_fd = child->poll_fd;

	*handlep = ctl;
	return 0;

 _err:
	remap_free(priv);
	return result;
}

/*! \page control_plugins

\section control_plugins_remap Plugin: Remap & map

This plugin can remap (rename) identifiers (except the numid part) for
a child control to another. The plugin can also merge the multiple
child controls to one or split one control to more.

\code
ctl.name {
	type remap              # Remap controls
	child STR               # Child name
	# or
	child {                 # Child definition
		type STR
		...
	}
	remap {
		# the ID strings are parsed in the amixer style like 'name="Headphone Playback Switch",index=2'
		SRC_ID1_STR DST_ID1_STR
		SRC_ID2_STR DST_ID2_STR
		...
	}
	map {
		# join two stereo controls to one
		CREATE_ID1_STR {
			SRC_ID1_STR {
				vindex.0 0	# source channel 0 to merged channel 0
				vindex.1 1
			}
			SRC_ID2_STR {
				vindex.2 0
				vindex.3 1	# source channel 1 to merged channel 3
			}
		}
		# split stereo to mono
		CREATE_ID2_STR {
			SRC_ID3_STR {
				vindex.0 0	# stereo to mono (first channel)
			}
		}
		CREATE_ID3_STR {
			SRC_ID4_STR {
				vindex.0 1	# stereo to mono (second channel)
			}
		}
		# join two stereo to one stereo (minimum value is returned for read operation)
		CREATE_ID4_STR {
			SRC_ID5_STR.vindex.0 [ 0 1 ] # source channels 0+1 to merged channel 0
			SRC_ID6_STR.vindex.1 [ 0 1 ] # source channels 0+1 to merged channel 1
		}
	}
	sync {
		# synchronize multiple controls without any translations
		sample_group_1 [
			SYNC_ID1_STR
			SYNC_ID2_STR
		]
		# synchronize multiple controls without any translations
		# add functionality on/off switch
		sample_group_2 {
			switch SYNC_SWITCH_ID
			controls [
				SYNC_ID3_STR
				SYNC_ID4_STR
			]
		}
	}
}
\endcode

\subsection control_plugins_route_funcref Function reference

<UL>
  <LI>snd_ctl_remap_open()
  <LI>_snd_ctl_remap_open()
</UL>

*/

/**
 * \brief Creates a new remap & map control plugin
 * \param handlep Returns created control handle
 * \param name Name of control
 * \param root Root configuration node
 * \param conf Configuration node with control remap description
 * \param mode Control handle mode
 * \retval zero on success otherwise a negative error code
 * \warning Using of this function might be dangerous in the sense
 *          of compatibility reasons. The prototype might be freely
 *          changed in future.
 */
int _snd_ctl_remap_open(snd_ctl_t **handlep, char *name, snd_config_t *root, snd_config_t *conf, int mode)
{
	snd_config_iterator_t i, next;
	snd_config_t *child = NULL;
	snd_config_t *remap = NULL;
	snd_config_t *map = NULL;
	snd_config_t *sync = NULL;
	snd_ctl_t *cctl;
	int err;

	snd_config_for_each(i, next, conf) {
		snd_config_t *n = snd_config_iterator_entry(i);
		const char *id;
		if (snd_config_get_id(n, &id) < 0)
			continue;
		if (_snd_conf_generic_id(id))
			continue;
		if (strcmp(id, "remap") == 0) {
			remap = n;
			continue;
		}
		if (strcmp(id, "map") == 0) {
			map = n;
			continue;
		}
		if (strcmp(id, "sync") == 0) {
			sync = n;
			continue;
		}
		if (strcmp(id, "child") == 0) {
			child = n;
			continue;
		}
		SNDERR("Unknown field %s", id);
		return -EINVAL;
	}
	if (!child) {
		SNDERR("child is not defined");
		return -EINVAL;
	}
	err = _snd_ctl_open_child(&cctl, root, child, mode, conf);
	if (err < 0)
		return err;
	err = snd_ctl_remap_open(handlep, name, remap, map, sync, cctl, mode);
	if (err < 0)
		snd_ctl_close(cctl);
	return err;
}
#ifndef DOC_HIDDEN
SND_DLSYM_BUILD_VERSION(_snd_ctl_remap_open, SND_CONTROL_DLSYM_VERSION);
#endif
