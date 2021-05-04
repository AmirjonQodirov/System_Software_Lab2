#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <ftw.h>
#include "hfsp.h"
#include "../read_be/read_be.h"

#define HEAD_RESERVED_SIZE 1024
#define ROOT_FOLDER_ID 1
#define KEY_SIZE_LENGTH 2

#define USER_RECORDS_SIZE 128
#define MAP_RECORD_OFFSET 256
ERRORS ERR = ERR_DEF;

//read_hfsp
bool checking_for_null_term(const char *buffer, uint32_t length) {
    for (int i = 0; i < length; ++i) {
        if (buffer[i] != '\0') {
            printf("%d\n",i);
            return false;
        }
    }
    return true;
}

//read_hfsp
hfsp_fork_data *read_fork_data_from_fd(uint64_t fd) {
    hfsp_fork_data *res = malloc(sizeof(hfsp_fork_data));
    read_uint64(&res->logicalSize, fd);
    read_uint32(&res->clumpSize, fd);
    read_uint32(&res->totalBlocks, fd);
    for (int i = 0; i < 8; ++i) {
        read_uint32(&res->extents[i].startBlock, fd);
        read_uint32(&res->extents[i].blockCount, fd);
    }
    return res;
}

//read_hfsp
hfsp_volume_header *read_header_from_fd(uint64_t fd) {
    hfsp_volume_header *res = malloc(sizeof(hfsp_volume_header));
    lseek(fd, 40, SEEK_CUR);
    read_uint32(&res->blockSize, fd);
    lseek(fd, 36, SEEK_CUR);
    lseek(fd, 32, SEEK_CUR);
    lseek(fd, 80, SEEK_CUR);
    lseek(fd, 80, SEEK_CUR);
    res->catalogFile = read_fork_data_from_fd(fd);
    lseek(fd, 80, SEEK_CUR);
    lseek(fd, 80, SEEK_CUR);
    return res;
}

//for nodes
hfsp_node_descriptor *read_descriptor(uint64_t fd, uint32_t offset) {
    lseek(fd, offset, SEEK_SET);
    hfsp_node_descriptor *descriptor = malloc(sizeof(hfsp_node_descriptor));
    read_uint32(&descriptor->fLink, fd);
    read_uint32(&descriptor->bLink, fd);
    lseek(fd, 2, SEEK_CUR);
    read_uint16(&descriptor->numRecords, fd);
    lseek(fd, 2, SEEK_CUR);
    return descriptor;
}

//for creating header-node in btree
hfsp_header_record *read_header_record(uint64_t fd) {
    hfsp_header_record *result = malloc(sizeof(hfsp_header_record));
    lseek(fd, 10, SEEK_CUR);
    read_uint32(&result->firstLeafNode, fd);
    read_uint32(&result->lastLeafNode, fd);
    read_uint16(&result->nodeSize, fd);
    read_uint16(&result->maxKeyLength, fd);
    read_uint32(&result->totalNodes, fd);
    read_uint32(&result->freeNodes, fd);
    lseek(fd, 12, SEEK_CUR);
    lseek(fd, 64, SEEK_CUR);
    return result;
}

//for creating b-tree(without filling records)
hfsp_node *read_header_node(hfsp_btree *btree_ptr) {
    hfsp_node *header_node = malloc(sizeof(hfsp_node));
    
    header_node->descriptor = read_descriptor(btree_ptr->fd, btree_ptr->btree_offset);
    
    header_node->record_pointers = malloc(sizeof(uint16_t) * (header_node->descriptor->numRecords + 1));
    header_node->records = malloc(sizeof(void *) * header_node->descriptor->numRecords);
    header_node->records[0] = read_header_record(btree_ptr->fd);
    
    header_node->records[1] = malloc(USER_RECORDS_SIZE);
    read(btree_ptr->fd, header_node->records[1], USER_RECORDS_SIZE);

    uint32_t map_record_size = ((hfsp_header_record *) header_node->records[0])->nodeSize - MAP_RECORD_OFFSET;    
    header_node->records[2] = malloc(map_record_size);
    read(btree_ptr->fd, header_node->records[2], map_record_size);
    return header_node;
}

