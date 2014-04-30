/*
 *  cpuinfo-private.h - Private interface
 *
 *  cpuinfo (C) 2006-2007 Gwenole Beauchesne
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public License
 *  as published by the Free Software Foundation; either version 2.1
 *  of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef CPUINFO_PRIVATE_H
#define CPUINFO_PRIVATE_H

#ifdef __cplusplus
extern "C" {
#endif

#define CPUINFO_FEATURES_SZ_(NAME) \
		(1 + ((CPUINFO_FEATURE_##NAME##_MAX - CPUINFO_FEATURE_##NAME) / 32))

struct cpuinfo {
  int vendor;											// CPU vendor
  char *model;											// CPU model name
  int frequency;										// CPU frequency in MHz
  int socket;											// CPU socket type
  int n_cores;											// Number of CPU cores
  int n_threads;										// Number of threads per CPU core
  cpuinfo_cache_t cache_info;							// Cache descriptors
  uint32_t features[CPUINFO_FEATURES_SZ_(COMMON)];		// Common CPU features
  void *opaque;											// Arch-dependent data
};

/* ========================================================================= */
/* == Lists                                                               == */
/* ========================================================================= */

typedef struct cpuinfo_list {
  const void *data;
  struct cpuinfo_list *next;
} *cpuinfo_list_t;

// Clear list
extern int cpuinfo_list_clear(cpuinfo_list_t *lp) attribute_hidden;

// Insert new element into the list
extern int cpuinfo_list_insert(cpuinfo_list_t *lp, const void *ptr, int size) attribute_hidden;
#define cpuinfo_list_insert(LIST, PTR) (cpuinfo_list_insert)(LIST, PTR, sizeof(*(PTR)))

#define cpuinfo_caches_list_insert(PTR) do {		\
  if (cpuinfo_list_insert(&caches_list, PTR) < 0) {	\
	cpuinfo_list_clear(&caches_list);				\
	return NULL;									\
  }													\
} while (0)

/* ========================================================================= */
/* == Processor Features Information                                      == */
/* ========================================================================= */

// Feature test function (expected to SIGILL if opcode is not supported)
typedef void (*cpuinfo_feature_test_function_t)(void);

// Returns true if function succeeds, false if SIGILL was caught
extern int cpuinfo_feature_test_function(cpuinfo_feature_test_function_t func) attribute_hidden;

// Accessors for cpuinfo_features[] table
extern int cpuinfo_feature_get_bit(struct cpuinfo *cip, int feature) attribute_hidden;
extern void cpuinfo_feature_set_bit(struct cpuinfo *cip, int feature) attribute_hidden;

/* ========================================================================= */
/* == Arch-specific Interface                                             == */
/* ========================================================================= */

// Returns a new cpuinfo descriptor
extern int cpuinfo_arch_new(struct cpuinfo *cip) attribute_hidden;

// Release the cpuinfo descriptor and all allocated data
extern void cpuinfo_arch_destroy(struct cpuinfo *cip) attribute_hidden;

// Get processor vendor ID 
extern int cpuinfo_arch_get_vendor(struct cpuinfo *cip) attribute_hidden;

// Get processor name
extern char *cpuinfo_arch_get_model(struct cpuinfo *cip) attribute_hidden;

// Get processor frequency in MHz
extern int cpuinfo_arch_get_frequency(struct cpuinfo *cip) attribute_hidden;

// Get processor socket ID
extern int cpuinfo_arch_get_socket(struct cpuinfo *cip) attribute_hidden;

// Get number of cores per CPU package
extern int cpuinfo_arch_get_cores(struct cpuinfo *cip) attribute_hidden;

// Get number of threads per CPU core
extern int cpuinfo_arch_get_threads(struct cpuinfo *cip) attribute_hidden;

// Get cache information (returns the number of caches detected)
extern cpuinfo_list_t cpuinfo_arch_get_caches(struct cpuinfo *cip) attribute_hidden;

// Returns features table
extern uint32_t *cpuinfo_arch_feature_table(struct cpuinfo *cip, int feature) attribute_hidden;

// Returns 1 if CPU supports the specified feature
extern int cpuinfo_arch_has_feature(struct cpuinfo *cip, int feature) attribute_hidden;

#ifdef __cplusplus
}
#endif

#endif /* CPUINFO_PRIVATE_H */
