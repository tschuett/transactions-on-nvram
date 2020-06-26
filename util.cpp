#include "util.h"

#include <cstdlib>
#include <execinfo.h>

#include <map>
#include <string>

namespace {
std::map<std::string, bool> Features;

// Read control register 0 (XCR0). Used to detect features such as AVX.
static bool getX86XCR0(unsigned *rEAX, unsigned *rEDX) {
#if defined(__GNUC__) || defined(__clang__)
  // Check xgetbv; this uses a .byte sequence instead of the instruction
  // directly because older assemblers do not include support for xgetbv and
  // there is no easy way to conditionally compile based on the assembler
  // used.
  __asm__(".byte 0x0f, 0x01, 0xd0" : "=a"(*rEAX), "=d"(*rEDX) : "c"(0));
  return false;
#elif defined(_MSC_FULL_VER) && defined(_XCR_XFEATURE_ENABLED_MASK)
  unsigned long long Result = _xgetbv(_XCR_XFEATURE_ENABLED_MASK);
  *rEAX = Result;
  *rEDX = Result >> 32;
  return false;
#else
  return true;
#endif
}
/// getX86CpuIDAndInfo - Execute the specified cpuid and return the 4 values
/// in
/// the specified arguments.  If we can't run cpuid on the host, return true.
static bool getX86CpuIDAndInfo(unsigned value, unsigned *rEAX, unsigned *rEBX,
                               unsigned *rECX, unsigned *rEDX) {
#if defined(__GNUC__) || defined(__clang__)
#if defined(__x86_64__)
  // gcc doesn't know cpuid would clobber ebx/rbx. Preserve it manually.
  // FIXME: should we save this for Clang?
  __asm__("movq\t%%rbx, %%rsi\n\t"
          "cpuid\n\t"
          "xchgq\t%%rbx, %%rsi\n\t"
          : "=a"(*rEAX), "=S"(*rEBX), "=c"(*rECX), "=d"(*rEDX)
          : "a"(value));
  return false;
#elif defined(__i386__)
  __asm__("movl\t%%ebx, %%esi\n\t"
          "cpuid\n\t"
          "xchgl\t%%ebx, %%esi\n\t"
          : "=a"(*rEAX), "=S"(*rEBX), "=c"(*rECX), "=d"(*rEDX)
          : "a"(value));
  return false;
#else
  return true;
#endif
#elif defined(_MSC_VER)
  // The MSVC intrinsic is portable across x86 and x64.
  int registers[4];
  __cpuid(registers, value);
  *rEAX = registers[0];
  *rEBX = registers[1];
  *rECX = registers[2];
  *rEDX = registers[3];
  return false;
#else
  return true;
#endif
}

/// getX86CpuIDAndInfoEx - Execute the specified cpuid with subleaf and return
/// the 4 values in the specified arguments.  If we can't run cpuid on the
/// host, return true.
static bool getX86CpuIDAndInfoEx(unsigned value, unsigned subleaf,
                                 unsigned *rEAX, unsigned *rEBX, unsigned *rECX,
                                 unsigned *rEDX) {
#if defined(__GNUC__) || defined(__clang__)
#if defined(__x86_64__)
  // gcc doesn't know cpuid would clobber ebx/rbx. Preserve it manually.
  // FIXME: should we save this for Clang?
  __asm__("movq\t%%rbx, %%rsi\n\t"
          "cpuid\n\t"
          "xchgq\t%%rbx, %%rsi\n\t"
          : "=a"(*rEAX), "=S"(*rEBX), "=c"(*rECX), "=d"(*rEDX)
          : "a"(value), "c"(subleaf));
  return false;
#elif defined(__i386__)
  __asm__("movl\t%%ebx, %%esi\n\t"
          "cpuid\n\t"
          "xchgl\t%%ebx, %%esi\n\t"
          : "=a"(*rEAX), "=S"(*rEBX), "=c"(*rECX), "=d"(*rEDX)
          : "a"(value), "c"(subleaf));
  return false;
#else
  return true;
#endif
#elif defined(_MSC_VER)
  int registers[4];
  __cpuidex(registers, value, subleaf);
  *rEAX = registers[0];
  *rEBX = registers[1];
  *rECX = registers[2];
  *rEDX = registers[3];
  return false;
#else
  return true;
#endif
}
} // namespace

bool debug = false;

