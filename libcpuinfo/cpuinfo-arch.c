/*
 *  cpuinfo-x86.c - Processor identification code, x86 specific
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
#include <unistd.h>
#include <ctype.h>
#include <sys/time.h>
#if defined __linux__
#include <sys/utsname.h>
#endif
#include "cpuinfo.h"
#include "cpuinfo-private.h"

#define DEBUG 1
#include "debug.h"

cpuinfo_feature_t cpuinfo_feature_architecture = CPUINFO_FEATURE_X86,
		  cpuinfo_feature_architecture_max = CPUINFO_FEATURE_X86_MAX;

static int cpuinfo_has_ac()
{
  unsigned long a, c;
  __asm__ __volatile__ ("pushf\n\t"
						"pop %0\n\t"
						"mov %0, %1\n\t"
						"xor $0x40000, %0\n\t"
						"push %0\n\t"
						"popf\n\t"
						"pushf\n\t"
						"pop %0\n\t"
						: "=a" (a), "=c" (c)
						:: "cc");

  return a != c;
}

static int cpuinfo_has_cpuid()
{
  unsigned long a, c;
  __asm__ __volatile__ ("pushf\n\t"
						"pop %0\n\t"
						"mov %0, %1\n\t"
						"xor $0x200000, %0\n\t"
						"push %0\n\t"
						"popf\n\t"
						"pushf\n\t"
						"pop %0\n\t"
						: "=a" (a), "=c" (c)
						:: "cc");

  return a != c;
}

static void cpuid(uint32_t op, uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx)
{
  uint32_t a = eax ? *eax : 0;
  uint32_t b = ebx ? *ebx : 0;
  uint32_t c = ecx ? *ecx : 0;
  uint32_t d = edx ? *edx : 0;
  if(!cpuinfo_has_cpuid())
	return;

#if defined __i386__
  __asm__ __volatile__ ("xchgl	%%ebx,%0\n\t"
						"cpuid	\n\t"
						"xchgl	%%ebx,%0\n\t"
						: "+r" (b), "=a" (a), "=c" (c), "=d" (d)
						: "1" (op), "2" (c));
#else
  __asm__ __volatile__ ("cpuid"
						: "=a" (a), "=b" (b), "=c" (c), "=d" (d)
						: "0" (op), "2" (c));
#endif

  if (eax) *eax = a;
  if (ebx) *ebx = b;
  if (ecx) *ecx = c;
  if (edx) *edx = d;
}

// Arch-dependent data
struct x86_cpuinfo {
  uint32_t features[CPUINFO_FEATURES_SZ_(X86)];
};

typedef struct x86_cpuinfo x86_cpuinfo_t;

// Returns a new cpuinfo descriptor
int cpuinfo_arch_new(struct cpuinfo *cip)
{
  x86_cpuinfo_t *p = (x86_cpuinfo_t *)malloc(sizeof(*p));
  if (p == NULL)
	return -1;
  memset(p->features, 0, sizeof(p->features));
  cip->opaque = p;
  return 0;
}

// Release the cpuinfo descriptor and all allocated data
void cpuinfo_arch_destroy(struct cpuinfo *cip)
{
  if (cip->opaque)
	free(cip->opaque);
}

// Dump all useful information for debugging
int cpuinfo_dump(struct cpuinfo *cip, FILE *out)
{
  uint32_t i, n;
  uint32_t cpuid_level;
  uint32_t eax, ebx, ecx, edx;

  char v[13] = { 0, };
  cpuid(0, &cpuid_level, (uint32_t *)&v[0], (uint32_t *)&v[8], (uint32_t *)&v[4]);
  fprintf(out, "Vendor ID string: '%s'\n", v);
  fprintf(out, "\n");

  fprintf(out, "Maximum supported standard level: %08x\n", cpuid_level);
  for (i = 0; i <= cpuid_level; i++) {
	cpuid(i, &eax, &ebx, &ecx, &edx);
	fprintf(out, "%08x: eax %08x, ebx %08x, ecx %08x, edx %08x\n", i, eax, ebx, ecx, edx);
	if (i == 4) { // special case for cpuid(4)
	  for (n = 0; /* nothing */; n++) {
		ecx = n;
		cpuid(4, &eax, &ebx, &ecx, &edx);
		if ((eax & 0x1f) == 0)
		  break;
		fprintf(out, "--- %04d: eax %08x, ebx %08x, ecx %08x, edx %08x\n", n, eax, ebx, ecx, edx);
	  }
	}
  }
  fprintf(out, "\n");

  // Extended-level
  cpuid(0x80000000, &cpuid_level, NULL, NULL, NULL);
  if ((cpuid_level & 0xffff0000) == 0x80000000) {
	fprintf(out, "Maximum supported extended level: %08x\n", cpuid_level);
	for (i = 0x80000000; i <= cpuid_level; i++) {
	  cpuid(i, &eax, &ebx, &ecx, &edx);
	  fprintf(out, "%08x: eax %08x, ebx %08x, ecx %08x, edx %08x\n", i, eax, ebx, ecx, edx);
	}
	fprintf(out, "\n");
  }

  // Transmeta level
  cpuid(0x80860000, &cpuid_level, NULL, NULL, NULL);
  if ((cpuid_level & 0xffff0000) == 0x80860000) {
	fprintf(out, "Maximum supported Transmeta level: %08x\n", cpuid_level);
	for (i = 0x80860000; i <= cpuid_level; i++) {
	  cpuid(i, &eax, &ebx, &ecx, &edx);
	  fprintf(out, "%08x: eax %08x, ebx %08x, ecx %08x, edx %08x\n", i, eax, ebx, ecx, edx);
	}
	fprintf(out, "\n");
  }

  // Centaur level
  cpuid(0xc0000000, &cpuid_level, NULL, NULL, NULL);
  if ((cpuid_level & 0xffff0000) == 0xc0000000) {
	fprintf(out, "Maximum supported Centaur level: %08x\n", cpuid_level);
	for (i = 0xc0000000; i <= cpuid_level; i++) {
	  cpuid(i, &eax, &ebx, &ecx, &edx);
	  fprintf(out, "%08x: eax %08x, ebx %08x, ecx %08x, edx %08x\n", i, eax, ebx, ecx, edx);
	}
	fprintf(out, "\n");
  }

  return 0;
}

// Get processor vendor ID 
int cpuinfo_arch_get_vendor(struct cpuinfo *cip)
{
  int vendor = -1;

  char v[13] = { 0, };
  cpuid(0, NULL, (uint32_t *)&v[0], (uint32_t *)&v[8], (uint32_t *)&v[4]);

  if (!strcmp(v, "GenuineIntel"))
	vendor = CPUINFO_VENDOR_INTEL;
  else if (!strcmp(v, "AuthenticAMD"))
	vendor = CPUINFO_VENDOR_AMD;
  else if (!strcmp(v, "GenuineTMx86"))
	vendor = CPUINFO_VENDOR_TRANSMETA;
  else if (!strcmp(v, "UMC UMC UMC "))
	vendor = CPUINFO_VENDOR_UMC;
  else if (!strcmp(v, "CyrixInstead"))
	vendor = CPUINFO_VENDOR_CYRIX;
  else if (!strcmp(v, "NexGenDriven"))
	vendor = CPUINFO_VENDOR_NEXTGEN;
  else if (!strcmp(v, "CentaurHauls"))
	vendor = CPUINFO_VENDOR_CENTAUR;
  else if (!strcmp(v, "SiS SiS SiS "))
	vendor = CPUINFO_VENDOR_SIS;
  else if (!strcmp(v, "Geode by NSC"))
	vendor = CPUINFO_VENDOR_NSC;
  else {
	uint32_t cpuid_level;
	cpuid(0x80000000, &cpuid_level, NULL, NULL, NULL);
	if ((cpuid_level & 0xffff0000) == 0x80000000) {
	  cpuid(0x80000000, NULL, (uint32_t *)&v[0], (uint32_t *)&v[8], (uint32_t *)&v[4]);
	  if (!strcmp(v, "TransmetaCPU"))
		vendor = CPUINFO_VENDOR_TRANSMETA;
	}
  }

  if (vendor < 0)
	vendor = CPUINFO_VENDOR_UNKNOWN;

  return vendor;
}

