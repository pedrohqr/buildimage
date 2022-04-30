/* Author(s): Pedro Henrique de Queiroz Ramos
 * Creates operating system image suitable for placement on a boot disk
*/
/* TODO: Comment on the status of your submission. Largely unimplemented */
//#include <assert.h>
#include <stdbool.h>
#include <elf.h>
//#include <errno.h>
//#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define IMAGE_FILE "./image"
#define ARGS "[--extended] <bootblock> <executable-file> ..."

#define SECTOR_SIZE 512       /* floppy sector size in bytes */
//#define BOOTLOADER_SIG_OFFSET 0x1fe /* offset for boot loader signature */
// more defines...

/* global variables */
char *fn_bootblock; //file name bootblock
char *fn_kernel;    //file name kernel

/* Check params argc argv:
   return true if args was passed correctly, else, return false */
bool check_args(int argc, char **argv){
	FILE *aux;
	bool ret = true;

	if (argc == 3){
		aux = fopen(argv[1], "rb"); //bootblock
		if (aux == NULL){
			ret = false;
			printf("bootblock invalid\n");
		}
		else{
			fn_bootblock = argv[1];
			fclose(aux);
		}

		aux = fopen(argv[2], "rb"); //kernel
		if (aux == NULL){
			ret = false;
			printf("kernel invalid\n");
		}
		else{
			fn_kernel = argv[2];
			fclose(aux);
		}

	}else if(argc == 4){
		aux = fopen(argv[2], "rb"); //bootblock
		if (aux == NULL){
			ret = false;
			printf("bootblock invalid\n");
		}
		else{
			fn_bootblock = argv[2];
			fclose(aux);
		}

		aux = fopen(argv[3], "rb"); //kernel
		if (aux == NULL){
			ret = false;
			printf("kernel invalid\n");
		}
		else{
			fn_kernel = argv[3];
			fclose(aux);
		}
	}else
		ret = false;

	return ret;
}

/* check so its really an elf file */
void check_ELF(Elf32_Ehdr **header, char *filename){
    if(memcmp((*header)->e_ident, ELFMAG, SELFMAG) != 0){
    	printf("%s is not a valid ELF file\n", filename);
		exit(-1);
    }
}

/* print the error for a file */
void error_file(char *str){
	printf("Could not open file %s\n", str);
	exit(-1);
}

/* Reads in an executable file in ELF format */
void read_exec_file(FILE **execfile, char *filename, Elf32_Ehdr *ehdr, Elf32_Phdr *phdr)
{     
    Elf32_Ehdr header;
  	Elf32_Phdr pheader;

    if(*execfile){
	    // read the header
	    fread(&header, sizeof(header), 1, *execfile);

	    // check so its really an elf file
	    if(memcmp(&header.e_ident, ELFMAG, SELFMAG) == 0){ 
	    	fread(&pheader, sizeof(pheader), 1, *execfile); // read program header	    	
	    	*ehdr = header; 
	    	*phdr = pheader;
	    }else{
	    	printf("Not valid ELF file for %s\n", filename);
	    	exit(-1);
	    }     
  	}else{
  		printf("Could not open file %s\n", filename);
    	exit(-1);
  	}
}

