/*
 *  cpuinfo-common.c - Common utility functions
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

#include "sysdeps.h"
#include <signal.h>
#include <setjmp.h>
#include <assert.h>
#include "cpuinfo.h"
#include "cpuinfo-private.h"

#define DEBUG 0
#include "debug.h"


cpuinfo_feature_t cpuinfo_feature_common = CPUINFO_FEATURE_COMMON,
		  cpuinfo_feature_common_max = CPUINFO_FEATURE_COMMON_MAX;

// Returns a new cpuinfo descriptor
struct cpuinfo *cpuinfo_new(void)
{
  cpuinfo_t *cip = (cpuinfo_t *)malloc(sizeof(*cip));
  if (cip) {
	cip->vendor = -1;
	cip->model = NULL;
	cip->frequency = -1;
	cip->socket = -1;
	cip->n_cores = -1;
	cip->n_threads = -1;
	cip->cache_info.count = -1;
	cip->cache_info.descriptors = NULL;
	cip->opaque = NULL;
	memset(cip->features, 0, sizeof(cip->features));
	if (cpuinfo_arch_new(cip) < 0) {
	  free(cip);
	  return NULL;
	}
  }
  return cip;
}

// Release the cpuinfo descriptor and all allocated data
void cpuinfo_destroy(struct cpuinfo *cip)
{
  if (cip) {
	cpuinfo_arch_destroy(cip);
	if (cip->model)
	  free(cip->model);
	if (cip->cache_info.descriptors)
	  free((void *)cip->cache_info.descriptors);
	free(cip);
  }
}

// Get processor vendor ID 
int cpuinfo_get_vendor(struct cpuinfo *cip)
{
  if (cip == NULL)
	return -1;
  if (cip->vendor < 0) {
	cip->vendor = cpuinfo_arch_get_vendor(cip);
	if (cip->vendor < 0)
	  cip->vendor = CPUINFO_VENDOR_UNKNOWN;
  }
  return cip->vendor;
}

// Get processor name
const char *cpuinfo_get_model(struct cpuinfo *cip)
{
  if (cip == NULL)
	return NULL;
  if (cip->model == NULL) {
	cip->model = cpuinfo_arch_get_model(cip);
	if (cip->model == NULL) {
	  static const char unknown_model[] = "<unknown>";
	  if ((cip->model = (char *)malloc(sizeof(unknown_model))) == NULL)
		return NULL;
	  strcpy(cip->model, unknown_model);
	}
  }
  return cip->model;
}

// Get processor frequency in MHz
int cpuinfo_get_frequency(struct cpuinfo *cip)
{
  if (cip == NULL)
	return -1;
  if (cip->frequency <= 0)
	cip->frequency = cpuinfo_arch_get_frequency(cip);
  return cip->frequency;
}

// Get processor socket ID
int cpuinfo_get_socket(struct cpuinfo *cip)
{
  if (cip == NULL)
	return -1;
  if (cip->socket < 0) {
	cip->socket = cpuinfo_arch_get_socket(cip);
	if (cip->socket < 0)
	  cip->socket = CPUINFO_SOCKET_UNKNOWN;
  }
  return cip->socket;
}

// Get number of cores per CPU package
int cpuinfo_get_cores(struct cpuinfo *cip)
{
  if (cip == NULL)
	return -1;
  if (cip->n_cores < 0) {
	cip->n_cores = cpuinfo_arch_get_cores(cip);
	if (cip->n_cores < 1)
	  cip->n_cores = 1;
  }
  return cip->n_cores;
}

// Get number of threads per CPU core
int cpuinfo_get_threads(struct cpuinfo *cip)
{
  if (cip == NULL)
	return -1;
  if (cip->n_threads < 0) {
	cip->n_threads = cpuinfo_arch_get_threads(cip);
	if (cip->n_threads < 1)
	  cip->n_threads = 1;
  }
  return cip->n_threads;
}

// Cache descriptor comparator
static int cache_desc_compare(const void *a, const void *b)
{
  const cpuinfo_cache_descriptor_t *cdp1 = (const cpuinfo_cache_descriptor_t *)a;
  const cpuinfo_cache_descriptor_t *cdp2 = (const cpuinfo_cache_descriptor_t *)b;

  if (cdp1->type == cdp2->type)
	return cdp1->level - cdp2->level;
  
  // trace cache first
  if (cdp1->type == CPUINFO_CACHE_TYPE_TRACE)
	return -1;
  if (cdp2->type == CPUINFO_CACHE_TYPE_TRACE)
	return +1;

  // code cache next
  if (cdp1->type == CPUINFO_CACHE_TYPE_CODE)
	return -1;
  if (cdp2->type == CPUINFO_CACHE_TYPE_CODE)
	return +1;

  // data cache next
  if (cdp1->type == CPUINFO_CACHE_TYPE_DATA)
	return -1;
  if (cdp2->type == CPUINFO_CACHE_TYPE_DATA)
	return +1;

  // unified cache finally
  if (cdp1->type == CPUINFO_CACHE_TYPE_UNIFIED)
	return -1;
  if (cdp2->type == CPUINFO_CACHE_TYPE_UNIFIED)
	return +1;

  // ???
  return 0;
}

// Get cache information (returns read-only descriptors)
const cpuinfo_cache_t *cpuinfo_get_caches(struct cpuinfo *cip)
{
  if (cip == NULL)
	return NULL;
  if (cip->cache_info.count < 0) {
	int count = 0;
	cpuinfo_cache_descriptor_t *descs = NULL;
	cpuinfo_list_t caches_list = cpuinfo_arch_get_caches(cip);
	if (caches_list) {
	  int i;
	  cpuinfo_list_t p = caches_list;
	  while (p) {
		++count;
		p = p->next;
	  }
	  if ((descs = (cpuinfo_cache_descriptor_t *)malloc(count * sizeof(*descs))) != NULL) {
		p = caches_list;
		for (i = 0; i < count; i++) {
		  memcpy(&descs[i], p->data, sizeof(*descs));
		  p = p->next;
		}
		qsort(descs, count, sizeof(*descs), cache_desc_compare);
	  }
	  cpuinfo_list_clear(&caches_list);
	}
	cip->cache_info.count = count;
	cip->cache_info.descriptors = descs;
  }
  return &cip->cache_info;
}

// Returns 1 if CPU supports the specified feature
int cpuinfo_has_feature(struct cpuinfo *cip, int feature)
{
#ifdef HAVE_SYS_PERSONALITY_H
  if((feature == CPUINFO_FEATURE_64BIT) && (personality(0xffffffff) & PER_MASK) == PER_LINUX32)
      return false;
#endif

  return cpuinfo_arch_has_feature(cip, feature);
}


/* ========================================================================= */
/* == Processor Features Information                                      == */
/* ========================================================================= */

