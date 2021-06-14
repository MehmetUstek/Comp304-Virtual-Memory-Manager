/**
 * virtmem.c 
 */

#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

#define TLB_SIZE 16
#define PAGES 1024
// Page size -1
#define PAGE_MASK 1023
#define FRAMES 256

#define PAGE_SIZE 1024
#define OFFSET_BITS 10
// Page size -1
#define OFFSET_MASK 1023

#define MEMORY_SIZE FRAMES * PAGE_SIZE
#define VIRTUAL_SIZE PAGES * PAGE_SIZE
// Max number of characters per line of input file to read.
#define BUFFER_SIZE 10

struct tlbentry {
  unsigned char logical;
  unsigned char physical;
};

struct tlbentry tlb[TLB_SIZE];
int tlbindex = 0;

int pagetable[PAGES];

signed char main_memory[MEMORY_SIZE];

signed char *backing;


int lru_replacement_policy(int* pageRefTbl, int logical_page, int pageFault) {
  
  if(pageFault <= FRAMES) {
    return pageFault - 1;
  }
  int i;
  int minIndex = 0;
  int minValue = 0;
  for(i = 0; i < PAGES; i++) {
    if(pagetable[i] != -1) {
      if(minValue == 0 || minValue > pageRefTbl[i]) {
        minIndex = i;
        minValue = pageRefTbl[i];
        
      }
    }
  }
return pagetable[minIndex]; 
}

int fifo_replacement_policy(unsigned char *current_page){
	int page = *current_page;
	*current_page = (*current_page + 1) % FRAMES;
	return page;
}
void fromLogicalToPhysicalMemory(int logical_page, int physical_page){
	 memcpy(main_memory + physical_page * PAGE_SIZE, backing + logical_page * PAGE_SIZE, PAGE_SIZE);
	 for(int i = 0; i < PAGES; i++){
	 	if(pagetable[i] == physical_page){
	 		pagetable[i] = -1;
      break;
	 	}
	 }
	 pagetable[logical_page] = physical_page;
}

int max(int a, int b)
{
  if (a > b)
    return a;
  return b;
}
int search_tlb(unsigned char logical_page) {
  int i;
  for (i = max((tlbindex - TLB_SIZE), 0); i < tlbindex; i++)
  {
    struct tlbentry *entry = &tlb[i % TLB_SIZE];

    if (entry->logical == logical_page)
    {
      return entry->physical;
    }
  }

  return -1;
}
void add_to_tlb(unsigned char logical, unsigned char physical) {
  struct tlbentry *entry = &tlb[tlbindex % TLB_SIZE];
  tlbindex++;
  entry->logical = logical;
  entry->physical = physical;
}

int main(int argc, const char *argv[])
{
  int parsed_command = 0;
  printf("ARGC: %d",argc);
  if (argc != 5) {
    fprintf(stderr, "Usage ./virtmem backingstore input\n");
    exit(1);
  }
  else if(argc == 5){
    if(strcmp(argv[3], "-p") == 0) {
      parsed_command = atoi(argv[4]);
      printf("Parsed: %d",parsed_command);
    }
  }
  
  const char *backing_filename = argv[1]; 
  int backing_fd = open(backing_filename, O_RDONLY);
  backing = mmap(0, VIRTUAL_SIZE, PROT_READ, MAP_PRIVATE, backing_fd, 0); 
  
  const char *input_filename = argv[2];
  FILE *input_fp = fopen(input_filename, "r");
  
  int i;
  for (i = 0; i < PAGES; i++) {
    pagetable[i] = -1;
  }
  char buffer[BUFFER_SIZE];
  
  int total_addresses = 0;
  int tlb_hits = 0;
  int page_faults = 0;
  unsigned char free_page = 0;

  int counterTable[PAGES] = {0};
  
  while (fgets(buffer, BUFFER_SIZE, input_fp) != NULL) {
    total_addresses++;
    int logical_address = atoi(buffer);

    int offset = logical_address & OFFSET_MASK;
    int logical_page =  (logical_address >> OFFSET_BITS) & PAGE_MASK;
    
    if(parsed_command == 1){
      counterTable[logical_page] = total_addresses;
    }

    int physical_page = search_tlb(logical_page);
    if (physical_page != -1) {
      tlb_hits++;
    } else {
      physical_page = pagetable[logical_page];
      
      if (physical_page == -1) {
        page_faults++;
        if(parsed_command == 0){
        	physical_page = fifo_replacement_policy(&free_page);
          // pagetable[logical_page] = physical_page;
          fromLogicalToPhysicalMemory(logical_page, physical_page);
        }
        else if (parsed_command == 1) {
          physical_page = lru_replacement_policy(counterTable, logical_page, page_faults);
          fromLogicalToPhysicalMemory(logical_page, physical_page);
        }
      }
      
      add_to_tlb(logical_page, physical_page);
    }
    
    int physical_address = (physical_page << OFFSET_BITS) | offset;
    signed char value = main_memory[physical_page * PAGE_SIZE + offset];
    
    printf("Virtual address: %d Physical address: %d Value: %d\n", logical_address, physical_address, value);
  }
  
  printf("Number of Translated Addresses = %d\n", total_addresses);
  printf("Page Faults = %d\n", page_faults);
  printf("Page Fault Rate = %.3f\n", page_faults / (1. * total_addresses));
  printf("TLB Hits = %d\n", tlb_hits);
  printf("TLB Hit Rate = %.3f\n", tlb_hits / (1. * total_addresses));
  
  return 0;
}