/* Writes the bootblock to the image file */
void write_bootblock(FILE **imagefile, FILE *bootfile, Elf32_Ehdr *boot_header, Elf32_Phdr *boot_phdr)
{
	if(*imagefile){
	    //write elf's LOAD segment to image, and pad to 512 bytes(1 sector)
	    char *buffer; //buffer to read headers
	    const char MBR_signature[] = {0x55, 0xaa}; // bytes of MBR
	    unsigned int n_sector; //number of sectors
	    unsigned int n_byte; //number of bytes

	    // count how many sectors in bootblock
	    if (boot_phdr->p_memsz % SECTOR_SIZE != 0)
	        n_sector = boot_phdr->p_memsz / SECTOR_SIZE + 1;
	    else
	        n_sector = boot_phdr->p_memsz / SECTOR_SIZE;

	    // get size of bootblock
	    n_byte = n_sector * SECTOR_SIZE;

	    //alloc a buffer to read
		if (!(buffer = (char *)calloc(n_byte, sizeof(char)))) { 
	        printf("malloc buffer failed! at line %d\n", __LINE__);
	        exit(-1);
	    }		 	    

	    //set pointer to offset address of bootblock
	    if (fseek(bootfile, boot_phdr->p_offset, SEEK_SET)) { 
	        printf("fseek failed! at line %d\n", __LINE__);
	        exit(-1);
	    }

	    //read file and assign to the buffer
	    if (boot_phdr->p_filesz != fread(buffer, 1, boot_phdr->p_filesz, bootfile)) { 
	        printf("read error at line %d!\n", __LINE__);
	        exit(-1);
	    }

	    //write buffer on the image
	    if (n_byte != fwrite(buffer, 1, n_byte, *imagefile)) { 
	        printf("write error at line %d!\n", __LINE__);
	        exit(-1);
	    }

	    //write MBR signature to image, which is 2 bytes
	    if (fseek(*imagefile, 510, SEEK_SET)) { 			//set the file pointer to the		    	
	        printf("fseek failed! at line %d\n", __LINE__); //last but one byte
	        exit(-1);										
	    }

	    //add the 2 bytes of MBR
	    if (2 != fwrite(MBR_signature, 1, 2, *imagefile)) { 
	        printf("write error at line %d!\n", __LINE__);
	        exit(-1);
	    }
	    free(buffer);
	}
}

/* Writes the kernel to the image file */
void write_kernel(FILE **imagefile,FILE *kernelfile,Elf32_Ehdr *kernel_header, Elf32_Phdr *kernel_phdr)
{
	if(*imagefile){
		char *buffer; //buffer to read headers
	    unsigned int n_sector; //number of sectors
	    unsigned int n_byte; //number of bytes

	    // get the nuumber of sectors of kernel file
	    if (kernel_phdr->p_memsz % SECTOR_SIZE != 0)
	        n_sector = kernel_phdr->p_memsz / SECTOR_SIZE + 1;
	    else
	        n_sector = kernel_phdr->p_memsz / SECTOR_SIZE;

	    // get the size of kernel
	    n_byte = n_sector * SECTOR_SIZE;

	    // alloc a buffer to read
		if (!(buffer = (char *)calloc(n_byte, sizeof(char)))) { 
	        printf("malloc buffer failed! at line %d\n", __LINE__);
	        exit(-1);
	    }		 

	    // set pointer to offset address of kernel
	    if (fseek(kernelfile, kernel_phdr->p_offset, SEEK_SET)) { 
	        printf("fseek failed! at line %d\n", __LINE__);
	        exit(-1);
	    }

	    //read file and assign to the buffer
	    if (kernel_phdr->p_filesz != fread(buffer, 1, kernel_phdr->p_filesz, kernelfile)) { 
	        printf("read error at line %d!\n", __LINE__);
	        exit(-1);
	    }

	    //write buffer on the image
	    if (n_byte != fwrite(buffer, 1, n_byte, *imagefile)) { 
	        printf("write error at line %d!\n", __LINE__);
	        exit(-1);
	    }
	    free(buffer);
	}
}

/* Counts the number of sectors in the kernel */
int count_kernel_sectors(Elf32_Phdr *kernel_phdr)
{   
	unsigned int file_sz = kernel_phdr->p_filesz;

	if(file_sz%SECTOR_SIZE == 0){ //doing ceil
		return (file_sz/SECTOR_SIZE);
	}else{
		return (file_sz/SECTOR_SIZE)+1;
	}
}

/* Records the number of sectors in the kernel */
void record_kernel_sectors(FILE **imagefile, Elf32_Ehdr *kernel_header, Elf32_Phdr *kernel_phdr, int num_sec)
{
	char *buffer = (char *)calloc(2, sizeof(char));
	*buffer = num_sec;	
	if(*imagefile){
		fseek(*imagefile, 2, SEEK_SET); //replace at 3th and 4th byte
		fwrite(buffer, 2, 2, *imagefile);
	}else{
		printf("Error at record kernel sectors in image file\n");
		exit(-1);
	}
}