void printCPU() {
  unsigned EAX = 0, EBX = 0, ECX = 0, EDX = 0;

  unsigned MaxLevel;
  union {
    unsigned u[3];
    char c[12];
  } text;

  if (getX86CpuIDAndInfo(0, &MaxLevel, text.u + 0, text.u + 2, text.u + 1) ||
      MaxLevel < 1)
    return;

  unsigned MaxExtLevel;
  getX86CpuIDAndInfo(0x80000000, &MaxExtLevel, &EBX, &ECX, &EDX);

  getX86CpuIDAndInfo(1, &EAX, &EBX, &ECX, &EDX);

  Features["popcnt"] = (ECX >> 23) & 1;

  // If CPUID indicates support for XSAVE, XRESTORE and AVX, and XGETBV
  // indicates that the AVX registers will be saved and restored on context
  // switch, then we have full AVX support.
  bool HasAVXSave = ((ECX >> 27) & 1) && ((ECX >> 28) & 1) &&
                    !getX86XCR0(&EAX, &EDX) && ((EAX & 0x6) == 0x6);
  // AVX512 requires additional context to be saved by the OS.
  bool HasAVX512Save = HasAVXSave && ((EAX & 0xe0) == 0xe0);

  Features["avx"] = HasAVXSave;

  bool HasExtLeaf1 = MaxExtLevel >= 0x80000001 &&
                     !getX86CpuIDAndInfo(0x80000001, &EAX, &EBX, &ECX, &EDX);
  Features["lzcnt"] = HasExtLeaf1 && ((ECX >> 5) & 1);

  bool HasLeaf7 =
      MaxLevel >= 7 && !getX86CpuIDAndInfoEx(0x7, 0x0, &EAX, &EBX, &ECX, &EDX);

  Features[std::string("bmi")] = HasLeaf7 && ((EBX >> 3) & 1);
  Features[std::string("bmi2")] = HasLeaf7 && ((EBX >> 8) & 1);
  Features[std::string("avx2")] = HasLeaf7 && ((EBX >> 5) & 1) && HasAVXSave;
  Features[std::string("pcommit")] = HasLeaf7 && ((EBX >> 22) & 1);
  Features[std::string("clflushopt")] = HasLeaf7 && ((EBX >> 23) & 1);
  Features[std::string("clwb")] = HasLeaf7 && ((EBX >> 24) & 1);
  Features[std::string("avx512f")] =
      HasLeaf7 && ((EBX >> 16) & 1) && HasAVX512Save;
  Features["avx512dq"] = HasLeaf7 && ((EBX >> 17) & 1) && HasAVX512Save;
  Features[std::string("avx512vl")] =
      HasLeaf7 && ((EBX >> 31) & 1) && HasAVX512Save;
  Features[std::string("avx512bw")] =
      HasLeaf7 && ((EBX >> 30) & 1) && HasAVX512Save;
  Features["avx512cd"] = HasLeaf7 && ((EBX >> 28) & 1) && HasAVX512Save;
  Features[std::string("avx512vbmi")] =
      HasLeaf7 && ((ECX >> 1) & 1) && HasAVX512Save;
  Features[std::string("avx512vbmi")] =
      HasLeaf7 && ((ECX >> 1) & 1) && HasAVX512Save;
  Features[std::string("avx512vbmi2")] =
      HasLeaf7 && ((ECX >> 6) & 1) && HasAVX512Save;
  Features[std::string("avx512bitalg")] =
      HasLeaf7 && ((ECX >> 12) & 1) && HasAVX512Save;
  Features[std::string("avx512vpopcntdq")] =
      HasLeaf7 && ((ECX >> 14) & 1) && HasAVX512Save;
  Features[std::string("vpclmulqdq")] =
      HasLeaf7 && ((ECX >> 10) & 1) && HasAVXSave;
  Features["avx512ifma"] = HasLeaf7 && ((EBX >> 21) & 1) && HasAVX512Save;
  Features[std::string("rdpid")] = HasLeaf7 && ((ECX >> 22) & 1);
  Features[std::string("cldemote")] = HasLeaf7 && ((ECX >> 25) & 1);
  Features[std::string("movdiri")] = HasLeaf7 && ((ECX >> 27) & 1);
  Features[std::string("movdir64b")] = HasLeaf7 && ((ECX >> 28) & 1);

  // Miscellaneous memory related features, detected by
  // using the 0x80000008 leaf of the CPUID instruction
  bool HasExtLeaf8 = MaxExtLevel >= 0x80000008 &&
                     !getX86CpuIDAndInfo(0x80000008, &EAX, &EBX, &ECX, &EDX);
  Features["clzero"] = HasExtLeaf8 && ((EBX >> 0) & 1);
  Features["wbnoinvd"] = HasExtLeaf8 && ((EBX >> 9) & 1);

  for (auto &kv : Features)
    if (kv.second)
      printf("%s ", kv.first.c_str());
  printf("%s\n", VERSION);
}

void printStackTrace() {
  const int BT_BUF_SIZE = 100;
  int j, nptrs;
  void *buffer[BT_BUF_SIZE];
  char **strings;

  nptrs = backtrace(buffer, BT_BUF_SIZE);
  printf("backtrace() returned %d addresses\n", nptrs);

  /* The call backtrace_symbols_fd(buffer, nptrs, STDOUT_FILENO)
     would produce similar output to the following: */

  strings = backtrace_symbols(buffer, nptrs);
  if (strings == NULL) {
    perror("backtrace_symbols");
    exit(EXIT_FAILURE);
  }

  for (j = 0; j < nptrs; j++)
    printf("%s\n", strings[j]);

  free(strings);
}