//for getting header_record from header node
hfsp_header_record *get_header_record(hfsp_btree *btree_ptr) {
    return (hfsp_header_record *) btree_ptr->header_node->records[0];
}

//when read node
void set_rec_pointers(hfsp_node *ptr, uint64_t fd, uint32_t offset) {
    int32_t size = ptr->descriptor->numRecords + 1;
    ptr->record_pointers = malloc(SIZE16 * size);
    lseek(fd, offset - (size * SIZE16), SEEK_SET);
    for (int i = size - 1; i >= 0; --i) {
        read_uint16(&ptr->record_pointers[i], fd);
    }
}

//when read node with data
void set_recs(hfsp_node *ptr, uint64_t fd, uint32_t offset) {
    ptr->records = malloc(sizeof(char *) * ptr->descriptor->numRecords);
    for (int i = 0; i < ptr->descriptor->numRecords; ++i) {
        size_t rec_size = ptr->record_pointers[i + 1] - ptr->record_pointers[i];
        ptr->records[i] = malloc(rec_size);
        lseek(fd, ptr->record_pointers[i] + offset, SEEK_SET);
        read(fd, ptr->records[i], rec_size);
    }
}

hfsp_node *read_node(hfsp_btree *btree_ptr, bool with_data, uint32_t node_size, uint32_t node_number) {
    uint32_t node_offset = btree_ptr->btree_offset + node_size * node_number;
    hfsp_node *result = malloc(sizeof(hfsp_node));
    result->descriptor = read_descriptor(btree_ptr->fd, node_offset);
    set_rec_pointers(result, btree_ptr->fd, node_offset + node_size);
    if (with_data) {
        set_recs(result, btree_ptr->fd, node_offset);
    }
    return result;
}

hfsp_node *get_node(uint32_t node_number, hfsp_btree *btree_ptr, bool with_data) {
    if (btree_ptr->leaf_nodes[node_number - btree_ptr->no_leaf_nodes_num] == NULL) {
        btree_ptr->leaf_nodes[node_number - btree_ptr->no_leaf_nodes_num] = read_node(
                btree_ptr,
                with_data,
                get_header_record(btree_ptr)->nodeSize,
                node_number
        );
    }
    return btree_ptr->leaf_nodes[node_number - btree_ptr->no_leaf_nodes_num];
}

//initial b-tree
hfsp_btree *create_btree_start_offset(uint64_t fd, uint32_t btree_offset) {
    hfsp_btree *result = malloc(sizeof(hfsp_btree));
    lseek(fd, btree_offset, SEEK_SET);
    result->btree_offset = btree_offset;
    result->fd = fd;
    
    result->header_node = read_header_node(result);
    hfsp_header_record *h_record = get_header_record(result);

    result->leaf_nodes_num = h_record->lastLeafNode -
                             h_record->firstLeafNode + 1;
                     
    result->no_leaf_nodes_num = h_record->totalNodes - h_record->freeNodes - result->leaf_nodes_num;

    result->leaf_nodes = malloc(sizeof(void *) * result->leaf_nodes_num);
    if (result->leaf_nodes_num == 1) {
        result->leaf_nodes[0] = get_node(h_record->firstLeafNode, result, true);
    }
    return result;
}