// Get AMD processor name
static char *get_model_amd_npt(struct cpuinfo *cip)
{
  // assume we are a valid AMD NPT Family 0Fh processor
  uint32_t eax, ebx;
  cpuid(0x80000001, &eax, &ebx, NULL, NULL);
  uint32_t BrandId = ebx & 0xffff;

  uint32_t PwrLmt = ((BrandId >> 5) & 0xe) | ((BrandId >> 14) & 1);		// BrandId[8:6,14]
  uint32_t BrandTableIndex = (BrandId >> 9) & 0x1f;						// BrandId[13:9]
  uint32_t NN = ((BrandId >> 9) & 0x40) | (BrandId & 0x3f);				// BrandId[15,5:0]
  int CmpCap = cpuinfo_get_cores(cip) > 1;

  typedef struct processor_name_string {
	int8_t cmp;
	uint8_t index;
	uint8_t pwr_lmt;
	const char *name;
	char model;
  } processor_name_string_t;
  static const processor_name_string_t socket_F_table[] = {
	{  1, 0x01, 0x6, "Opteron 22%d HE",			'R' },
	{  1, 0x01, 0xA, "Opteron 22%d",			'R' },
	{  1, 0x01, 0xC, "Opteron 22%d SE",			'R' },
	{  1, 0x04, 0x6, "Opteron 82%d HE",			'R' },
	{  1, 0x04, 0xA, "Opteron 82%d",			'R' },
	{  1, 0x04, 0xC, "Opteron 82%d SE",			'R' },
	{ -1, 0x00, 0x0, "AMD Engineering Sample",	    },
	{ -1, 0x00, 0x0, NULL,						    }
  };
  static const processor_name_string_t socket_AM2_table[] = {
	{  0, 0x04, 0x4, "Athlon 64 %d00+",			'T' },
	{  0, 0x04, 0x8, "Athlon 64 %d00+",			'T' },
	{  0, 0x06, 0x4, "Sempron %d00+",			'T' },
	{  0, 0x06, 0x8, "Sempron %d00+",			'T' },
	{  1, 0x01, 0xA, "Opteron 12%d",			'R' },
	{  1, 0x01, 0xC, "Opteron 12%d SE",			'R' },
	{  1, 0x04, 0x2, "Athlon 64 X2 %d00+",		'T' },
	{  1, 0x04, 0x6, "Athlon 64 X2 %d00+",		'T' },
	{  1, 0x04, 0x8, "Athlon 64 X2 %d00+",		'T' },
	{  1, 0x05, 0xC, "Athlon 64 FX-%d",			'Z' },
	{ -1, 0x00, 0x0, "AMD Engineering Sample",	    },
	{ -1, 0x00, 0x0, NULL,						    }
  };
  static const processor_name_string_t socket_S1_table[] = {
	{  1, 0x02, 0xC, "Turion 64 X2 TL-%d",		'Y' },
	{ -1, 0x00, 0x0, "AMD Engineering Sample",	    },
	{ -1, 0x00, 0x0, NULL,						    }
  };

  const processor_name_string_t *model_names = NULL;
  switch (cpuinfo_get_socket(cip)) {
  case CPUINFO_SOCKET_F:
	model_names = socket_F_table;
	break;
  case CPUINFO_SOCKET_AM2:
	model_names = socket_AM2_table;
	break;
  case CPUINFO_SOCKET_S1:
	model_names = socket_S1_table;
	break;
  }
  if (model_names == NULL)
	return NULL;

  int i;
  for (i = 0; model_names[i].name != NULL; i++) {
	const processor_name_string_t *mp = &model_names[i];
	if ((mp->cmp == -1 || mp->cmp == CmpCap)
		&& mp->index == BrandTableIndex
		&& mp->pwr_lmt == PwrLmt) {
	  int model_number = mp->model;
	  switch (model_number) {
	  case 'R': model_number = -1 + NN; break;
	  case 'P': model_number = 26 + NN; break;
	  case 'T': model_number = 15 + CmpCap * 10 + NN; break;
	  case 'Z': model_number = 57 + NN; break;
	  case 'Y': model_number = 29 + NN; break;
	  }
	  char *model = (char *)malloc(64);
	  if (model) {
		if (model_number)
		  sprintf("%s", model, mp->name, model_number);
		else
		  sprintf("%s", model, mp->name);
	  }
	  return model;
	}
  }

  return NULL;
}

