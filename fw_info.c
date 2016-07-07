/*
 * Copyright (C) 2012 Daniel Wyatt <Daniel.Wyatt@gmail.com>.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */
#include <stdio.h>
#include <stdint.h>
#include <utils.h>

typedef uint32_t __be32;

struct bnx2_fw_file_section {
	__be32 addr;
	__be32 len;
	__be32 offset;
} __attribute__ ((__packed__));

struct bnx2_rv2p_fw_file_entry {
	struct bnx2_fw_file_section rv2p;
	__be32 fixup[8];
} __attribute__ ((__packed__));

struct bnx2_rv2p_fw_file {
	struct bnx2_rv2p_fw_file_entry proc1;
	struct bnx2_rv2p_fw_file_entry proc2;
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

void print_indent ( int indent ) {
	int i;
	for ( i = 0; i < indent; i++ ) {
		printf ( "    " );
	}
}

void section_info ( struct bnx2_fw_file_section *section, int indent ) {
	print_indent ( indent );
	printf ( "addr  : 0x%08X\n", be32_to_cpu ( section->addr ) );
	print_indent ( indent );
	printf ( "len   : 0x%08X\n", be32_to_cpu ( section->len ) );
	print_indent ( indent );
	printf ( "offset: 0x%08X\n", be32_to_cpu ( section->offset ) );
}

void mips_entry_info ( struct bnx2_mips_fw_file_entry *e ) {
	print_indent ( 1 );
	printf ( "start addr: 0x%08X\n", be32_to_cpu ( e->start_addr ) );
	print_indent ( 1 );
	printf ( ".text:\n" );
	section_info ( &e->text, 2 );

	print_indent ( 1 );
	printf ( ".data:\n" );
	section_info ( &e->data, 2 );

	print_indent ( 1 );
	printf ( ".rodata:\n" );
	section_info ( &e->rodata, 2 );
}

void mips_info ( struct bnx2_mips_fw_file *f ) {
	printf ( "MIPS firmware\n" );
	printf ( "COM (Completion Processor):\n");
	mips_entry_info ( &f->com );
	printf ( "CP (Command Processor):\n");
	mips_entry_info ( &f->cp );
	printf ( "RXP (RX Processor):\n");
	mips_entry_info ( &f->rxp );
	printf ( "TPAT (TX Patch-up Processor):\n");
	mips_entry_info ( &f->tpat );
	printf ( "TXP (TX Processor):\n");
	mips_entry_info ( &f->txp );
}

void rv2p_info ( struct bnx2_rv2p_fw_file *f ) {
	printf ( "RV2P firmware\n" );
	printf ( "proc1:\n" );
	section_info ( &f->proc1.rv2p, 1 );

	printf ( "proc2:\n" );
	section_info ( &f->proc2.rv2p, 1 );
}

int main ( int argc, char *argv[] ) {
	FILE *fp;
	struct bnx2_rv2p_fw_file rv2p_fw;
	struct bnx2_mips_fw_file mips_fw;
	if ( argc != 2 ) {
		fprintf ( stderr, "Usage: %s <firmware.fw>\n", argv[0] );
		return 1;
	}
	fp = fopen ( argv[1], "rb" );
	if ( ! fp ) {
		fprintf ( stderr, "Failed to open file '%s'\n", argv[1] );
		return 1;
	}
	if ( strstr ( argv[1], "rv2p" ) ) {
		if ( fread ( &rv2p_fw, sizeof(rv2p_fw), 1, fp ) != 1 ) {
			fprintf ( stderr, "I/O error on file '%s'\n", argv[1]);
			return 1;
		}
		rv2p_info ( &rv2p_fw );
	}
	else if ( strstr ( argv[1], "mips" ) ) {
		if ( fread ( &mips_fw, sizeof(mips_fw), 1, fp ) != 1 ) {
			fprintf ( stderr, "I/O error on file '%s'\n", argv[1]);
			return 1;
		}
		mips_info ( &mips_fw );
	} else {
		fprintf ( stderr, "Filename '%s' unrecognized, should contain 'rv2p' or 'mips'\n", argv[1] );
		return 1;
	}
	return 0;
}