//get key from offset(untill rec_pointers)
hfsp_catalog_key *get_catalog_key(hfsp_node *node_ptr, uint32_t record_number, hfsp_btree *btree_ptr, uint32_t node_index) {
    hfsp_catalog_key *res = malloc(sizeof(hfsp_catalog_key));

    uint32_t offset = btree_ptr->btree_offset + node_index * get_header_record(btree_ptr)->nodeSize +
                      node_ptr->record_pointers[record_number];
    lseek(btree_ptr->fd, offset, SEEK_SET);
    read_uint16(&res->key_length, btree_ptr->fd);
    read_uint32(&res->parent_id, btree_ptr->fd);
    read_uint16(&res->sym_length, btree_ptr->fd);
    res->node_name = malloc(res->sym_length);
    for (int i = 0; i < res->sym_length; ++i) {
        read_char(&res->node_name[i], btree_ptr->fd);
        read_char(&res->node_name[i], btree_ptr->fd);
    }

    return res;
}

void free_key(hfsp_catalog_key *ptr) {
    free(ptr->node_name);
    free(ptr);
}

uint32_t fill_key(hfsp_btree *btree_ptr, uint32_t item_id, hfsp_catalog_key *fkey) {
    uint32_t index = get_header_record(btree_ptr)->firstLeafNode;
    hfsp_node *current;
    while (index != 0) {
        current = get_node(index, btree_ptr, true);
        hfsp_catalog_key *key;
        for (int j = 0; j < current->descriptor->numRecords; ++j) {
            key = get_catalog_key(current, j, btree_ptr, index);
            if (key->parent_id == item_id) {
                fkey->sym_length = key->sym_length;
                fkey->key_length = key->key_length;
                fkey->parent_id = key->parent_id;
                fkey->node_name = key->node_name;

                uint32_t offset = btree_ptr->btree_offset + index * get_header_record(btree_ptr)->nodeSize +
                                  current->record_pointers[j] + fkey->key_length + KEY_SIZE_LENGTH;
                lseek(btree_ptr->fd, offset, SEEK_SET);
                uint32_t result = current->record_pointers[j + 1] - current->record_pointers[j];
                return result;
            }
            free_key(key);
        }
        index = current->descriptor->fLink;
    }
    return -1;
}

//getting folder id
hfsp_catalog_folder *get_folder_id(hfsp_btree *btree_ptr) {
    hfsp_catalog_folder *folder = malloc(sizeof(hfsp_catalog_folder));
    lseek(btree_ptr->fd, 8, SEEK_CUR);
    read_uint32(&folder->folderID, btree_ptr->fd);
    lseek(btree_ptr->fd, 20, SEEK_CUR);
    lseek(btree_ptr->fd, 48, SEEK_CUR);
    lseek(btree_ptr->fd, 8, SEEK_CUR);
    return folder;
}

//FUNCTION_1
hfsplus *read_hfsplus(char *filename) {
    uint64_t fd = open(filename, O_RDONLY);
    
    char buffer[HEAD_RESERVED_SIZE];
    read(fd, buffer, HEAD_RESERVED_SIZE);
    if (!checking_for_null_term(buffer, HEAD_RESERVED_SIZE)) {
        ERR =  ERR_RESERVE;
        return NULL;
    }
    
    hfsplus *res = malloc(sizeof(hfsplus));
    res->vol_header = read_header_from_fd(fd);
    
    uint32_t catalog_file_offset = res->vol_header->blockSize * res->vol_header->catalogFile->extents[0].startBlock;
    res->catalog_file = create_btree_start_offset(fd, catalog_file_offset);
    
    hfsp_catalog_key *catalog_keys = malloc(sizeof(hfsp_catalog_key));
    uint32_t read_size = fill_key(res->catalog_file, ROOT_FOLDER_ID, catalog_keys);
    if (read_size <= 0) {
        ERR = ERR_WRONG_STRUCTURE;
        return NULL;
    }
    
    res->current_folder_info = get_folder_id(res->catalog_file);
    res->current_folder_id = res->current_folder_info->folderID;
    res->path_folder_depth = 1;
    res->pathFolderIds[0] = ROOT_FOLDER_ID;
    res->pathFolderIds[1] = res->current_folder_id;
    res->pwd = catalog_keys->node_name;
    //key.node_name -> pwd
    free(catalog_keys);
    return res;
}