static jmp_buf cpuinfo_env; // XXX use a lock!

static void sigill_handler(int sig)
{
  assert(sig == SIGILL);
  longjmp(cpuinfo_env, 1);
}

// Returns true if function succeeds, false if SIGILL was caught
int cpuinfo_feature_test_function(cpuinfo_feature_test_function_t func)
{
#ifdef HAVE_SIGACTION
  struct sigaction old_sigill_sa, sigill_sa;
  sigemptyset(&sigill_sa.sa_mask);
  sigill_sa.sa_flags = 0;
  sigill_sa.sa_handler = sigill_handler;
  if (sigaction(SIGILL, &sigill_sa, &old_sigill_sa) != 0)
	return 0;
#else
  void (*old_sigill_handler)(int);
  if ((old_sigill_handler = signal(SIGILL, sigill_handler)) == SIG_ERR)
	return 0;
#endif

  int has_feature = 0;
  if (setjmp(cpuinfo_env) == 0) {
	func();
	has_feature = 1;
  }

#ifdef HAVE_SIGACTION
  sigaction(SIGILL, &old_sigill_sa, NULL);
#else
  signal(SIGILL, old_sigill_handler);
#endif
  return has_feature;
}

// Accessors for cpuinfo_features[] table
int cpuinfo_feature_get_bit(struct cpuinfo *cip, int feature)
{
  uint32_t *ftp = cpuinfo_arch_feature_table(cip, feature);
  if (ftp) {
	feature &= CPUINFO_FEATURE_MASK;
	return ftp[feature / 32] & (1 << (feature % 32));
  }
  return 0;
}

void cpuinfo_feature_set_bit(struct cpuinfo *cip, int feature)
{
  uint32_t *ftp = cpuinfo_arch_feature_table(cip, feature);
  if (ftp) {
	feature &= CPUINFO_FEATURE_MASK;
	ftp[feature / 32] |= 1 << (feature % 32);
  }
}


/* ========================================================================= */
/* == Stringification of CPU Information bits                             == */
/* ========================================================================= */