static char *get_model_amd_k8(struct cpuinfo *cip)
{
  // assume we are a valid AMD K8 Family processor
  uint32_t eax, ebx;
  cpuid(1, &eax, &ebx, NULL, NULL);
  uint32_t eightbit_brand_id = ebx & 0xff;

  if ((eax & 0xfffcff00) == 0x00040f00)
	return get_model_amd_npt(cip);

  uint32_t ecx, edx;
  cpuid(0x80000001, NULL, &ebx, &ecx, &edx);
  uint32_t brand_id = ebx & 0xffff;

  int BrandTableIndex, NN;
  if (eightbit_brand_id != 0) {
	BrandTableIndex = (eightbit_brand_id >> 3) & 0x1c;	// {0b,8BitBrandId[7:5],00b}
	NN = eightbit_brand_id & 0x1f;						// {0b,8BitBrandId[4:0]}
  }
  else if (brand_id == 0) {
	BrandTableIndex = 0;
	NN = 0;
  }
  else {
	BrandTableIndex = (brand_id >> 6) & 0x3f;			// BrandId[11:6]
	NN = brand_id & 0x3f;								// BrandId[5:0]
  }

  static const struct {
	const char *name;
	char model;
  }
  BrandTable[64] = {
	{ "Engineering Sample",		    }, /* 0x00 */
	{ NULL,						    }, /* 0x01 */
	{ NULL,						    }, /* 0x02 */
	{ NULL,						    }, /* 0x03 */
	{ "Athlon 64 %d00+",		'X' }, /* 0x04 */
	{ "Athlon 64 X2 %d00+",		'X' }, /* 0x05 */
	{ NULL,						    }, /* 0x06 */
	{ NULL,						    }, /* 0x07 */
	{ "Athlon 64 %d00+",		'X' }, /* 0x08 */
	{ "Athlon 64 %d00+",		'X' }, /* 0x09 */
	{ "Turion 64 ML-%d",		'X' }, /* 0x0A */
	{ "Turion 64 MT-%d",		'X' }, /* 0x0B */
	{ "Opteron 1%d",			'Y' }, /* 0x0C */
	{ "Opteron 1%d",			'Y' }, /* 0x0D */
	{ "Opteron 1%d HE",			'Y' }, /* 0x0E */
	{ "Opteron 1%d EE",			'Y' }, /* 0x0F */
	{ "Opteron 2%d",			'Y' }, /* 0x10 */
	{ "Opteron 2%d",			'Y' }, /* 0x11 */
	{ "Opteron 2%d HE",			'Y' }, /* 0x12 */
	{ "Opteron 2%d EE",			'Y' }, /* 0x13 */
	{ "Opteron 8%d",			'Y' }, /* 0x14 */
	{ "Opteron 8%d",			'Y' }, /* 0x15 */
	{ "Opteron 8%d HE",			'Y' }, /* 0x16 */
	{ "Opteron 8%d EE",			'Y' }, /* 0x17 */
	{ "Athlon 64 %d00+",		'E' }, /* 0x18 */
	{ NULL,						    }, /* 0x19 */
	{ NULL,						    }, /* 0x1A */
	{ NULL,						    }, /* 0x1B */
	{ NULL,						    }, /* 0x1C */
	{ "Athlon XP-M %d00+",		'X' }, /* 0x1D */
	{ "Athlon XP-M %d00+",		'X' }, /* 0x1E */
	{ NULL,						    }, /* 0x1F */
	{ "Athlon XP %d00+",		'X' }, /* 0x20 */
	{ "Sempron %d00+",			'T' }, /* 0x21 */
	{ "Sempron %d00+",			'T' }, /* 0x22 */
	{ "Sempron %d00+",			'T' }, /* 0x23 */
	{ "Athlon 64 FX-%d",		'Z' }, /* 0x24 */
	{ NULL,						    }, /* 0x01 */
	{ "Sempron %d00+",			'T' }, /* 0x26 */
	{ NULL,						    }, /* 0x27 */
	{ NULL,						    }, /* 0x28 */
	{ "Opteron 1%d SE",			'R' }, /* 0x29 */
	{ "Opteron 2%d SE",			'R' }, /* 0x2A */
	{ "Opteron 8%d SE",			'R' }, /* 0x2B */
	{ "Opteron 1%d",			'R' }, /* 0x2C */
	{ "Opteron 1%d",			'R' }, /* 0x2D */
	{ "Opteron 1%d HE",			'R' }, /* 0x2E */
	{ "Opteron 1%d EE",			'R' }, /* 0x2F */
	{ "Opteron 2%d",			'R' }, /* 0x30 */
	{ "Opteron 2%d",			'R' }, /* 0x31 */
	{ "Opteron 2%d HE",			'R' }, /* 0x32 */
	{ "Opteron 2%d EE",			'R' }, /* 0x33 */
	{ "Opteron 8%d",			'R' }, /* 0x34 */
	{ "Opteron 8%d",			'R' }, /* 0x35 */
	{ "Opteron 8%d HE",			'R' }, /* 0x36 */
	{ "Opteron 8%d EE",			'R' }, /* 0x37 */
	{ "Opteron 1%d",			'R' }, /* 0x38 */
	{ "Opteron 2%d",			'R' }, /* 0x39 */
	{ "Opteron 8%d",			'R' }, /* 0x3A */
	{ NULL,						    }, /* 0x3B */
	{ NULL,						    }, /* 0x3C */
	{ NULL,						    }, /* 0x3D */
	{ NULL,						    }, /* 0x3E */
	{ NULL,						    }  /* 0x3F */
  };
  assert((sizeof(BrandTable) / sizeof(BrandTable[0])) == 64);

  int model_number = BrandTable[BrandTableIndex].model;
  switch (model_number) {
  case 'X': model_number = 22 + NN; break;
  case 'Y': model_number = 38 + (2 * NN); break;
  case 'Z': model_number = 24 + NN; break;
  case 'T': model_number = 24 + NN; break;
  case 'R': model_number = 45 + (5 * NN); break;
  case 'E': model_number = 9 + NN; break;
  }

  const char *name = BrandTable[BrandTableIndex].name;
  if (name == NULL)
	return NULL;

  char *model = (char *)malloc(64);
  if (model) {
	if (model_number)
	  sprintf("%s", model, name, model_number);
	else
	  sprintf("%s", model, name);
  }

  return model;
}

static char *get_model_amd_k7(struct cpuinfo *cip)
{
  // XXX to be filled in later
  return NULL;
}

static char *get_model_amd(struct cpuinfo *cip)
{
  // assume we are a valid AMD processor
  uint32_t cpuid_level;
  cpuid(0, &cpuid_level, NULL, NULL, NULL);
  if (cpuid_level < 1)
	return NULL;

  uint32_t eax;
  cpuid(1, &eax, NULL, NULL, NULL);
  if ((eax & 0xfff0ff00) == 0x00000f00)
	return get_model_amd_k8(cip);

  // AMD Processor Recognition Application Note for Processors Prior to AMD Family OFh Processors (Rev 3.13)
  if ((eax & 0xf00) == 0x600)
	return get_model_amd_k7(cip);

  const char *processor = NULL;
  switch ((eax >> 4) & 0xff) {
  case 0x50:
  case 0x51:
  case 0x52:
  case 0x53:
	processor = "K5";
	break;
  case 0x56:
  case 0x57:
	processor = "K6";
	break;
  case 0x58:
	processor = "K6-2";
	break;
  case 0x59:
	processor = "K6-III";
	break;
  }

  if (processor) {
	char *model = (char *)malloc(strlen(processor) + 1);
	if (model)
	  strcpy(model, processor);
	return model;
  }

  return NULL;
}

// Get Intel processor name
static char *get_model_intel(struct cpuinfo *cip)
{
  // assume we are a valid Intel processor
  uint32_t cpuid_level;
  cpuid(0, &cpuid_level, NULL, NULL, NULL);
  if (cpuid_level < 1)
	return NULL;

  uint32_t eax, ebx;
  cpuid(1, &eax, &ebx, NULL, NULL);
  const char *processor = NULL;

  // check Brand ID
  uint32_t fms = eax & 0xfff;
  uint32_t brand_id = ebx & 0xff;
  if (brand_id) {
	// AP485, Table 5-1
	switch (brand_id) {
	case 0x01: processor = "Celeron";											break;
	case 0x02: processor = "Pentium III";										break;
	case 0x03: processor = fms == 0x6b1 ? "Celeron" : "Pentium III Xeon";		break;
	case 0x04: processor = "Pentium III";										break;
	case 0x06: processor = "Mobile Pentium III";								break;
	case 0x07: processor = "Mobile Celeron";									break;
	case 0x08: processor = fms >= 0xf13 ? "Genuine" : "Pentium 4";				break;
	case 0x09: processor = "Pentium 4";											break;
	case 0x0a: processor = "Celeron";											break;
	case 0x0b: processor = fms < 0xf13 ? "Xeon MP" : "Xeon";					break;
	case 0x0c: processor = "Xeon MP";											break;
	case 0x0e: processor = fms < 0xf13 ? "Xeon" : "Mobile Pentium 4";			break;
	case 0x0f: processor = "Mobile Celeron";									break;
	case 0x11: processor = "Mobile Genuine";									break;
	case 0x12: processor = "Celeron M";											break;
	case 0x13: processor = "Mobile Celeron";									break;
	case 0x14: processor = "Celeron";											break;
	case 0x15: processor = "Mobile Genuine";									break;
	case 0x16: processor = "Pentium M";											break;
	case 0x17: processor = "Mobile Celeron";									break;
	}
  }

  if (processor) {
	char *model = (char *)malloc(strlen(processor) + 1);
	if (model)
	  strcpy(model, processor);
	return model;
  }

  return NULL;
}