/* Prints segment information for --extended option */
void extended_opt(Elf32_Ehdr *beh, Elf32_Phdr *bph, int k_phnum, Elf32_Ehdr *keh, Elf32_Phdr *kph, int num_sec)
{

	/* print number of disk sectors used by the image */

  
	/* bootblock segment info */
	printf("0x%.4x: %s\n", beh->e_entry, fn_bootblock);
	printf("\tsegment 0\n");
	printf("\toffset 0x%.4x\tvaddr 0x%.4x\n", bph->p_offset, bph->p_vaddr);
	printf("\tfilesz 0x%.4x\tmemsz 0x%.4x\n", bph->p_filesz, bph->p_memsz);
	printf("\twriting 0x%.4x bytes\n", bph->p_filesz);
	printf("\tpadding up to 0x%.4x\n", SECTOR_SIZE); //as the bootblock load the first sector,
													 //we can use padding up to the sector size
	/* print kernel segment info */
	printf("0x%.4x: %s\n", keh->e_entry, fn_kernel);
	printf("\tsegment 0\n");
	printf("\toffset 0x%.4x\tvaddr 0x%.4x\n", kph->p_offset, kph->p_vaddr);
	printf("\tfilesz 0x%.4x\tmemsz 0x%.4x\n", kph->p_filesz, kph->p_memsz);	
	printf("\twriting 0x%.4x bytes\n", kph->p_filesz);
	/* calc padding up */
	if(kph->p_filesz % SECTOR_SIZE == 0)
		printf("\tpadding up to 0x%.4x\n", kph->p_filesz);
	else
		printf("\tpadding up to 0x%.4x\n", (count_kernel_sectors(kph)+1)*SECTOR_SIZE);

	/* print kernel size in sectors */
	printf("os_size: %d sectors\n", num_sec);
}
// more helper functions...

/* MAIN */
int main(int argc, char **argv)
{
	// check input
	if (!check_args(argc, argv)){
		printf("USAGE: %s\n", ARGS);
		exit(-1);
	}

	FILE *kernelfile, *bootfile,*imagefile;  // file pointers for bootblock,kernel and image

	Elf32_Ehdr boot_header;	//bootblock ELF header
	Elf32_Ehdr kernel_header; //kernel ELF header
	Elf32_Phdr boot_program_header; //bootblock program header
	Elf32_Phdr kernel_program_header; //kernel program header

	// open files
	bootfile = fopen(fn_bootblock, "rb");
	kernelfile = fopen(fn_kernel, "rb");

	// check if files was opened successfully
	if(bootfile==NULL)
		error_file(fn_bootblock);
	if(kernelfile==NULL)
		error_file(fn_kernel);

	/* build image file */
	imagefile = fopen(IMAGE_FILE, "wb");
	if (imagefile == NULL){
		printf("Error at creation image file\n");
		exit(-1);
	}

	/* read executable bootblock file */
	read_exec_file(&bootfile, fn_bootblock, &boot_header, &boot_program_header);

	/* write bootblock */
	write_bootblock(&imagefile, bootfile, &boot_header, &boot_program_header);

	/* read executable kernel file */
	read_exec_file(&kernelfile, fn_kernel, &kernel_header, &kernel_program_header);

	/* write kernel segments to image */
	write_kernel(&imagefile, kernelfile, &kernel_header, &kernel_program_header);

	/* tell the bootloader how many sectors to read to load the kernel */	
	int n_sectors;
	n_sectors = count_kernel_sectors(&kernel_program_header);
	record_kernel_sectors(&imagefile, &kernel_header, &kernel_program_header, n_sectors);

	/* check for  --extended option */
	if(!strncmp(argv[1], "--extended", 11)){
		extended_opt(&boot_header, &boot_program_header, boot_header.e_phnum, &kernel_header, &kernel_program_header, n_sectors);
	}else{
		printf("0x%.4x: %s\n", boot_header.e_entry, fn_bootblock);
		printf("0x%.4x: %s\n", kernel_header.e_entry, fn_kernel);
	}

	fclose(imagefile);
	fclose(kernelfile);
	fclose(bootfile);

	return 0;
} // ends main()