const char *cpuinfo_string_of_vendor(int vendor)
{
  const char *str = "<unknown>";
  switch (vendor) {
  case CPUINFO_VENDOR_AMD:		str = "AMD";			break;
  case CPUINFO_VENDOR_CENTAUR:		str = "Centaur";		break;
  case CPUINFO_VENDOR_CYRIX:		str = "Cyrix";			break;
  case CPUINFO_VENDOR_IBM:		str = "IBM";			break;
  case CPUINFO_VENDOR_INTEL:		str = "Intel";			break;
  case CPUINFO_VENDOR_MOTOROLA:		str = "Motorola";		break;
  case CPUINFO_VENDOR_MIPS:		str = "MIPS Technologies";	break;
  case CPUINFO_VENDOR_NEXTGEN:		str = "NextGen";		break;
  case CPUINFO_VENDOR_NSC:		str = "National Semiconductor";	break;
  case CPUINFO_VENDOR_PMC:		str = "PMC-Sierra";		break;
  case CPUINFO_VENDOR_RISE:		str = "Rise Technology";	break;
  case CPUINFO_VENDOR_SIS:		str = "SiS";			break;
  case CPUINFO_VENDOR_TRANSMETA:	str = "Transmeta";		break;
  case CPUINFO_VENDOR_UMC:		str = "UMC";			break;
  case CPUINFO_VENDOR_PASEMI:		str = "P.A. Semi";		break;
  }
  return str;
}

const char *cpuinfo_string_of_socket(int socket)
{
  const char *str = "Socket <unknown>";
  switch (socket) {
  case CPUINFO_SOCKET_478:		str = "Socket 478";		break;
  case CPUINFO_SOCKET_479:		str = "Socket 479";		break;
  case CPUINFO_SOCKET_604:		str = "Socket mPGA604";		break;
  case CPUINFO_SOCKET_771:		str = "Socket LGA771";		break;
  case CPUINFO_SOCKET_775:		str = "Socket LGA775";		break;
  case CPUINFO_SOCKET_754:		str = "Socket 754";		break;
  case CPUINFO_SOCKET_939:		str = "Socket 939";		break;
  case CPUINFO_SOCKET_940:		str = "Socket 940";		break;
  case CPUINFO_SOCKET_AM2:		str = "Socket AM2";		break;
  case CPUINFO_SOCKET_F:		str = "Socket F";		break;
  case CPUINFO_SOCKET_S1:		str = "Socket S1";		break;
  }
  return str;
}

const char *cpuinfo_string_of_cache_type(int cache_type)
{
  const char *str = "<unknown>";
  switch (cache_type) {
  case CPUINFO_CACHE_TYPE_DATA:		str = "data";		break;
  case CPUINFO_CACHE_TYPE_CODE:		str = "code";		break;
  case CPUINFO_CACHE_TYPE_UNIFIED:	str = "unified";	break;
  case CPUINFO_CACHE_TYPE_TRACE:	str = "trace";		break;
  }
  return str;
}

typedef struct {
#ifndef HAVE_DESIGNATED_INITIALIZERS
  int feature;
#endif
  const char *name;
  const char *detail;
} cpuinfo_feature_string_t;