// Get Centaur processor name
static char *get_model_centaur(struct cpuinfo *cip)
{
  // assume we are a valid Centaur processor
  uint32_t eax;
  cpuid(1, &eax, NULL, NULL, NULL);

  const char *processor = NULL;
  switch ((eax >> 4) & 0xff) {
  case 0x66:
	processor = "VIA C3 [Samuel]";
	break;
  case 0x67:
	if (eax & 8)
	  processor = "VIA C3 [Ezra]";
	else
	  processor = "VIA C3 [Samuel 2]";
	break;
  case 0x68:
	processor = "VIA C3 [Ezra-T]";
	break;
  case 0x69:
	processor = "VIA C3 [Nehemiah]";
	break;
  case 0x6a:
	if (eax & 8)
	  processor = "VIA C7-M [Esther]";
	else
	  processor = "VIA C7 [Esther]";
	break;
  }

  if (processor) {
	char *model = (char *)malloc(strlen(processor) + 1);
	if (model)
	  strcpy(model, processor);
	return model;
  }

  return NULL;
}

// Sanitize BrandID string
// FIXME: better go through the troubles of decoding tables to have clean names
static const char *goto_next_block(const char *cp)
{
  while (*cp && *cp != ' ' && *cp != '(')
	++cp;
  return cp;
}

static const char *skip_blanks(const char *cp)
{
  while (*cp && *cp == ' ')
	++cp;
  return cp;
}

static const char *skip_tokens(const char *cp)
{
  static const char *skip_list[] = {
	"AMD", "Intel",				// processor vendors
	"(TM)", "(R)", "(tm)",		// copyright marks
	"CPU", "Processor", "@",	// superfluous tags
	"Dual-Core", "Genuine",
	NULL
  };
  int i;
  for (i = 0; skip_list[i] != NULL; i++) {
	int len = strlen(skip_list[i]);
	if (strncmp(cp, skip_list[i], len) == 0)
	  return cp + len;
  }
  return cp;
}

static int freq_string(const char *cp, const char *ep)
{
  const char *bp = cp;
  while (cp < ep && (*cp == '.' || isdigit(*cp)))
	++cp;
  return cp != bp && cp + 2 < ep
	&& (cp[0] == 'M' || cp[0] == 'G') && cp[1] == 'H' && cp[2] == 'z';
}

static char *sanitize_brand_string(const char *str)
{
  char *model = (char *)malloc(64);
  if (model == NULL)
	return NULL;
  const char *cp;
  char *mp = model;
  cp = skip_tokens(skip_tokens(skip_blanks(str))); // skip Vendor(TM)
  do {
	const char *op = cp;
	const char *ep = cp;
	while ((ep = skip_tokens(skip_blanks(ep))) != op)
	  op = ep;
	cp = skip_blanks(ep);
	ep = goto_next_block(cp);
	if (!freq_string(cp, ep)) {
	  if (mp != model)
		*mp++ = ' ';
	  strncpy(mp, cp, ep - cp);
	  mp += ep - cp;
	}
	cp = ep;
  } while (*cp != 0);
  *mp = '\0';
  if (mp == model) {
	free(model);
	model = NULL;
  }
  return model;
}

// Get processor name
char *cpuinfo_arch_get_model(struct cpuinfo *cip)
{
  char *model = NULL;

  switch (cpuinfo_get_vendor(cip)) {
  case CPUINFO_VENDOR_AMD:
	model = get_model_amd(cip);
	break;
  case CPUINFO_VENDOR_INTEL:
	// XXX proper identification sequence implies 0x80000004 first if supported
	model = get_model_intel(cip);
	break;
  case CPUINFO_VENDOR_CENTAUR:
	model = get_model_centaur(cip);
	break;
  }

  if (model == NULL) {
	uint32_t cpuid_level;
	cpuid(0x80000000, &cpuid_level, NULL, NULL, NULL);
	if ((cpuid_level & 0xffff0000) == 0x80000000 && cpuid_level >= 0x80000004) {
	  D(bug("cpuinfo_get_model: cpuid(0x80000002)\n"));
	  union { uint32_t r[13]; char str[52]; } m = { { 0, } };
	  cpuid(0x80000002, &m.r[0], &m.r[1], &m.r[2], &m.r[3]);
	  cpuid(0x80000003, &m.r[4], &m.r[5], &m.r[6], &m.r[7]);
	  cpuid(0x80000004, &m.r[8], &m.r[9], &m.r[10], &m.r[11]);
	  model = sanitize_brand_string(m.str);
	}
  }

  return model;
}

// Get processor ticks
static inline uint64_t get_ticks(void)
{
  uint32_t low, high;
  __asm__ __volatile__ ("rdtsc" : "=a" (low), "=d" (high));
  return (((uint64_t)high) << 32) | low;
}

// Get current value of microsecond timer
static inline uint64_t get_ticks_usec(void)
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return ((uint64_t)tv.tv_sec * 1000000) + tv.tv_usec;
}

// Try to get CPU frequency from other OS-dependent means
static int os_get_frequency(void)
{
  int freq = 0;

#if defined __linux__
  FILE *proc_file = fopen("/proc/cpuinfo", "r");
  if (proc_file) {
	char line[256];
	while(fgets(line, sizeof(line), proc_file)) {
	  // Read line
	  int len = strlen(line);
	  if (len == 0)
		continue;
	  line[len-1] = 0;

	  // Parse line
	  float f;
	  if (sscanf(line, "cpu MHz : %f", &f) == 1)
		freq = (int)f;
	}
	fclose(proc_file);
  }
#endif

  return freq;
}

// Get processor frequency in MHz
int cpuinfo_arch_get_frequency(struct cpuinfo *cip)
{
  uint64_t start, stop;
  uint64_t ticks_start, ticks_stop;

  // Make sure TSC is available
  uint32_t edx;
  cpuid(1, NULL, NULL, NULL, &edx);
  if ((edx & (1 << 4)) == 0)
	return os_get_frequency();

  start = get_ticks_usec();
  ticks_start = get_ticks();
  while ((get_ticks_usec() - start) < 50000) {
	static unsigned long next = 1;
	next = next * 1103515245 + 12345;
  }
  ticks_stop = get_ticks();
  stop = get_ticks_usec();

  uint64_t freq = (ticks_stop - ticks_start) / (stop - start);
  return ((freq % 10) >= 5) ? (((freq / 10) * 10) + 10) : ((freq / 10) * 10);
}

