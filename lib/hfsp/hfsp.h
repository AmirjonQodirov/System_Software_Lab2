#ifndef SOFTWARE1LIB_HFSP_H
#define SOFTWARE1LIB_HFSP_H

#include <stdint.h>
#include <bits/types/FILE.h>
#include <stdlib.h>
#include <stdbool.h>

enum {
    kHFSPlusFolderRecord = 1,
    kHFSPlusFileRecord = 2,
    kHFSPlusFolderThreadRecord = 3,
    kHFSPlusFileThreadRecord = 4
};

typedef struct {
    uint32_t startBlock;
    uint32_t blockCount;
} hfsp_extent_descriptor;

typedef struct {
    uint64_t logicalSize;
    uint32_t clumpSize;
    uint32_t totalBlocks;
    hfsp_extent_descriptor extents[8];
} hfsp_fork_data;

typedef struct {
    //40 before blockSize
    uint32_t blockSize;
    //36 after blockSize
    //8*4 for finderInfo
    //mesto dly 5 specialnih failov & get catalogFile
    //80 for allocationFile
    //80 for extentsFile
    hfsp_fork_data *catalogFile;
    //80 for allocationFile
    //80 for extentsFile
} hfsp_volume_header;

typedef struct {
    uint32_t fLink;
    uint32_t bLink;
    //2 for kind&height
    uint16_t numRecords;
    //2 for reserved
} hfsp_node_descriptor;

typedef struct  {
    //10
    uint32_t    firstLeafNode;
    uint32_t    lastLeafNode;
    uint16_t    nodeSize;
    uint16_t    maxKeyLength;
    uint32_t    totalNodes;
    uint32_t    freeNodes;
    //12
    //4*16
} hfsp_header_record;

typedef struct {
    hfsp_node_descriptor *descriptor;
    void **records;
    uint16_t *record_pointers;
} hfsp_node;

typedef struct {
    hfsp_node *header_node;
    hfsp_node **leaf_nodes;
    uint32_t leaf_nodes_num;
    uint32_t no_leaf_nodes_num;
    
    uint64_t fd;
    uint32_t btree_offset;
} hfsp_btree;

typedef struct {
    uint16_t key_length;
    uint32_t parent_id;
    uint16_t sym_length;
    char *node_name;
} hfsp_catalog_key;

typedef struct {
    //8 before fID
    uint32_t folderID;
    //last 76
} hfsp_catalog_folder;

typedef struct {
    //32
    //48
    //8
    hfsp_fork_data dataFork;
    //40
} hfsp_catalog_file;

typedef struct {
    char *pwd;
    uint32_t pathFolderIds[500];
    uint32_t path_folder_depth;
    uint32_t current_folder_id;
    
    hfsp_catalog_folder *current_folder_info;
    hfsp_volume_header *vol_header;
    hfsp_btree *catalog_file;
} hfsplus;

typedef enum {
    ERR_DEF, //def
    ERR_CANT_FIND, //emp
    ERR_FIND_FILE, //file_record
    ERR_ON_ROOT, //on_root
    ERR_RESERVE, //is_not_nullTERM
    ERR_WRONG_STRUCTURE //bad_structure
} ERRORS;

ERRORS ERR;

//FUNCTIONS

hfsplus *read_hfsplus(char *filename);

char *pwd(hfsplus *fs);

uint32_t ls(hfsplus *fs, char **buffer, int *setType);

char *cd(hfsplus *fs, char *destination);

char *back(hfsplus *fs);

int32_t copy(char *name, char *destination, hfsplus *fs);

#endif //SOFTWARE1LIB_HFSP_H
