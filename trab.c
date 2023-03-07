#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <string.h>

#define CHECK_BIT(var, pos) ((var) & (1 << (pos)))
#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)       \
    (byte & 0x80 ? '1' : '0'),     \
        (byte & 0x40 ? '1' : '0'), \
        (byte & 0x20 ? '1' : '0'), \
        (byte & 0x10 ? '1' : '0'), \
        (byte & 0x08 ? '1' : '0'), \
        (byte & 0x04 ? '1' : '0'), \
        (byte & 0x02 ? '1' : '0'), \
        (byte & 0x01 ? '1' : '0')

#pragma pack(push, 1)
struct FAT32_volume_ID
{
    unsigned char unused1[11];
    unsigned short BPB_BytsPerSec; /* Bytes Per Sector  	- 	offset 0x0B		-	16 Bits	 	- 	Always 512 Bytes */
    unsigned char BPB_SecPerClus;  /* Sectors Per Cluster 	- 	offset 0x0D 	-	8 Bits 		- 	1,2,4,8,16,32,64,128 */
    unsigned short BPB_RsvdSecCnt; /* # Reserved Sectors 	- 	offset 0x0E		- 	16 Bits		- 	Usually 0x20 */
    unsigned char BPB_NumFATs;     /* Number of FATs 		- 	offset 0x10		- 	8 Bits		- 	Always 2 */
    unsigned char unused2[19];
    unsigned int BPB_FATSz32; /* Sectors Per FAT 		- 	offset 0x24		- 	32 Bits		- 	Depends on disk size */
    unsigned char unused3[4];
    unsigned int BPB_RootClus; /* Root Directory 		- 	offset 0x2C 	-	32 Bits		-	Usually 0x00000002 */
    unsigned char unused4[23];
    unsigned char BPB_Label[11]; /* Volume label 		-	ofsset 0x47		-	12 bytes    -   String name 		*/
    unsigned char BPB_Fat[8];    /* Type of fat 			-	ofsset 0x52		-	8 bytes     -   String name 		*/
    unsigned char unused5[420];
    unsigned short signature; /* Signature (none)		- 	offset 0x1FE	-	16 Bits		- 	Always 0xAA55 */
};

struct directory_entry
{
    unsigned char DIR_Name[8]; /* Short Filename		0x00	11 Bytes*/
    unsigned char File_ext[3]; /* File Extension		0x00	11 Bytes*/
    unsigned char Attrib;      /* Byte	DIR_Attr		0x0B	8 Bits */
    unsigned char unused1[8];
    unsigned short DIR_FstClusHI; /* First Cluster High 	0x14	16 Bits */
    unsigned char unused2[4];
    unsigned short DIR_FstClusLO; /* First Cluster Low	0x1A	16 Bits */
    unsigned int DIR_FileSize;    /* File Size 			0x1C	32 Bits*/
};
#pragma pack(pop)

/*
	Attrib mean:
		0x01: read only
		0x02: hidden
		0x04: system
		0x08: volume label
		0x10: directory
		0x20: archived
*/

struct directory_table
{
    struct directory_entry entry[128];
};

struct FAT32_volume_ID FAT32_vol;
struct directory_table dt;
struct directory_table dteste;

/** Print caracters. */
void print_name(unsigned char *n, int sz)
{
    int i;

    for (i = 0; i < sz; i++) {
        printf("%c", n[i]);
    }

    printf("\n");
}

/* Print FAT32 voluem ID. */
void print_FAT32_volume_ID(struct FAT32_volume_ID *ft) {
    printf("--------------- FAT32 Volume ID --------------------\n");
    printf("Volume name: ");
    print_name(ft->BPB_Label, 11);
    printf("FAT Type: ");
    print_name(ft->BPB_Fat, 8);
    printf("Bytes Per Sector: %i\n", ft->BPB_BytsPerSec);
    printf("Sectors Per Cluster: %i\n", ft->BPB_SecPerClus);
    printf("Number of Reserved Sectors: %i\n", ft->BPB_RsvdSecCnt);
    printf("Number of FATs: %i\n", ft->BPB_NumFATs);
    printf("Sectors Per FAT: %i\n", ft->BPB_FATSz32);
    printf("Root Directory: %i\n", ft->BPB_RootClus);
    printf("Signature: 0x%x\n", ft->signature);
    printf("----------------------------------------------------\n");
}

/** Print directory table entry . */
void print_dir_entry_name(struct directory_entry *de) {

    int i;
    unsigned int aux_cluster;

    if (de->Attrib & 0x10) {
        printf("d ");
    }
    else {
        printf("f ");
    }

    for (i = 0; i < 8 && de->DIR_Name[i] != ' '; i++) {
        printf("%c", de->DIR_Name[i]);
    }

    if (de->Attrib & 0x10) {
        aux_cluster = (de->DIR_FstClusHI << 16) + de->DIR_FstClusLO;
        printf(" -- next cluster: %d\n", aux_cluster);
        return;
    }

    printf(".");

    for (i = 0; i < 3; i++) {
        printf("%c", de->File_ext[i]);
    }

    printf(" %d", de->DIR_FileSize);

    aux_cluster = (de->DIR_FstClusHI << 16) + de->DIR_FstClusLO;
    printf(" -- next cluster: %d\n", aux_cluster);
}

void recursive_dir_explorer(struct directory_table *dt) {}