// Get processor socket ID
static int cpuinfo_get_socket_amd(void)
{
  int socket = -1;

  uint32_t eax;
  cpuid(1, &eax, NULL, NULL, NULL);
  if ((eax & 0xfff0ff00) == 0x00000f00) {	// AMD K8
	// Factored from AMD Revision Guide, rev 3.59
	switch ((eax >> 4) & 0xf) {
	case 0x4: case 0x8: case 0xc:
	  socket = CPUINFO_SOCKET_754;
	  break;
	case 0x3: case 0x7: case 0xb: case 0xf:
	  socket = CPUINFO_SOCKET_939;
	  break;
	case 0x1: case 0x5:
	  socket = CPUINFO_SOCKET_940;
	  break;
	default:
	  D(bug("K8 cpuid(1) => %08x\n", eax));
	  break;
	}
	if ((eax & 0xfffcff00) == 0x00040f00) {
	  // AMD NPT Family 0Fh (Orleans/Manila)
	  cpuid(0x80000001, &eax, NULL, NULL, NULL);
	  switch ((eax >> 4) & 3) {
	  case 0:
		socket = CPUINFO_SOCKET_S1;
		break;
	  case 1:
		socket = CPUINFO_SOCKET_F;
		break;
	  case 3:
		socket = CPUINFO_SOCKET_AM2;
		break;
	  }
	}
  }

  return socket;
}

int cpuinfo_arch_get_socket(struct cpuinfo *cip)
{
  int socket = -1;

  if (cpuinfo_get_vendor(cip) == CPUINFO_VENDOR_AMD)
	socket = cpuinfo_get_socket_amd();

  return socket;
}

// Get number of cores per CPU package
int cpuinfo_arch_get_cores(struct cpuinfo *cip)
{
  uint32_t eax, ecx;

  /* Intel Dual Core characterisation */
  if (cpuinfo_get_vendor(cip) == CPUINFO_VENDOR_INTEL) {
	cpuid(0, &eax, NULL, NULL, NULL);
	if (eax >= 4) {
	  ecx = 0;
	  cpuid(4, &eax, NULL, &ecx, NULL);
	  return 1 + ((eax >> 26) & 0x3f);
	}
  }

  /* AMD Dual Core characterisation */
  else if (cpuinfo_get_vendor(cip) == CPUINFO_VENDOR_AMD) {
	cpuid(0x80000000, &eax, NULL, NULL, NULL);
	if (eax >= 0x80000008) {
	  cpuid(0x80000008, NULL, NULL, &ecx, NULL);
	  return 1 + (ecx & 0xff);
	}
  }

  return 1;
}

// Get number of threads per CPU core
int cpuinfo_arch_get_threads(struct cpuinfo *cip)
{
  uint32_t eax, ebx, edx;

  switch (cpuinfo_get_vendor(cip)) {
  case CPUINFO_VENDOR_INTEL:
	/* Check for Hyper Threading Technology activated */
	/* See "Intel Processor Identification and the CPUID Instruction" (3.3 Feature Flags) */
	cpuid(0, &eax, NULL, NULL, NULL);
	if (eax >= 1) {
	  cpuid(1, NULL, &ebx, NULL, &edx);
	  if (edx & (1 << 28)) { /* HTT flag */
		int n_cores = cpuinfo_get_cores(cip);
		assert(n_cores > 0);
		return ((ebx >> 16) & 0xff) / n_cores;
	  }
	}
	break;
  }

  return 1;
}

// Get cache information (initialize with iter = 0, returns the
// iteration number or -1 if no more information available)
// Reference: Application Note 485 -- Intel Processor Identification
static const struct {
  uint8_t desc;
  uint8_t level;
  uint8_t type;
  uint16_t size;
}
intel_cache_table[] = {
#define C_(LEVEL, TYPE) LEVEL, CPUINFO_CACHE_TYPE_##TYPE
  { 0x06, C_(1, CODE),		    8 }, // 4-way set assoc, 32 byte line size
  { 0x08, C_(1, CODE),		   16 }, // 4-way set assoc, 32 byte line size
  { 0x0a, C_(1, DATA),		    8 }, // 2 way set assoc, 32 byte line size
  { 0x0c, C_(1, DATA),		   16 }, // 4-way set assoc, 32 byte line size
  { 0x10, C_(1, DATA),		   16 }, // 4-way set assoc, 64 byte line size
  { 0x15, C_(1, CODE),		   16 }, // 4-way set assoc, 64 byte line size
  { 0x1a, C_(2, UNIFIED),	   96 }, // 6-way set assoc, 64 byte line size
  { 0x22, C_(3, UNIFIED),	  512 }, // 4-way set assoc, sectored cache, 64 byte line size
  { 0x23, C_(3, UNIFIED),	 1024 }, // 8-way set assoc, sectored cache, 64 byte line size
  { 0x25, C_(3, UNIFIED),	 2048 }, // 8-way set assoc, sectored cache, 64 byte line size
  { 0x29, C_(3, UNIFIED),	 4096 }, // 8-way set assoc, sectored cache, 64 byte line size
  { 0x2c, C_(1, DATA),		   32 }, // 8-way set assoc, 64 byte line size
  { 0x30, C_(1, CODE),		   32 }, // 8-way set assoc, 64 byte line size
  { 0x39, C_(2, UNIFIED),	  128 }, // 4-way set assoc, sectored cache, 64 byte line size
  { 0x3a, C_(2, UNIFIED),	  192 }, // 6-way set assoc, sectored cache, 64 byte line size
  { 0x3b, C_(2, UNIFIED),	  128 }, // 2-way set assoc, sectored cache, 64 byte line size
  { 0x3c, C_(2, UNIFIED),	  256 }, // 4-way set assoc, sectored cache, 64 byte line size
  { 0x3d, C_(2, UNIFIED),	  384 }, // 6-way set assoc, sectored cache, 64 byte line size
  { 0x3e, C_(2, UNIFIED),	  512 }, // 4-way set assoc, sectored cache, 64 byte line size
  { 0x41, C_(2, UNIFIED),	  128 }, // 4-way set assoc, 32 byte line size
  { 0x42, C_(2, UNIFIED),	  256 }, // 4-way set assoc, 32 byte line size
  { 0x43, C_(2, UNIFIED),	  512 }, // 4-way set assoc, 32 byte line size
  { 0x44, C_(2, UNIFIED),	 1024 }, // 4-way set assoc, 32 byte line size
  { 0x45, C_(2, UNIFIED),	 2048 }, // 4-way set assoc, 32 byte line size
  { 0x46, C_(3, UNIFIED),	 4096 }, // 4-way set assoc, 64 byte line size
  { 0x47, C_(3, UNIFIED),	 8192 }, // 8-way set assoc, 64 byte line size
  { 0x49, C_(3, UNIFIED),	 4096 }, // 16-way set assoc, 64 byte line size
  { 0x4a, C_(3, UNIFIED),	 6144 }, // 12-way set assoc, 64 byte line size
  { 0x4b, C_(3, UNIFIED),	 8192 }, // 16-way set assoc, 64 byte line size
  { 0x4c, C_(3, UNIFIED),	12288 }, // 12-way set assoc, 64 byte line size
  { 0x4d, C_(3, UNIFIED),	16384 }, // 16-way set assoc, 64 byte line size
  { 0x60, C_(1, DATA),		   16 }, // 8-way set assoc, sectored cache, 64 byte line size
  { 0x66, C_(1, DATA),		    8 }, // 4-way set assoc, sectored cache, 64 byte line size
  { 0x67, C_(1, DATA),		   16 }, // 4-way set assoc, sectored cache, 64 byte line size
  { 0x68, C_(1, DATA),		   32 }, // 4-way set assoc, sectored cache, 64 byte line size
  { 0x70, C_(0, TRACE),		   12 }, // 8-way set assoc
  { 0x71, C_(0, TRACE),		   16 }, // 8-way set assoc
  { 0x72, C_(0, TRACE),		   32 }, // 8-way set assoc
  { 0x73, C_(0, TRACE),		   64 }, // 8-way set assoc
  { 0x77, C_(1, CODE),		   16 }, // 4-way set assoc, sectored cache, 64 byte line size
  { 0x78, C_(2, UNIFIED),	 1024 }, // 4-way set assoc, 64 byte line size
  { 0x79, C_(2, UNIFIED),	  128 }, // 8-way set assoc, sectored cache, 64 byte line size
  { 0x7a, C_(2, UNIFIED),	  256 }, // 8-way set assoc, sectored cache, 64 byte line size
  { 0x7b, C_(2, UNIFIED),	  512 }, // 8-way set assoc, sectored cache, 64 byte line size
  { 0x7c, C_(2, UNIFIED),	 1024 }, // 8-way set assoc, sectored cache, 64 byte line size
  { 0x7e, C_(2, UNIFIED),	  256 }, // 8-way set assoc, sectored cache, 128 byte line size
  { 0x7d, C_(2, UNIFIED),	 2048 }, // 8-way set assoc, 64 byte line size
  { 0x7f, C_(2, UNIFIED),	  512 }, // 2-way set assoc, 64 byte line size
  { 0x82, C_(2, UNIFIED),	  256 }, // 8-way set assoc, 32 byte line size
  { 0x83, C_(2, UNIFIED),	  512 }, // 8-way set assoc, 32 byte line size
  { 0x84, C_(2, UNIFIED),	 1024 }, // 8-way set assoc, 32 byte line size
  { 0x85, C_(2, UNIFIED),	 2048 }, // 8-way set assoc, 32 byte line size
  { 0x86, C_(2, UNIFIED),	  512 }, // 4-way set assoc, 64 byte line size
  { 0x87, C_(2, UNIFIED),	 1024 }, // 8-way set assoc, 64 byte line size
  { 0x88, C_(3, UNIFIED),	 2048 }, // 4-way set assoc, 64 byte line size
  { 0x89, C_(3, UNIFIED),	 4096 }, // 4-way set assoc, 64 byte line size
  { 0x8a, C_(3, UNIFIED),	 8192 }, // 4-way set assoc, 64 byte line size
  { 0x8d, C_(3, UNIFIED),	 3072 }, // 12-way set assoc, 128 byte line size
  { 0x00, C_(0, UNKNOWN),	    0 }
#undef C_
};

