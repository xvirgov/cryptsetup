/*
 * libcryptsetup - cryptsetup library internal
 *
 * Copyright (C) 2004, Jana Saout <jana@saout.de>
 * Copyright (C) 2004-2007, Clemens Fruhwirth <clemens@endorphin.org>
 * Copyright (C) 2009-2017, Red Hat, Inc. All rights reserved.
 * Copyright (C) 2009-2017, Milan Broz
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef INTERNAL_H
#define INTERNAL_H

#include <stdint.h>
#include <stdarg.h>
#include <stdbool.h>
#include <unistd.h>
#include <inttypes.h>

#include "nls.h"
#include "bitops.h"
#include "utils_crypt.h"
#include "utils_loop.h"
#include "utils_dm.h"
#include "utils_fips.h"
#include "crypto_backend.h"

#include "libcryptsetup.h"

/* to silent gcc -Wcast-qual for const cast */
#define CONST_CAST(x) (x)(uintptr_t)

#define SECTOR_SHIFT		9
#define SECTOR_SIZE		(1 << SECTOR_SHIFT)
#define DEFAULT_DISK_ALIGNMENT	1048576 /* 1MiB */
#define DEFAULT_MEM_ALIGNMENT	4096
#define MAX_ERROR_LENGTH	512

#define at_least(a, b) ({ __typeof__(a) __at_least = (a); (__at_least >= (b))?__at_least:(b); })

struct crypt_device;

struct volume_key {
	size_t keylength;
	char key[];
};

struct volume_key *crypt_alloc_volume_key(size_t keylength, const char *key);
struct volume_key *crypt_generate_volume_key(struct crypt_device *cd, size_t keylength);
void crypt_free_volume_key(struct volume_key *vk);

/* Device backend */
struct device;
int device_alloc(struct device **device, const char *path);
void device_free(struct device *device);
const char *device_path(const struct device *device);
const char *device_block_path(const struct device *device);
void device_topology_alignment(struct device *device,
			    unsigned long *required_alignment, /* bytes */
			    unsigned long *alignment_offset,   /* bytes */
			    unsigned long default_alignment);
size_t device_block_size(struct device *device);
int device_read_ahead(struct device *device, uint32_t *read_ahead);
int device_size(struct device *device, uint64_t *size);
int device_open(struct device *device, int flags);
void device_disable_direct_io(struct device *device);
int device_is_identical(struct device *device1, struct device *device2);
int device_is_rotational(struct device *device);
size_t device_alignment(struct device *device);

enum devcheck { DEV_OK = 0, DEV_EXCL = 1, DEV_SHARED = 2 };
int device_check_access(struct crypt_device *cd,
			struct device *device,
			enum devcheck device_check);
int device_block_adjust(struct crypt_device *cd,
			struct device *device,
			enum devcheck device_check,
			uint64_t device_offset,
			uint64_t *size,
			uint32_t *flags);
size_t size_round_up(size_t size, size_t block);

/* Receive backend devices from context helpers */
struct device *crypt_metadata_device(struct crypt_device *cd);
struct device *crypt_data_device(struct crypt_device *cd);

int crypt_confirm(struct crypt_device *cd, const char *msg);

char *crypt_lookup_dev(const char *dev_id);
int crypt_dev_is_rotational(int major, int minor);
int crypt_dev_is_partition(const char *dev_path);
char *crypt_get_partition_device(const char *dev_path, uint64_t offset, uint64_t size);
char *crypt_get_base_device(const char *dev_path);
uint64_t crypt_dev_partition_offset(const char *dev_path);

ssize_t write_buffer(int fd, const void *buf, size_t count);
ssize_t read_buffer(int fd, void *buf, size_t count);
ssize_t write_blockwise(int fd, size_t bsize, size_t alignment, void *orig_buf, size_t count);
ssize_t read_blockwise(int fd, size_t bsize, size_t alignment, void *buf, size_t count);
ssize_t write_lseek_blockwise(int fd, size_t bsize, size_t alignment, void *buf, size_t count, off_t offset);
ssize_t read_lseek_blockwise(int fd, size_t bsize, size_t alignment, void *buf, size_t count, off_t offset);

size_t crypt_getpagesize(void);
int init_crypto(struct crypt_device *ctx);

void logger(struct crypt_device *cd, int class, const char *file, int line, const char *format, ...) __attribute__ ((format (printf, 5, 6)));
#define log_dbg(x...) logger(NULL, CRYPT_LOG_DEBUG, __FILE__, __LINE__, x)
#define log_std(c, x...) logger(c, CRYPT_LOG_NORMAL, __FILE__, __LINE__, x)
#define log_verbose(c, x...) logger(c, CRYPT_LOG_VERBOSE, __FILE__, __LINE__, x)
#define log_err(c, x...) logger(c, CRYPT_LOG_ERROR, __FILE__, __LINE__, x)

int crypt_get_debug_level(void);

int crypt_memlock_inc(struct crypt_device *ctx);
int crypt_memlock_dec(struct crypt_device *ctx);

int crypt_random_init(struct crypt_device *ctx);
int crypt_random_get(struct crypt_device *ctx, char *buf, size_t len, int quality);
void crypt_random_exit(void);
int crypt_random_default_key_rng(void);

int crypt_plain_hash(struct crypt_device *ctx,
		     const char *hash_name,
		     char *key, size_t key_size,
		     const char *passphrase, size_t passphrase_size);
int PLAIN_activate(struct crypt_device *cd,
		     const char *name,
		     struct volume_key *vk,
		     uint64_t size,
		     uint32_t flags);

void *crypt_get_hdr(struct crypt_device *cd, const char *type);

int crypt_wipe_device(struct crypt_device *cd,
	struct device *device,
	crypt_wipe_pattern pattern,
	uint64_t offset,
	uint64_t length,
	size_t wipe_block_size,
	int (*progress)(uint64_t size, uint64_t offset, void *usrptr),
	void *usrptr);

#endif /* INTERNAL_H */
