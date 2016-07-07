#include <libelf.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <utils.h>

typedef uint32_t __be32;

struct bnx2_fw_file_section {
	__be32 addr;
	__be32 len;
	__be32 offset;
} __attribute__ ((__packed__));

struct bnx2_mips_fw_file_entry {
	__be32 start_addr;
	struct bnx2_fw_file_section text;
	struct bnx2_fw_file_section data;
	struct bnx2_fw_file_section rodata;
} __attribute__ ((__packed__));

struct bnx2_mips_fw_file {
	struct bnx2_mips_fw_file_entry com;
	struct bnx2_mips_fw_file_entry cp;
	struct bnx2_mips_fw_file_entry rxp;
	struct bnx2_mips_fw_file_entry tpat;
	struct bnx2_mips_fw_file_entry txp;
} __attribute__ ((__packed__));

char string_tbl[] = {
		'\0',
		'.','t','e','x','t','\0',
		'.','d','a','t','a','\0',
		'.','r','o','d','a','t','a','\0',
};

int main ( int argc, char *argv[] ) {
	int fd;
	FILE *fp;
	Elf *e;
	Elf_Scn *scn;
	Elf_Data *data;
	Elf32_Ehdr *ehdr;
	Elf32_Phdr *phdr;
	Elf32_Shdr *shdr;
	struct bnx2_mips_fw_file fw;
	struct bnx2_mips_fw_file_entry *entry;
	uint8_t *text_bytes;
	uint8_t *data_bytes;
	uint8_t *rodata_bytes;

	if ( argc != 4 ) {
		fprintf ( stderr, "Usage: %s <mips.fw> <com|cp|rxp|tpat|txp> <out.elf>\n", argv[0] );
		return 1;
	}

	fp = fopen ( argv[1], "rb" );
	if ( ! fp ) {
		fprintf ( stderr, "Failed to open file '%s'\n", argv[1] );
		return 1;
	}
	if ( fread ( &fw, sizeof ( struct bnx2_mips_fw_file ), 1, fp ) != 1 ) {
		fprintf ( stderr, "I/O error for file '%s'\n", argv[1] );
		return 1;
	}
	if ( strcmp ( argv[2], "com" ) == 0 )
		entry = &fw.com;
	else if ( strcmp ( argv[2], "cp" ) == 0 )
		entry = &fw.cp;
	else if ( strcmp ( argv[2], "rxp" ) == 0 )
		entry = &fw.cp;
	else if ( strcmp ( argv[2], "tpat" ) == 0 )
		entry = &fw.cp;
	else if ( strcmp ( argv[2], "txp" ) == 0 )
		entry = &fw.cp;
	else {
		fprintf ( stderr, "Invalid CPU argument (see usage)\n" );
		return 1;
	}
	entry->start_addr = be32_to_cpu ( entry->start_addr );
	entry->text.addr = be32_to_cpu ( entry->text.addr );
	entry->text.len = be32_to_cpu ( entry->text.len );
	entry->text.offset = be32_to_cpu ( entry->text.offset );
	entry->data.addr = be32_to_cpu ( entry->data.addr );
	entry->data.len = be32_to_cpu ( entry->data.len );
	entry->data.offset = be32_to_cpu ( entry->data.offset );
	entry->rodata.addr = be32_to_cpu ( entry->rodata.addr );
	entry->rodata.len = be32_to_cpu ( entry->rodata.len );
	entry->rodata.offset = be32_to_cpu ( entry->rodata.offset );
	if ( elf_version ( EV_CURRENT ) == EV_NONE ) {
		fprintf ( stderr, "libelf initialization failed\n" );
		return 1;
	}
	fd = open ( argv[3], O_WRONLY | O_CREAT, 0777 );
	if ( fd < 0 ) {
		fprintf ( stderr, "Failed to open output file '%s'\n", argv[3] );
		return 1;
	}
	if ( ( e = elf_begin ( fd, ELF_C_WRITE, NULL ) ) == NULL ) {
		fprintf ( stderr, "libelf error: %s\n", elf_errmsg( -1 ) );
		return 1;
	}
	if ( ( ehdr = elf32_newehdr ( e ) ) == NULL ) {
		fprintf ( stderr, "libelf error: %s\n", elf_errmsg ( -1 ) );
		return 1;
	}
	ehdr->e_ident[EI_DATA] = ELFDATA2MSB;
	ehdr->e_machine = EM_MIPS;
	ehdr->e_type = ET_EXEC;
	ehdr->e_version = EV_CURRENT;
	ehdr->e_entry = entry->start_addr;
	if ( ( phdr = elf32_newphdr ( e, 1 ) ) == NULL ) {
		fprintf ( stderr, "libelf error: %s\n", elf_errmsg ( -1 ) );
		return 1;
	}
	if ( ( scn = elf_newscn ( e ) ) == NULL ) {
		fprintf ( stderr, "libelf error: %s\n", elf_errmsg ( -1 ) );
		return 1;
	}
	if ( ( data = elf_newdata ( scn ) ) == NULL ) {
		fprintf ( stderr, "libelf error: %s\n", elf_errmsg ( -1 ) );
		return 1;
	}
	text_bytes = malloc ( entry->text.len );
	if ( ! text_bytes ) {
		fprintf ( stderr, "malloc failed\n" );
		return 1;
	}
	if ( fseek ( fp, entry->text.offset, SEEK_SET ) != 0 ) {
		fprintf ( stderr, "I/O error for file '%s'\n", argv[1] );
		return 1;
	}
	if ( fread ( text_bytes, 1, entry->text.len, fp ) != entry->text.len ) {
		fprintf ( stderr, "I/O error for file '%s'\n", argv[1] );
		return 1;
	}
	data->d_align = 1;
	data->d_off = 0LL;
	data->d_buf = text_bytes;
	data->d_type = ELF_T_BYTE;
	data->d_size = entry->text.len;
	data->d_version = EV_CURRENT;

	if ( ( shdr = elf32_getshdr ( scn ) ) == NULL ) {
		fprintf ( stderr, "libelf error: %s\n", elf_errmsg ( -1 ) );
		return 1;
	}
	shdr->sh_name = 1;
	shdr->sh_type = SHT_PROGBITS;
	shdr->sh_flags = SHF_ALLOC | SHF_EXECINSTR;
	shdr->sh_addr = entry->text.addr;
	shdr->sh_entsize = 0;

	if ( ( scn = elf_newscn ( e ) ) == NULL ) {
		fprintf ( stderr, "libelf error: %s\n", elf_errmsg ( -1 ) );
		return 1;
	}
	if ( ( data = elf_newdata ( scn ) ) == NULL ) {
		return 1;
		fprintf ( stderr, "libelf error: %s\n", elf_errmsg ( -1 ) );
	}
	data_bytes = malloc ( entry->data.len );
	if ( ! data_bytes ) {
		fprintf ( stderr, "malloc failed\n" );
		return 1;
	}
	if ( fseek ( fp, entry->data.offset, SEEK_SET ) != 0 ) {
		fprintf ( stderr, "I/O error for file '%s'\n", argv[1] );
		return 1;
	}
	if ( fread ( data_bytes, 1, entry->data.len, fp ) != entry->data.len ) {
		fprintf ( stderr, "I/O error for file '%s'\n", argv[1] );
		return 1;
	}
	data->d_align = 1;
	data->d_off = 0LL;
	data->d_buf = data_bytes;
	data->d_type = ELF_T_BYTE;
	data->d_size = entry->data.len;
	data->d_version = EV_CURRENT;

	if ( ( shdr = elf32_getshdr ( scn ) ) == NULL ) {
		fprintf ( stderr, "libelf error: %s\n", elf_errmsg ( -1 ) );
		return 1;
	}
	shdr->sh_name = 7;
	shdr->sh_type = SHT_PROGBITS;
	shdr->sh_flags = SHF_ALLOC | SHF_WRITE;
	shdr->sh_addr = entry->data.addr;
	shdr->sh_entsize = 0;

	if ( ( scn = elf_newscn ( e ) ) == NULL ) {
		fprintf ( stderr, "libelf error: %s\n", elf_errmsg ( -1 ) );
		return 1;
	}
	if ( ( data = elf_newdata ( scn ) ) == NULL ) {
		fprintf ( stderr, "libelf error: %s\n", elf_errmsg ( -1 ) );
		return 1;
	}
	rodata_bytes = malloc ( entry->rodata.len );
	if ( ! rodata_bytes ) {
		fprintf ( stderr, "malloc failed\n" );
		return 1;
	}
	if ( fseek ( fp, entry->rodata.offset, SEEK_SET ) != 0 ) {
		fprintf ( stderr, "I/O error for file '%s'\n", argv[1] );
		return 1;
	}
	if ( fread ( rodata_bytes, 1, entry->rodata.len, fp ) != entry->rodata.len ) {
		fprintf ( stderr, "I/O error for file '%s'\n", argv[1] );
		return 1;
	}
	fclose ( fp );
	data->d_align = 1;
	data->d_off = 0LL;
	data->d_buf = rodata_bytes;
	data->d_type = ELF_T_BYTE;
	data->d_size = entry->rodata.len;
	data->d_version = EV_CURRENT;

	if ( ( shdr = elf32_getshdr ( scn ) ) == NULL ) {
		fprintf ( stderr, "libelf error: %s\n", elf_errmsg ( -1 ) );
		return 1;
	}
	shdr->sh_name = 13;
	shdr->sh_type = SHT_PROGBITS;
	shdr->sh_flags = SHF_ALLOC;
	shdr->sh_addr = entry->rodata.addr;
	shdr->sh_entsize = 0;

	if ( ( scn = elf_newscn ( e ) ) == NULL ) {
		fprintf ( stderr, "libelf error: %s\n", elf_errmsg ( -1 ) );
		return 1;
	}
	if ( ( data = elf_newdata ( scn ) ) == NULL ) {
		fprintf ( stderr, "libelf error: %s\n", elf_errmsg ( -1 ) );
		return 1;
	}
	data->d_align = 1;
	data->d_buf = string_tbl;
	data->d_off = 0LL;
	data->d_size = sizeof ( string_tbl );
	data->d_type = ELF_T_BYTE;
	data->d_version = EV_CURRENT;
	
	if ( ( shdr = elf32_getshdr ( scn ) ) == NULL ) {
		fprintf ( stderr, "libelf error: %s\n", elf_errmsg ( -1 ) );
		return 1;
	}
	shdr->sh_name = 0;
	shdr->sh_type = SHT_STRTAB;
	shdr->sh_flags = SHF_STRINGS | SHF_ALLOC;
	shdr->sh_entsize = 0;

	ehdr->e_shstrndx = elf_ndxscn ( scn );

	if ( elf_update ( e, ELF_C_NULL ) < 0 ) {
		fprintf ( stderr, "libelf error: %s\n", elf_errmsg ( -1 ) );
		return 1;
	}
	phdr->p_type = PT_PHDR;
	phdr->p_offset = ehdr->e_phoff;
	phdr->p_filesz = elf32_fsize ( ELF_T_PHDR, 1, EV_CURRENT );
	phdr->p_vaddr = 0x8000000;
	elf_flagphdr ( e, ELF_C_SET, ELF_F_DIRTY );
	if ( elf_update ( e, ELF_C_WRITE ) < 0 ) {
		fprintf ( stderr, "libelf error: %s\n", elf_errmsg ( -1 ) );
		return 1;
	}
	elf_end ( e );
	close ( fd );
	printf ( "Done\n" );
	return 0;
}