enum {
  CACHE_INFO_ERRATA_AMD_DURON = 1,	 // AMD K7 processors with CPUID=630h (Duron)
  CACHE_INFO_ERRATA_VIA_C3_1,		 // VIA C3 processors with CPUID=670..68Fh
  CACHE_INFO_ERRATA_VIA_C3_2,		 // VIA C3 processors with CPUID=691h ('Nehemiah' stepping 1)
};

static int has_cache_info_errata_amd(struct cpuinfo *cip, int errata)
{
  // XXX store F/M/S fields in x86_cpuinfo_t
  uint32_t eax;
  cpuid(1, &eax, NULL, NULL, NULL);
  if ((eax & 0xfff) == 0x630) {
	if (errata == CACHE_INFO_ERRATA_AMD_DURON) {
	  D(bug("cpuinfo_get_cache: errata for AMD K7 processors with CPUID=630h (Duron)\n"));
	  return 1;
	}
  }
  return 0;
}

static int has_cache_info_errata_centaur(struct cpuinfo *cip, int errata)
{
  // XXX store F/M/S fields in x86_cpuinfo_t
  uint32_t eax;
  cpuid(1, &eax, NULL, NULL, NULL);
  switch ((eax >> 4) & 0xff) {
  case 0x67:
  case 0x68:
	if (errata == CACHE_INFO_ERRATA_VIA_C3_1) {
	  D(bug("cpuinfo_get_cache: errata for VIA C3 processors with CPUID=670..68Fh\n"));
	  return 1;
	}
	break;
  case 0x69:
	if ((eax & 0xf) == 1) {
	  if (errata == CACHE_INFO_ERRATA_VIA_C3_2) {
		D(bug("cpuinfo_get_cache: errata for VIA C3 processors with CPUID=691h ('Nehemiah' stepping 1)\n"));
		return 1;
	  }
	}
	break;
  }
  return 0;
}

static inline int has_cache_info_errata(struct cpuinfo *cip, int errata)
{
  switch (cpuinfo_get_vendor(cip)) {
  case CPUINFO_VENDOR_AMD:
	return has_cache_info_errata_amd(cip, errata);
  case CPUINFO_VENDOR_CENTAUR:
	return has_cache_info_errata_centaur(cip, errata);
  }
  return 0;
}