//FUNCTION_2
char *pwd(hfsplus *fs) {
    return fs->pwd;
}

//for getting childs
uint32_t get_ls(hfsp_btree *btree_ptr, uint32_t item_id, char **buffer, int *type_file) {
    uint32_t index = get_header_record(btree_ptr)->firstLeafNode;

    hfsp_node *curr_node;
    uint32_t cnt = 0;
    while (index != 0) {
        curr_node = get_node(index, btree_ptr, true);
        hfsp_catalog_key *key;
        for (int j = 0; j < curr_node->descriptor->numRecords; ++j) {
            key = get_catalog_key(curr_node, j, btree_ptr, index);
            //first two bytes contains type_of_record
            int16_t type;
            read_int16(&type, btree_ptr->fd);
            if (key->parent_id == item_id && (type == kHFSPlusFolderRecord || type == kHFSPlusFileRecord) &&
                strcmp(key->node_name, "") != 0) {
                if(type == kHFSPlusFolderRecord){
                    *type_file = 0;
                }else if(type == kHFSPlusFileRecord){
                    *type_file = 1;
                }
                buffer[cnt++] = key->node_name;
                free(key);
            } else {
                free_key(key);
            }
        }
        index = curr_node->descriptor->fLink;
    }
    return cnt;
}
uint32_t ls(hfsplus *fs, char **buffer, int *is_file) {
    return get_ls(fs->catalog_file, fs->current_folder_id, buffer, is_file);
}


//for getting possition & type for cd 
int32_t get_possition(char *key_name, hfsp_btree *btree_ptr, uint32_t parent_id, int16_t *type) {
    int32_t index = get_header_record(btree_ptr)->firstLeafNode;
    hfsp_node *curr_node;

    while (index != 0) {
        curr_node = get_node(index, btree_ptr, true);
        for (int i = 0; i < curr_node->descriptor->numRecords; ++i) {
            hfsp_catalog_key *key = get_catalog_key(curr_node, i, btree_ptr, index);
            if (strlen(key_name) == key->sym_length && memcmp(key_name, key->node_name, key->sym_length) == 0 && key->parent_id == parent_id) {
                uint32_t offset = btree_ptr->btree_offset + index * get_header_record(btree_ptr)->nodeSize +
                                  curr_node->record_pointers[i] + KEY_SIZE_LENGTH + key->key_length;
                lseek(btree_ptr->fd, offset, SEEK_SET);

                read_int16(type, btree_ptr->fd);
                if (*type == kHFSPlusFileRecord || *type == kHFSPlusFolderRecord){
                    free_key(key);
                    return offset;
                }
            }
            free_key(key);
        }
        index = curr_node->descriptor->fLink;
    }
    return -1;
}

//FUNCTION_3
char *cd(hfsplus *fs, char *destination) {
    char *part_dest = strtok(destination, "/");
    while (part_dest != NULL) {
        int16_t type;
        int32_t offset = get_possition(part_dest, fs->catalog_file, fs->current_folder_id, &type);
        if (offset < 0) {
            ERR = ERR_CANT_FIND;
            return NULL;
        }
        if (type != kHFSPlusFolderRecord) {
            ERR = ERR_FIND_FILE;
            return NULL;
        }
        lseek(fs->catalog_file->fd, offset, SEEK_SET);
        fs->current_folder_info = get_folder_id(fs->catalog_file);
        fs->current_folder_id = fs->current_folder_info->folderID;
        fs->path_folder_depth++;
        fs->pathFolderIds[fs->path_folder_depth] = fs->current_folder_id;
        strcat(fs->pwd, "/");
        strcat(fs->pwd, part_dest);
        part_dest = strtok(NULL, "/");
    }
    return fs->pwd;
}