/* Read a chunk of data. */
int read_data(FILE *f, unsigned int pos, void *bf, unsigned int sz) {

    int ret;

    if (fseek(f, pos, SEEK_SET)) {
        perror("fseek()");
    }

    ret = fread(bf, 1, sz, f);
    return ret;
}

/* Calc a cluster position in the disk. */
unsigned int calc_cluster_pos(struct FAT32_volume_ID *ft, unsigned int cluster) {
    return (ft->BPB_RsvdSecCnt + (ft->BPB_NumFATs * ft->BPB_FATSz32) + (cluster - 2) * ft->BPB_SecPerClus) * ft->BPB_BytsPerSec;
}


/* Printf all valid entries in a directory table. */
void print_dir(struct directory_table *dt, FILE *dev)
{
    int i;
    unsigned int clust_aux;
    unsigned char resto[256];

    for (i = 0; i < 128; i++) {
    
        memset(resto, 0, 256);
        if (dt->entry[i].DIR_Name[0] != 0 &&
            dt->entry[i].DIR_Name[0] != 0xE5 &&
            dt->entry[i].DIR_Name[0] != 0x2E) {

            if (dt->entry[i].DIR_Name[0] == 0x5) {
                dt->entry[i].DIR_Name[0] = 0xE5;
            }

            if ((dt->entry[i].Attrib & 0x8) || (dt->entry[i].Attrib & 0x4)) {
                continue;
            }

            print_dir_entry_name(&dt->entry[i]);
            
            if (dt->entry[i].Attrib & 0x10) {
                clust_aux = (dt->entry[i].DIR_FstClusHI << 16) + dt->entry[i].DIR_FstClusLO;
                unsigned int clust_pos = calc_cluster_pos(&FAT32_vol, clust_aux);
                struct directory_table dcluster;
                read_data(dev, clust_pos, &dcluster, 4096);
                print_dir(&dcluster, dev);
            }
            
            if (dt->entry[i].DIR_Name[7] & 0x0F) {
                int n = 1;
                do {
                    strcat(resto, &dt->entry[i - n].DIR_Name[1]);
                    strcat(resto, &dt->entry[i - n].DIR_Name[3]);
                    strcat(resto, &dt->entry[i - n].DIR_Name[5]);
                    strcat(resto, &dt->entry[i - n].DIR_Name[7]);
                    strcat(resto, &dt->entry[i - n].DIR_Name[9]);
                    strcat(resto, &dt->entry[i - n].DIR_Name[14]);
                    strcat(resto, &dt->entry[i - n].DIR_Name[16]);
                    strcat(resto, &dt->entry[i - n].DIR_Name[18]);
                    strcat(resto, &dt->entry[i - n].DIR_Name[20]);
                    strcat(resto, &dt->entry[i - n].DIR_Name[22]);
                    strcat(resto, &dt->entry[i - n].DIR_Name[24]);
                    strcat(resto, &dt->entry[i - n].DIR_Name[28]);
                    strcat(resto, &dt->entry[i - n].DIR_Name[30]);

                    n++;

                } while (!((1 << 6) & dt->entry[i - n + 1].DIR_Name[0]));

                printf("%d %s \n", n, resto);

                clust_aux = (dt->entry[i].DIR_FstClusHI << 16) + dt->entry[i].DIR_FstClusLO;
                unsigned int clust_pos = calc_cluster_pos(&FAT32_vol, clust_aux);
                struct directory_table dresto;
                read_data(dev, clust_pos, &dresto, 4096);
            }
        }
    }
}

int main(int argc, char **argv)
{
    FILE *dev;
    unsigned int fat1_pos, root_dir_pos;
    unsigned int *fat1, fatsz;

    if (argc < 2)
    {
        printf("USAGE: %s <filename>\n", argv[0]);
    }

    if ((dev = fopen(argv[1], "r")) == NULL)
    {
        perror("fopen()");
        return -1;
    }

    /* Read FAT32 Volume ID. */
    fread(&FAT32_vol, 1, 512, dev);

    print_FAT32_volume_ID(&FAT32_vol);

    if (strncmp((char *)FAT32_vol.BPB_Fat, "FAT32", 5))
    {
        printf("File system not suported.\n");
        return 0;
    }

    /* Basic variables to access the file system. 

	   This numbers are giving in cluster number, to use fseek()/fread() you 
	   need to multiply them by BPB_BytsPerSec. 
	*/

    /* Fat1 offset */
    fat1_pos = FAT32_vol.BPB_RsvdSecCnt * FAT32_vol.BPB_BytsPerSec;

    /* Fat size */
    fatsz = FAT32_vol.BPB_FATSz32 * FAT32_vol.BPB_BytsPerSec;
    fat1 = malloc(fatsz);

    /* Root Dir offset */
    root_dir_pos = calc_cluster_pos(&FAT32_vol, FAT32_vol.BPB_RootClus);

    printf("\n\n");
    printf("FAT1 offset: 0x%x\n", fat1_pos);
    printf("FAT size: %d bytes\n", fatsz);
    printf("Root Dir offset: 0x%x\n", root_dir_pos);

    /* Read fat1 table - Read only the first sector of FAT. The FAT is BPB_FATSz32 sectors. */
    read_data(dev, fat1_pos, fat1, fatsz);

    /* Read root dir table */
    read_data(dev, root_dir_pos, &dt, 4096);

    /* Show root directory */
    printf("\n\nROOT DIR: \n");
    print_dir(&dt, dev);

    free(fat1);
    fclose(dev);

    return 0;
}