cpuinfo_list_t cpuinfo_arch_get_caches(struct cpuinfo *cip)
{
  uint32_t cpuid_level;
  cpuid(0, &cpuid_level, NULL, NULL, NULL);

  cpuinfo_list_t caches_list = NULL;
  cpuinfo_cache_descriptor_t cache_desc;

  if (cpuid_level >= 4) {
	// XXX not MP safe cpuid()
	D(bug("cpuinfo_get_cache: cpuid(4)\n"));
	uint32_t eax, ebx, ecx, edx;
	int count = 0;
	int saw_L1I_cache = 0;
	for (;;) {
	  ecx = count;
	  cpuid(4, &eax, &ebx, &ecx, &edx);
	  int cache_type = eax & 0x1f;
	  if (cache_type == 0)
		break;
	  switch (cache_type) {
	  case 1: cache_type = CPUINFO_CACHE_TYPE_DATA; break;
	  case 2: cache_type = CPUINFO_CACHE_TYPE_CODE; break;
	  case 3: cache_type = CPUINFO_CACHE_TYPE_UNIFIED; break;
	  default: cache_type = CPUINFO_CACHE_TYPE_UNKNOWN; break;
	  }
	  cache_desc.type = cache_type;
	  cache_desc.level = (eax >> 5) & 7;
	  uint32_t W = 1 + ((ebx >> 22) & 0x3f);	// ways of associativity
	  uint32_t P = 1 + ((ebx >> 12) & 0x1f);	// physical line partition
	  uint32_t L = 1 + (ebx & 0xfff);			// system coherency line size
	  uint32_t S = 1 + ecx;						// number of sets
	  cache_desc.size = (L * W * P * S) / 1024;
	  cpuinfo_caches_list_insert(&cache_desc);
	  ++count;
	  if (cache_desc.type == CPUINFO_CACHE_TYPE_CODE && cache_desc.level == 1)
		saw_L1I_cache = 1;
	}
	/* XXX find a better way to detect 'Instruction Trace Cache'-based processors? */
	if (saw_L1I_cache)
	  return caches_list;
	cpuinfo_list_clear(&caches_list);
  }

  if (cpuid_level >= 2) {
	int i, j, k, n;
	uint32_t regs[4];
	uint8_t *dp = (uint8_t *)regs;
	D(bug("cpuinfo_get_cache: cpuid(2)\n"));
	cpuid(2, &regs[0], NULL, NULL, NULL);
	n = regs[0] & 0xff;						// number of times to iterate
	for (i = 0; i < n; i++) {
	  cpuid(2, &regs[0], &regs[1], &regs[2], &regs[3]);
	  for (j = 0; j < 4; j++) {
		if (regs[j] & 0x80000000)
		  regs[j] = 0;
	  }
	  for (j = 1; j < 16; j++) {
		uint8_t desc = dp[j];
		for (k = 0; intel_cache_table[k].desc != 0; k++) {
		  if (intel_cache_table[k].desc == desc) {
			cache_desc.type = intel_cache_table[k].type;
			cache_desc.level = intel_cache_table[k].level;
			cache_desc.size = intel_cache_table[k].size;
			cpuinfo_caches_list_insert(&cache_desc);
			D(bug("%02x\n", desc));
			break;
		  }
		}
	  }
	}
	return caches_list;
  }

  cpuid(0x80000000, &cpuid_level, NULL, NULL, NULL);
  if ((cpuid_level & 0xffff0000) == 0x80000000 && cpuid_level >= 0x80000005) {
	uint32_t ecx, edx;
	D(bug("cpuinfo_get_cache: cpuid(0x80000005)\n"));
	cpuid(0x80000005, NULL, NULL, &ecx, &edx);
	cache_desc.level = 1;
	cache_desc.type = CPUINFO_CACHE_TYPE_CODE;
	cache_desc.size = (edx >> 24) & 0xff;
	cpuinfo_caches_list_insert(&cache_desc);
	cache_desc.level = 1;
	cache_desc.type = CPUINFO_CACHE_TYPE_DATA;
	cache_desc.size = (ecx >> 24) & 0xff;
	cpuinfo_caches_list_insert(&cache_desc);
	if (cpuid_level >= 0x80000006) {
	  D(bug("cpuinfo_get_cache: cpuid(0x80000006)\n"));
	  cpuid(0x80000006, NULL, NULL, &ecx, NULL);
	  if (has_cache_info_errata(cip, CACHE_INFO_ERRATA_VIA_C3_1)) {
		if (((ecx >> 16) & 0xffff) != 0) {
		  cache_desc.level = 2;
		  cache_desc.type = CPUINFO_CACHE_TYPE_UNIFIED;
		  cache_desc.size = (ecx >> 24) & 0xff;
		  cpuinfo_caches_list_insert(&cache_desc);
		}
	  }
	  else {
		if (((ecx >> 12) & 0xfffff) != 0) {
		  cache_desc.level = 2;
		  cache_desc.type = CPUINFO_CACHE_TYPE_UNIFIED;
		  cache_desc.size = (ecx >> 16) & 0xffff;
		  if (has_cache_info_errata(cip, CACHE_INFO_ERRATA_AMD_DURON))
			cache_desc.size = 64;
		  else if (has_cache_info_errata(cip, CACHE_INFO_ERRATA_VIA_C3_2)) {
			if (cache_desc.size == 65)
			  cache_desc.size = 64;
		  }
		  cpuinfo_caches_list_insert(&cache_desc);
		}
	  }
	}
	return caches_list;
  }

  return NULL;
}

static int bsf_clobbers_eflags(void)
{
  int mismatch = 0;
  int g_ZF, g_CF, g_OF, g_SF, value;
  for (g_ZF = 0; g_ZF <= 1; g_ZF++) {
	for (g_CF = 0; g_CF <= 1; g_CF++) {
	  for (g_OF = 0; g_OF <= 1; g_OF++) {
		for (g_SF = 0; g_SF <= 1; g_SF++) {
		  for (value = -1; value <= 1; value++) {
			unsigned long flags = (g_SF << 7) | (g_OF << 11) | (g_ZF << 6) | g_CF;
			unsigned long tmp = value;
			__asm__ __volatile__ ("push %0; popf; bsf %1,%1; pushf; pop %0"
								  : "+r" (flags), "+r" (tmp) : : "cc");
			int OF = (flags >> 11) & 1;
			int SF = (flags >>  7) & 1;
			int ZF = (flags >>  6) & 1;
			int CF = flags & 1;
			int r_ZF = (value == 0);
			if (ZF != r_ZF || SF != g_SF || OF != g_OF || CF != g_CF)
				mismatch = 1;
		  }
		}
	  }
	}
  }
  return mismatch;
}

// Returns features table
uint32_t *cpuinfo_arch_feature_table(struct cpuinfo *cip, int feature)
{
  switch (feature & CPUINFO_FEATURE_ARCH) {
  case CPUINFO_FEATURE_COMMON:
	return cip->features;
  case CPUINFO_FEATURE_X86:
	return ((x86_cpuinfo_t *)(cip->opaque))->features;
  }
  return NULL;
}

#define feature_get_bit(NAME) cpuinfo_feature_get_bit(cip, CPUINFO_FEATURE_X86_##NAME)
#define feature_set_bit(NAME) cpuinfo_feature_set_bit(cip, CPUINFO_FEATURE_X86_##NAME)

