//
// A virtual memory simulation.
//
// Virtual memory is a sequence of 32-bit words.
//
// Physical memory is a shorter sequence of 32-bit words.
//
// Virtual memory addresses are 32 bits.
//
// Virtual and physical memory is managed in fixed-size pages.
//
// A page table, which is logically in physical memory, maps virtual
// memory addresses to physical memory addresses. Since physical
// memory is smaller than virtual memory, the page table might indicate
// that a particular page is not currently present in physical memory,
// and that it is logically stored on disk.
//
// However this is a simulation, so all memory words are actually stored
// in the memory of this program, and disk is not used. Similarly the page
// table does not actually exist in the simulated physical memory, but
// rather exists only in the data structures of the simulation.
//
// A translation lookaside buffer (TLB) is simulated. The TLB stores
// recent virtual-to-physical address translations.
//
// The following properties of the virtual memory are set when it is
// created:
//   1. size of the virtual memory in pages
//   2. size of the physical memory in pages
//   3. size of a page in words
//   4. number of TLB entries
//   5. page replacement algorithm
//   6. TLB replacement algorithm
//
// The size of the virtual memory must be larger than the size of the
// physical memory.
//
// The size of a page must be a power of two.
//
// The size of the virtual memory times the size of a page must be less
// than or equal to 2^32.
//
// The size of the TLB should be less than or equal to the size of physical
// memory.
//
// The replacement algorithms are either 0 for round-robin replacement or
// 1 for LRU replacement.
//
// A virtual memory system is initialized to have the first K pages
// of virtual memory loaded into physical memory, where K is the
// number of pages in the physical memory. Initially the virtual
// memory system contains arbitrary values (i.e. not necessarily
// zeros).
//
// The TLB is initialized to have the VM to PM mapping for the
// first N pages loaded into physical pages (i.e. starting at physical
// page 0), where N is the number of TLB entries.
//
// The goal of the simulation is to report the number of page misses,
// the number of TLB misses, and the number of disk writes.
//

#define VM_ROUNDROBIN_REPLACEMENT 0
#define VM_LRU_REPLACEMENT 1

// createVM
//
// Create the virtual memory system and return a "handle" for it.
//
// If there is a violation of any of the constraints on the properties
// of the virtual memory system, this function will return NULL.
//
// If this function fails for any other reason (e.g. malloc returns NULL),
// an error message will be printed to stderr and the program will be
// terminated.
//
void *createVM(
  unsigned int sizeVM,   // size of the virtual memory in pages
  unsigned int sizePM,   // size of the physical memory in pages
  unsigned int pageSize, // size of a page in words
  unsigned int sizeTLB,  // number of translation lookaside buffer entries
  char pageReplAlg,      // page replacement alg.: 0 is Round Robin, 1 is LRU
  char tlbReplAlg        // TLB replacement alg.: 0 is Round Robin, 1 is LRU
);

// readInt
//
// Read an int from virtual memory.
//
// If the handle is not one returned by createVM, the behavior is
// undefined.
//
// If the address is out of range, an error message will be printed to
// stderr and the program will be terminated.
//
int readInt(void *handle, unsigned int address);

// readFloat
//
// Read a float from virtual memory.
//
// If the handle is not one returned by createVM, the behavior is
// undefined.
//
// If the address is out of range, an error message will be printed to
// stderr and the program will be terminated.
//
float readFloat(void *handle, unsigned int address);

// writeInt
//
// Write an int to virtual memory.
//
// If the handle is not one returned by createVM, the behavior is
// undefined.
//
// If the address is out of range, an error message will be printed to
// stderr and the program will be terminated.
//
void writeInt(void *handle, unsigned int address, int value);

// writeFloat
//
// Write a float to virtual memory.
//
// If the handle is not one returned by createVM, the behavior is
// undefined.
//
// If the address is out of range, an error message will be printed to
// stderr and the program will be terminated.
//
void writeFloat(void *handle, unsigned int address, float value);

// printStatistics
//
// Print the total number of page faults, the total number of TLB misses
// and the total number of disk writes.
//
// If the handle is not one returned by createVM, the behavior is
// undefined.
//
// Here is a sample output:
//
//   Number of page faults: 123
//   Number of TLB misses: 125
//   Number of disk writes: 64
//
void printStatistics(void *handle);

// cleanupVM
//
// Cleanup the memory used by the simulation of the virtual memory system.
//
// If the handle is not one returned by createVM, the behavior is
// undefined.
//
void cleanupVM(void *handle);