//FUNCTION_4
char *back(hfsplus *fs) {
    if (fs->path_folder_depth > 1) {
        fs->path_folder_depth--;
        fs->current_folder_id = fs->pathFolderIds[fs->path_folder_depth];
        for (uint32_t i = strlen(fs->pwd) - 1; i >= 0; --i) {
            if (fs->pwd[i] == '/') {
                fs->pwd[i] = '\0';
                break;
            }
        }
        return fs->pwd;
    } else {
        ERR = ERR_ON_ROOT;
        return NULL;
    }
}

//to copying file
hfsp_catalog_file *get_data_fork(hfsp_btree *btree_ptr) {
    hfsp_catalog_file *file = malloc(sizeof(hfsp_catalog_file));
    lseek(btree_ptr->fd, 32, SEEK_CUR);
    
    lseek(btree_ptr->fd, 48, SEEK_CUR);
    
    lseek(btree_ptr->fd, 8, SEEK_CUR);
    file->dataFork = *read_fork_data_from_fd(btree_ptr->fd);
    lseek(btree_ptr->fd, 80, SEEK_CUR);
    return file;
}

int32_t cpf(char *destination, hfsplus *fs, int32_t offset) {
    if (offset < 0) {
        ERR = ERR_CANT_FIND;
        return -1;
    }
    lseek(fs->catalog_file->fd, offset, SEEK_SET);
    hfsp_catalog_file *file = get_data_fork(fs->catalog_file);
    char data[file->dataFork.logicalSize];
    if (file->dataFork.logicalSize > fs->vol_header->blockSize) {
        for (int i = 0; i < 8; ++i) {
            uint32_t data_offset = fs->vol_header->blockSize * (file->dataFork.extents[i].startBlock);
            uint32_t to_read = fs->vol_header->blockSize * file->dataFork.extents[i].blockCount;
            lseek(fs->catalog_file->fd, data_offset, SEEK_SET);
            read(fs->catalog_file->fd, data, to_read);
        }
    } else {
        uint32_t data_offset = fs->vol_header->blockSize * (file->dataFork.extents[0].startBlock);
        lseek(fs->catalog_file->fd, data_offset, SEEK_SET);
        read(fs->catalog_file->fd, data, file->dataFork.logicalSize);
    }

    int32_t dest_fd = open(destination, O_RDWR|O_CREAT, 0666);
    if (dest_fd != -1) {
        write(dest_fd, data, file->dataFork.logicalSize);
        close(dest_fd);
        return 0;
    } else {
        return -1;
    }
}

int32_t cpd(char *destination, hfsplus *fs, int32_t offset) {
    lseek(fs->catalog_file->fd, offset, SEEK_SET);
    hfsp_catalog_folder *folder = get_folder_id(fs->catalog_file);
    mkdir(destination, 0777);
    char *buffer[100];
    int *temp = malloc(sizeof(int));;
    int32_t found = get_ls(fs->catalog_file, folder->folderID, buffer, temp);

    for (int i = 0; i < found; ++i) {
        int16_t type;
        offset = get_possition(buffer[i], fs->catalog_file, folder->folderID, &type);
        char new_path[200] = {0};
        strcat(new_path, destination);
        strcat(new_path, "/");
        strcat(new_path, buffer[i]);
        if (type == kHFSPlusFolderRecord) {
            cpd(new_path, fs, offset);
        } else {
            cpf(new_path, fs, offset);
        }
    }
    return 0;
}

//FUNCTION_4
int32_t copy(char *name, char *destination, hfsplus *fs) {
    int16_t type;
    int32_t offset = get_possition(name, fs->catalog_file, fs->current_folder_id, &type);
    if (offset == -1) {
        ERR = ERR_CANT_FIND;
        return -1;
    } else {
        if (type == kHFSPlusFolderRecord) {
            return cpd( destination, fs, offset);
        } else {
            return cpf(destination, fs, offset);
        }
    }
}