#ifdef HAVE_DESIGNATED_INITIALIZERS
#define DEFINE_(ID, NAME, DETAIL) \
		[CPUINFO_FEATURE_##ID & CPUINFO_FEATURE_MASK] = { NAME, DETAIL }
#else
#define DEFINE_(ID, NAME, DETAIL) \
		{ CPUINFO_FEATURE_##ID, NAME, DETAIL }
#endif

static const cpuinfo_feature_string_t common_feature_strings[] = {
  DEFINE_(64BIT,		"64bit",	"64-bit instructions"								),
  DEFINE_(SIMD,			"simd",		"SIMD instructions"									),
  DEFINE_(POPCOUNT,		"popcount",	"Population count instruction"						),
};

static const int n_common_feature_strings = sizeof(common_feature_strings) / sizeof(common_feature_strings[0]);

static const cpuinfo_feature_string_t x86_feature_strings[] = {
  DEFINE_(X86,			"[x86]",	"-- x86-specific features --"),
  DEFINE_(X86_AC,		"ac",		"Alignment Check"),
  DEFINE_(X86_CPUID,		"cpuid",	"CPU Identificaion"),
  DEFINE_(X86_FPU,		"fpu",		"Floating Point Unit On-Chip"),
  DEFINE_(X86_VME,		"vme",		"Virtual 8086 Mode Enhancements"),
  DEFINE_(X86_DE,		"de",		"Debugging Extensions"),
  DEFINE_(X86_PSE,		"pse",		"Page Size Extension"),
  DEFINE_(X86_TSC,		"tsc",		"Time Stamp Counter"),
  DEFINE_(X86_MSR,		"msr",		"Model Specific Registers RDMSR and WRMSR Instructions"),
  DEFINE_(X86_PAE,		"pae",		"Physical Address Extension"),
  DEFINE_(X86_MCE,		"mce",		"Machine Check Exception"),
  DEFINE_(X86_CX8,		"cx8",		"CMPCXHG8B Instruction"),
  DEFINE_(X86_APIC,		"apic",		"APIC On-Chip"),
  DEFINE_(X86_SEP,		"sep",		"SYSENTER and SYSEXIT Instructions"),
  DEFINE_(X86_MTRR,		"mtrr",		"Memory Type Range Registers"),
  DEFINE_(X86_PGE,		"pge",		"PTE Global Bit"),
  DEFINE_(X86_MCA,		"mca",		"Machine Check Architecture"),
  DEFINE_(X86_CMOV,		"cmov",		"Conditional Moves"),
  DEFINE_(X86_PAT,		"pat",		"Page Attribute Table"),
  DEFINE_(X86_PSE_36,		"pse36",	"36-Bit Page Size Extension"),
  DEFINE_(X86_PSN,		"psn",		"Processor Serial Number"),
  DEFINE_(X86_CLFLUSH,		"clflush",	"CLFLUSH Instruction"),
  DEFINE_(X86_DS,		"ds",		"Debug Store"),
  DEFINE_(X86_ACPI,		"acpi",		"Thermal Monitor and Software Controlled Clock Facilities"),
  DEFINE_(X86_FXSR,		"fxsr",		"Supports FXSAVE/FXSTOR instructions"),
  DEFINE_(X86_FFXSR,		"ffxsr",	"Supports FXSAVE/FXSTOR instruction optimizations"),
  DEFINE_(X86_SS,		"ss",		"Self Snoop"),
  DEFINE_(X86_HTT,		"htt",		"Hyper-Threading Technology"),
  DEFINE_(X86_PBE,		"pbe",		"Pending Break Enable"),
  DEFINE_(X86_MMX,		"mmx",		"MMX Technology"),
  DEFINE_(X86_MMX_EXT,		"mmxext",	"MMX+ Technology (AMD or Cyrix)"),
  DEFINE_(X86_3DNOW,		"3dnow",	"3DNow! Technology"),
  DEFINE_(X86_3DNOW_EXT,	"3dnowext",	"Enhanced 3DNow! Technology"),
  DEFINE_(X86_3DNOW_PREFETCH,	"3dnowprefetch","3DNow! prefetch"),
  DEFINE_(X86_SSE,		"sse",		"SSE Technology"),
  DEFINE_(X86_SSE2,		"sse2",		"SSE2 Technology"),
  DEFINE_(X86_SSE3,		"sse3",		"SSE3 Technology (Prescott New Instructions)"),
  DEFINE_(X86_SSSE3,		"ssse3",	"SSSE3 Technology (Merom New Instructions)"),
  DEFINE_(X86_SSE4_1,		"sse4.1",	"SSE4.1 Technology (Penryn New Instructions)"),
  DEFINE_(X86_SSE4_2,		"sse4.2",	"SSE4.2 Technology (Nehalem New Instructions)"),
  DEFINE_(X86_SSE4A,		"sse4a",	"SSE4A Technology (AMD Barcelona Instructions)"),
  DEFINE_(X86_SSE5,		"sse5",		"SSE5 Technology (AMD Bulldozer Instructions)"),
  DEFINE_(X86_MISALIGNSSE,	"misalignsse",	"Misaligned SSE mode"),
  DEFINE_(X86_VMX,		"vmx",		"Intel Virtualisation Technology (VT)"),
  DEFINE_(X86_SVM,		"svm",		"AMD-v Technology (Pacifica)"),
  DEFINE_(X86_LM,		"lm",		"Long Mode (64-bit capable)"),
  DEFINE_(X86_LAHF64,		"lahf_lm",	"LAHF/SAHF Supported in 64-bit mode"),
  DEFINE_(X86_POPCNT,		"popcnt",	"POPCNT (population count) instruction supported"),
  DEFINE_(X86_TSC_DEADLINE,	"tsc_deadline",	"Time Stamp Counter Deadline"),
  DEFINE_(X86_ABM,		"abm",		"Advanced Bit Manipulation instructions (LZCNT9"),
  DEFINE_(X86_BSFCC,		"bsf_cc",	"BSF instruction clobbers condition codes"),
  DEFINE_(X86_TM,		"tm",		"Thermal Monitor"),
  DEFINE_(X86_TM2,		"tm2",		"Thermal Monitor 2"),
  DEFINE_(X86_IA64,		"ia64",		"Intel 64 Instruction Set Architecture"),
  DEFINE_(X86_EIST,		"eist",		"Enhanced Intel Speedstep Technology"),
  DEFINE_(X86_NX,		"nx",		"No eXecute (AMD NX) / Execute Disable (Intel XD)"),
  DEFINE_(X86_DTES64,		"dtes64",	"64-bit DS Area"),
  DEFINE_(X86_MONITOR,		"monitor",	"MONITOR/MWAIT"),
  DEFINE_(X86_DS_CPL,		"ds_cpl",	"CPL Qualified Debug Store"),
  DEFINE_(X86_SMX,		"smx",		"Safer Mode Extensions"),
  DEFINE_(X86_CNXT_ID,		"cnxt_id",	"L1 Context ID"),
  DEFINE_(X86_CX16,		"cx16",		"Supports CMPCXHG16B Instruction"),
  DEFINE_(X86_XTPR,		"xtpr",		"xTPR Update Conrol"),
  DEFINE_(X86_PDCM,		"pdcm",		"Perfmon and Debug Capability"),
  DEFINE_(X86_PCID,		"pcid",		"Process Context Identifiers"),
  DEFINE_(X86_DCA,		"dca",		"Supports prefetching from memory mapped device"),
  DEFINE_(X86_X2APIC,		"x2apic",	"Supports x2APIC"),
  DEFINE_(X86_MOVBE,		"movbe",	"Supports MOVBE instruction"),
  DEFINE_(X86_XSAVE,		"xsave",	"Supports XSAVE/XRSTOR instructions"),
  DEFINE_(X86_PCLMULQDQ,	"pclmulqdq",	"Supports PCLMULQDQ instruction"),
  DEFINE_(X86_FMA,		"fma",		"Supports FMA extensions using YMM state."),
  DEFINE_(X86_AES,		"aes",		"Supports AES instruction"),
  DEFINE_(X86_AVX,		"avx",		"Supports Advanced Vector Extensions"),
  DEFINE_(X86_F16C,		"f16c",		"F16C half-precision convert instruction"),
  DEFINE_(X86_HYPERVISOR,	"hypervisor",	"Hypervisor Guest Status"),
  DEFINE_(X86_CMP_LEGACY,	"cmp_legacy",	"Core multi-processing legacy mode"),
  DEFINE_(X86_EXTAPIC,		"extapic",	"Supports Advanced Vector Extensions"),
  DEFINE_(X86_CR8_LEGACY,	"cr8_legacy",	"LOCK MOV CR0 means MOV CR8"),
  DEFINE_(X86_OSVW,		"osvw",		"OS visible workaround"),
  DEFINE_(X86_IBS,		"ibs",		"Instruction based sampling"),
  DEFINE_(X86_SKINIT,		"skinit",	"Supports SKINIT/STGI instructions"),
  DEFINE_(X86_WDT,		"wdt",		"Watchdog timer support"),
  DEFINE_(X86_LWP,		"lwp",		"Lightweight profiling support"),
  DEFINE_(X86_FMA4,		"fma4",		"4-operand FMA instruction"),
  DEFINE_(X86_NODEID_MSR,	"nodeid_msr",	"NodeId MSR C001100C"),
  DEFINE_(X86_TBM,		"tbm",		"Trailing bit manipulation instruction support"),
  DEFINE_(X86_TOPOEXT,		"topoext",	"Topology extensions support"),
  DEFINE_(X86_PAGE1GB,		"page1gb",	"1-GB large page support"),
  DEFINE_(X86_RDTSCP,		"rdtscp",	"Supports RDTSCP instruction"),
  


};

static const int n_x86_feature_strings = sizeof(x86_feature_strings) / sizeof(x86_feature_strings[0]);

static const cpuinfo_feature_string_t ia64_feature_strings[] = {
  DEFINE_(IA64,			"[ia64]",	"-- ia64-specific features --"),
  DEFINE_(IA64_LB,		"lb",		"Long branch (brl) instruction available"),
  DEFINE_(IA64_SD,		"sd",		"Spontaneous deferral supported"),
  DEFINE_(IA64_AO,		"ao",		"16-byte atomic operations"),
};

static const int n_ia64_feature_strings = sizeof(ia64_feature_strings) / sizeof(ia64_feature_strings[0]);

static const cpuinfo_feature_string_t ppc_feature_strings[] = {
  DEFINE_(PPC,			"[ppc]",	"-- ppc-specific features --"),
  DEFINE_(PPC_VMX,		"vmx",		"Vector instruction set (AltiVec, VMX)"),
  DEFINE_(PPC_GPOPT,		"gpopt",	"General Purpose group optional instructions (fsqrt)"),
  DEFINE_(PPC_GFXOPT,		"gfxopt",	"Graphics group optional instructions (fsel, fres)"),
  DEFINE_(PPC_MFCRF,		"mfcrf",	"PowerPC V2.01 single field mfcr instruction"),
  DEFINE_(PPC_POPCNTB,		"popcntb",	"PowerPC V2.02 popcntb instruction"),
  DEFINE_(PPC_FPRND,		"fprnd",	"PowerPC V2.02 floating point rounding instructions (friz, frin)"),
  DEFINE_(PPC_MFPGPR,		"mfpgpr",	"PowerPC V2.05 move floating point to/from GPR instructions"),
};

static const int n_ppc_feature_strings = sizeof(ppc_feature_strings) / sizeof(ppc_feature_strings[0]);

static const cpuinfo_feature_string_t mips_feature_strings[] = {
  DEFINE_(MIPS,			"[mips]",	"-- mips-specific features --"),
};

static const int n_mips_feature_strings = sizeof(mips_feature_strings) / sizeof(mips_feature_strings[0]);

#undef DEFINE_

static const cpuinfo_feature_string_t *cpuinfo_feature_string_ptr(int feature)
{
  int fss = -1;
  const cpuinfo_feature_string_t *fsp = NULL;
  switch (feature & CPUINFO_FEATURE_ARCH) {
  case CPUINFO_FEATURE_COMMON:
	fsp = common_feature_strings;
	fss = n_common_feature_strings;
	break;
  case CPUINFO_FEATURE_X86:
	fsp = x86_feature_strings;
	fss = n_x86_feature_strings;
	break;
  case CPUINFO_FEATURE_IA64:
	fsp = ia64_feature_strings;
	fss = n_ia64_feature_strings;
	break;
  case CPUINFO_FEATURE_PPC:
	fsp = ppc_feature_strings;
	fss = n_ppc_feature_strings;
	break;
  case CPUINFO_FEATURE_MIPS:
	fsp = mips_feature_strings;
	fss = n_mips_feature_strings;
	break;
  }
  if (fsp) {
#ifdef HAVE_DESIGNATED_INITIALIZERS
	return &fsp[feature & CPUINFO_FEATURE_MASK];
#else
	int i;
	for (i = 0; i < fss; i++) {
	  if (fsp[i].feature == feature)
		return &fsp[i];
	}
#endif
  }
  return NULL;
}

const char *cpuinfo_string_of_feature(int feature)
{
  const cpuinfo_feature_string_t *fsp = cpuinfo_feature_string_ptr(feature);
  return fsp && fsp->name ? fsp->name : "<unknown>";
}

const char *cpuinfo_string_of_feature_detail(int feature)
{
  const cpuinfo_feature_string_t *fsp = cpuinfo_feature_string_ptr(feature);
  return fsp->detail ? fsp->detail : "<unknown>";
}


/* ========================================================================= */
/* == Lists                                                               == */
/* ========================================================================= */

// Clear list
int cpuinfo_list_clear(cpuinfo_list_t *lp)
{
  assert(lp != NULL);
  cpuinfo_list_t d, p = *lp;
  while (p) {
	d = p;
	p = p->next;
	free(d);
  }
  *lp = NULL;
  return 0;
}

// Insert new element into the list
int (cpuinfo_list_insert)(cpuinfo_list_t *lp, const void *ptr, int size)
{
  assert(lp != NULL);
  cpuinfo_list_t p = (cpuinfo_list_t)malloc(sizeof(*p));
  if (p == NULL)
	return -1;
  p->next = *lp;
  if ((p->data = malloc(size)) == NULL) {
	free(p);
	return -1;
  }
  memcpy((void *)p->data, ptr, size);
  *lp = p;
  return 0;
}