// Returns 1 if CPU supports the specified feature
int cpuinfo_arch_has_feature(struct cpuinfo *cip, int feature)
{
  if (!cpuinfo_feature_get_bit(cip, CPUINFO_FEATURE_X86)) {
#if defined __linux__
	static struct utsname un;
#endif
	cpuinfo_feature_set_bit(cip, CPUINFO_FEATURE_X86);
	if(cpuinfo_has_ac())
	    feature_set_bit(AC);
	if(cpuinfo_has_cpuid()) {
	    feature_set_bit(CPUID);
    
	    uint32_t eax = 0, ecx = 0, edx = 0;
	    cpuid(1, NULL, NULL, &ecx, &edx);
	    if (edx & (1 << 0))
		feature_set_bit(FPU);
	    if (edx & (1 << 1))
		feature_set_bit(VME);
	    if (edx & (1 << 2))
		feature_set_bit(DE);
	    if (edx & (1 << 3))
		feature_set_bit(PSE);
	    if (edx & (1 << 4))
		feature_set_bit(TSC);
	    if (edx & (1 << 5))
		feature_set_bit(MSR);
	    if (edx & (1 << 6))
		feature_set_bit(PAE);
	    if (edx & (1 << 7))
		feature_set_bit(MCE);
	    if (edx & (1 << 8))
		feature_set_bit(CX8);
	    if (edx & (1 << 9))
		feature_set_bit(APIC);
	    if (edx & (1 << 11))
		feature_set_bit(SEP);
	    if (edx & (1 << 12))
		feature_set_bit(MTRR);
	    if (edx & (1 << 13))
		feature_set_bit(PGE);
	    if (edx & (1 << 14))
		feature_set_bit(MCA);
	    if (edx & (1 << 15))
		feature_set_bit(CMOV);
	    if (edx & (1 << 16))
		feature_set_bit(PAT);
	    if (edx & (1 << 17))
		feature_set_bit(PSE_36);
	    if (edx & (1 << 18))
		feature_set_bit(PSN);
	    if (edx & (1 << 19))
		feature_set_bit(CLFLUSH);
	    if (edx & (1 << 21))
		feature_set_bit(DS);
	    if (edx & (1 << 22))
		feature_set_bit(ACPI);
	    if (edx & (1 << 23))
		feature_set_bit(MMX);
	    if (edx & (1 << 24))
		feature_set_bit(FXSR);
	    if (edx & (1 << 25))
		feature_set_bit(SSE);
	    if (edx & (1 << 26))
		feature_set_bit(SSE2);
	    if (edx & (1 << 27))
		feature_set_bit(SS);
	    if (edx & (1 << 28))
		feature_set_bit(HTT);	
	    if (edx & (1 << 29))
		feature_set_bit(TM);
	    if (edx & (1 << 30))
		feature_set_bit(IA64);
	    if (edx & (1 << 31))
		feature_set_bit(PBE);	

	    if (ecx & (1 << 0))
		feature_set_bit(SSE3);
	    if (ecx & (1 << 1))
		feature_set_bit(PCLMULQDQ);
	    if (ecx & (1 << 2))
		feature_set_bit(DTES64);
	    if (ecx & (1 << 3))
		feature_set_bit(MONITOR);
	    if (ecx & (1 << 4))
		feature_set_bit(DS_CPL);
	    if (ecx & (1 << 5))
		feature_set_bit(VMX);
	    if (ecx & (1 << 6))
		feature_set_bit(SMX);
	    if (ecx & (1 << 7))
		feature_set_bit(EIST);
	    if (ecx & (1 << 8))
		feature_set_bit(TM2);
	    if (ecx & (1 << 9))
		feature_set_bit(SSSE3);
	    if (ecx & (1 << 10))
		feature_set_bit(CNXT_ID);
	    if (ecx & (1 << 12))
		feature_set_bit(FMA);
	    if (ecx & (1 << 13))
		feature_set_bit(CX16);
	    if (ecx & (1 << 14))
		feature_set_bit(XTPR);
	    if (ecx & (1 << 15))
		feature_set_bit(PDCM);
	    if (ecx & (1 << 17))
		feature_set_bit(PCID);
	    if (ecx & (1 << 18))
		feature_set_bit(DCA);
	    if (ecx & (1 << 19))
		feature_set_bit(SSE4_1);
	    if (ecx & (1 << 20))
		feature_set_bit(SSE4_2);
	    if (ecx & (1 << 21))
		feature_set_bit(X2APIC);
	    if (ecx & (1 << 22))
		feature_set_bit(MOVBE);
	    if (ecx & (1 << 23))
		feature_set_bit(POPCNT);
	    if (ecx & (1 << 24))
		feature_set_bit(TSC_DEADLINE);
	    if (ecx & (1 << 25))
		feature_set_bit(AES);
	    if (ecx & (1 << 26))
		feature_set_bit(XSAVE);
	    if (ecx & (1 << 27))
		feature_set_bit(OSXSAVE);
	    if (ecx & (1 << 28))
		feature_set_bit(AVX);
	    if (ecx & (1 << 29))
		feature_set_bit(F16C);
	    if (ecx & (1 << 31))
		feature_set_bit(HYPERVISOR);

	    cpuid(0x80000000, &eax, NULL, NULL, NULL);
	    if ((eax & 0xffff0000) == 0x80000000 && eax >= 0x80000001) {
		cpuid(0x80000001, NULL, NULL, &ecx, &edx);
		if (ecx & (1 << 0))
		    feature_set_bit(LAHF64);
		if (ecx & (1 << 1))
		    feature_set_bit(CMP_LEGACY);
		if (ecx & (1 << 2))
		    feature_set_bit(SVM);
		if (ecx & (1 << 3))
		    feature_set_bit(EXTAPIC);
		if (ecx & (1 << 4))
		    feature_set_bit(CR8_LEGACY);
		if (ecx & (1 << 5))
		    feature_set_bit(ABM);
		if (ecx & (1 << 6))
		    feature_set_bit(SSE4A);
		if (ecx & (1 << 7))
		    feature_set_bit(MISALIGNSSE);
		if (ecx & (1 << 8))
		    feature_set_bit(3DNOW_PREFETCH);
		if (ecx & (1 << 9))
		    feature_set_bit(OSVW);
		if (ecx & (1 << 10))
		    feature_set_bit(IBS);
		if (ecx & (1 << 11))
		    feature_set_bit(SSE5);
		if (ecx & (1 << 12))
		    feature_set_bit(SKINIT);
		if (ecx & (1 << 13))
		    feature_set_bit(WDT);
		if (ecx & (1 << 15))
		    feature_set_bit(LWP);
		if (ecx & (1 << 16))
		    feature_set_bit(FMA4);
		if (ecx & (1 << 19))
		    feature_set_bit(NODEID_MSR);
		if (ecx & (1 << 21))
		    feature_set_bit(TBM);
		if (ecx & (1 << 22))
		    feature_set_bit(TOPOEXT);

		if (edx & (1 << 20))
		    feature_set_bit(NX);
		// XXX not sure they are the same MMX extensions...
		if (cpuinfo_get_vendor(cip) == CPUINFO_VENDOR_AMD && (edx & (1 << 22)))
		    feature_set_bit(MMX_EXT);
		if (cpuinfo_get_vendor(cip) == CPUINFO_VENDOR_CYRIX && (edx & (1 << 24)))
		    feature_set_bit(MMX_EXT);
		if (edx & (1 << 25))
		    feature_set_bit(FFXSR);
		if (edx & (1 << 26))
		    feature_set_bit(PAGE1GB);		
		if (edx & (1 << 27))
		    feature_set_bit(RDTSCP);
		if (edx & (1 << 29))
		    feature_set_bit(LM);
		if (edx & (1 << 30))
		    feature_set_bit(3DNOW_EXT);
		if (edx & (1 << 31))
		    feature_set_bit(3DNOW);
	    }
	}

	if (bsf_clobbers_eflags())
	  feature_set_bit(BSFCC);

	/*
	 * FIXME: checking size_t size is lame and will be dependent on build,
	 * 	  is there any way to check EFER.LMA from userspace perhaps?
	 */
#if defined __linux__
	uname(&un);
	/* don't really know what identifier is used on other platforms.. */
	if (feature_get_bit(LM) && !strcmp(un.machine, "x86_64"))
#else
	if (feature_get_bit(LM)	&& sizeof(size_t) == 8)
#endif
	  cpuinfo_feature_set_bit(cip, CPUINFO_FEATURE_64BIT);

	if (feature_get_bit(MMX) ||
		feature_get_bit(MMX_EXT) ||
		feature_get_bit(3DNOW) ||
		feature_get_bit(3DNOW_EXT) ||
		feature_get_bit(SSE) ||
		feature_get_bit(SSE2) ||
		feature_get_bit(SSE3) ||
		feature_get_bit(SSSE3) ||
		feature_get_bit(SSE4A) ||
		feature_get_bit(SSE4_1) ||
		feature_get_bit(SSE4_2) ||
		feature_get_bit(SSE5))
	  cpuinfo_feature_set_bit(cip, CPUINFO_FEATURE_SIMD);

	if (feature_get_bit(POPCNT))
	  cpuinfo_feature_set_bit(cip, CPUINFO_FEATURE_POPCOUNT);
  }

  return cpuinfo_feature_get_bit(cip, feature);
}
