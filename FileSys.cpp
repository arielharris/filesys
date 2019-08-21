// CPSC 3500: File System
// Implements the file system commands that are available to the shell.

#include <cstring>
#include <iostream>
using namespace std;

#include "FileSys.h"
#include "BasicFileSys.h"
#include "Blocks.h"

// mounts the file system
void FileSys::mount() {
  bfs.mount();
  curr_dir = 1;
}

// unmounts the file system
void FileSys::unmount() {
  bfs.unmount();
}

// make a directory
void FileSys::mkdir(const char *name)
{
	//check name length 
	if (strlen(name) > MAX_FNAME_SIZE){
		cerr << "Name is too long." << endl;
		return;
	}
	
	short new_block_num = bfs.get_free_block();
	
	// Check if disk is full
	if (new_block_num == 0) {
		cerr << "Disk is full!" << endl;
		return;
	}
	
	//access root directory
	dirblock_t parent_block;
	bfs.read_block(curr_dir, (void*) &parent_block);
	
	
	for (int i = 0; i < parent_block.num_entries; i++){
		if (strcmp(name, parent_block.dir_entries[i].name) == 0){
			cerr << "Directory already exists" << endl;
			return;
		}
	}

	
	num_dir = parent_block.num_entries;
	if (num_dir == MAX_DIR_ENTRIES){
		cerr << "Directory is full" << endl;
		return;
	}
	
	//initialize new directory
	dirblock_t new_dir_block;
	new_dir_block.magic = DIR_MAGIC_NUM;
	new_dir_block.num_entries = 0;
	bfs.write_block(new_block_num, (void *) &new_dir_block);

	//update parent directory
	parent_block.dir_entries[num_dir].block_num = new_block_num;
	for (int i = 0; i < strlen(name); ++i){
		parent_block.dir_entries[num_dir].name[i] = name[i];
	}
	parent_block.num_entries++;
	bfs.write_block(curr_dir, (void*) &parent_block);
	cout << "directory made" << endl;
}

// switch to a directory
void FileSys::cd(const char *name)
{
	short keep_dir = curr_dir;
	dirblock_t curr_block;
	bfs.read_block(curr_dir, (void*) &curr_block);
	for (int i = 0; i < curr_block.num_entries; i++){
		char* dir_name = curr_block.dir_entries[i].name;
		if (strcmp(dir_name, name) == 0){
			dirblock_t next_block;
			short next_dir = curr_block.dir_entries[i].block_num;
			bfs.read_block(next_dir, (void*) &next_block);
			if(next_block.magic != DIR_MAGIC_NUM){
				cerr << name << " is not a directory" << endl;
				return;
			}
			curr_dir = next_dir;
			cout << "In " << name << " directory" << endl;
			break;
		}
	}
	if (keep_dir == curr_dir)
		cerr << "Directory " << name << " not found" << endl;
}

// switch to home directory
void FileSys::home() 
{
	curr_dir = 1;
}

// remove a directory
void FileSys::rmdir(const char *name)
{
	dirblock_t curr_block;
	bfs.read_block(curr_dir, (void*) &curr_block);
	for (int i = 0; i < curr_block.num_entries; i++){
		char* dir_name = curr_block.dir_entries[i].name;
		if (strcmp(dir_name, name)==0){
			short rmv_dir = curr_block.dir_entries[i].block_num;				
			dirblock_t rmv_block;
			bfs.read_block(rmv_dir, (void*) &rmv_block);
			
			//check for cases
			if (rmv_block.num_entries != 0){
				cerr << "Directory is not empty" << endl;
				return;
			}
			
			if(rmv_block.magic != DIR_MAGIC_NUM){
				cerr << name << " is not a directory" << endl;
				return;
			}
			
			if (i != curr_block.num_entries - 1){
				for (int j = i; j < curr_block.num_entries; j++){
					strcpy(curr_block.dir_entries[j].name, curr_block.dir_entries[j+1].name);
					curr_block.dir_entries[j].block_num = curr_block.dir_entries[j+1].block_num;
				}
			}
			
			bfs.reclaim_block(rmv_dir);
			curr_block.num_entries--;
			bfs.write_block(curr_dir, (void*) &curr_block);
			cout << "Directory " << name << " removed" << endl;
			return;
		}
	}
	cerr << "Directory " << name << " not found" << endl;
}

// list the contents of current directory
void FileSys::ls()
{
	dirblock_t curr_block;
	bfs.read_block(curr_dir, (void*) &curr_block);
	
	for (int i = 0; i < curr_block.num_entries; i++) {
		short entry_num = curr_block.dir_entries[i].block_num;
		dirblock_t entry_dir;
		inode_t entry_node;
		bfs.read_block(entry_num, (void*) &entry_dir);
		bfs.read_block(entry_num, (void*) &entry_node);
		if(entry_dir.magic == DIR_MAGIC_NUM)
			cout << curr_block.dir_entries[i].name << '/' << endl;
		else if(entry_node.magic == INODE_MAGIC_NUM)
			cout << curr_block.dir_entries[i].name << endl; 
	}
}

// create an empty data file
void FileSys::create(const char *name)
{
	if (strlen(name) > MAX_FNAME_SIZE) {
		cerr << "Name of this file is too long." << endl;
		return;
	}
	
	short new_file_num = bfs.get_free_block();
	if (new_file_num == 0){
		cerr << "Disk is full" << endl;
		return;
	}
	
	//access root directory
	dirblock_t parent_block;
	bfs.read_block(curr_dir, (void*) &parent_block);
	
	for (int i = 0; i < parent_block.num_entries; i++){
		char* dir_name = parent_block.dir_entries[i].name;
		if (strcmp(dir_name, name)==0){
			cerr << name << "already exists" << endl;
			return;
		}
	}	
	
	num_dir = parent_block.num_entries;
	if (num_dir == MAX_DIR_ENTRIES){
		cerr << "Directory is full" << endl;
		return;
	}
	
	inode_t new_i_node;
	new_i_node.magic = INODE_MAGIC_NUM;
	new_i_node.size = 0;
	for (int i  = 0; i < MAX_DATA_BLOCKS; i ++) 
		new_i_node.blocks[i] = 0;
	bfs.write_block(new_file_num, (void*) &new_i_node);
	
		
	parent_block.dir_entries[num_dir].block_num = new_file_num;
	for (int i = 0; i < strlen(name); ++i){
		parent_block.dir_entries[num_dir].name[i] = name[i];
	}
	parent_block.num_entries++;

	bfs.write_block(curr_dir, (void*) &parent_block);
}

// append data to a data file
void FileSys::append(const char *name, const char *data)
{
	int size = strlen(data);
		
	if (size > MAX_FILE_SIZE)
	{
		cerr << "Data exceeds maximum file size" << endl;
		return;
	}
	
	dirblock_t dir_block;
	bfs.read_block(curr_dir, (void*) &dir_block);

	for (int i = 0; i < dir_block.num_entries; i++){
		char* file_name = dir_block.dir_entries[i].name;
		if (strcmp(file_name, name)==0){
			short file_num = dir_block.dir_entries[i].block_num;
			inode_t file_node;
			datablock_t new_data_block;
			bfs.read_block(file_num, (void*) &file_node);
		
			if (file_node.magic != INODE_MAGIC_NUM){
				cerr << name << "is a directory" << endl;
				return;
			}
		
			int block_index = size / BLOCK_SIZE;
			int data_index = size % BLOCK_SIZE;
			
			int h = 0;
			
			if (file_node.size == 0)
			{
				short new_data_num = bfs.get_free_block();
				
				//check if disk is full
				if(new_data_num == 0)
				{
					cerr << "Disk is full" << endl;
					return;
				}
				
				file_node.blocks[block_index] = new_data_num;
			}
		
		
			while (h < size &&  block_index + 1 <= MAX_DATA_BLOCKS) {	
				bfs.read_block(file_node.blocks[block_index], (void*) & new_data_block);
			
				if (file_node.size + size > MAX_FILE_SIZE)
				{
					cerr << "File exceeds maximum size" << endl;
					return;
				}
				
				if (data_index < BLOCK_SIZE)
				{
					new_data_block.data[data_index] = data[h];
					data_index++;
					h++;
					file_node.size++;
					bfs.write_block(file_node.blocks[block_index], (void*) &new_data_block);
				}
				
				if (data_index == BLOCK_SIZE && h < size)
				{
					data_index = 0;
					block_index++;
					short new_free_block = bfs.get_free_block();
					
					if(new_free_block == 0)
					{
						cerr << "Disk is full" << endl;
						return;
					}
					
					file_node.blocks[block_index] = new_free_block;
				}
			}
		bfs.write_block(file_num, (void*) &file_node);
		return;
		}
	}
	cerr << "File not found" << endl;
}

// display the contents of a data file
void FileSys::cat(const char *name)
{
	dirblock_t dir_block;
	bfs.read_block(curr_dir, (void*) &dir_block);
	for (int i = 0; i < dir_block.num_entries; i++){
		char* file_name = dir_block.dir_entries[i].name;
		if (strcmp(file_name, name) == 0){
			short file_num = dir_block.dir_entries[i].block_num;
			inode_t file_node;
			datablock_t data_block;
			bfs.read_block(file_num, (void*) &file_node);
			
			int blocks = 0;
			if (file_node.size % BLOCK_SIZE == 0)
				blocks = file_node.size/BLOCK_SIZE;
			else
				blocks = file_node.size/BLOCK_SIZE + 1;
			
			if (file_node.magic != INODE_MAGIC_NUM) {
				cerr << "File is dirctory" << endl;
				return;
			}
			
			for (int k = 0; k < blocks; k++){
				bfs.read_block(file_node.blocks[k], (void*) &data_block);
				
				if (k < NUM_BLOCKS - 1)
				{
					for (int j = 0; j < BLOCK_SIZE; j++)
						cout << data_block.data[j];
				} else if (k == blocks - 1) {
					if (file_node.size % BLOCK_SIZE != 0)
					{
						for (int j = 0; j < file_node.size % BLOCK_SIZE; j++)
							cout << data_block.data[j];
					} else {
					for (int j  = 0; j < BLOCK_SIZE; j++)
						cout << data_block.data[j];
					}
				}
			}
			cout << endl;
			return;
		}
	}
	cerr << "File not found" << endl;
}
	

// display the first N bytes of the file
void FileSys::head(const char *name, unsigned int n)
{
	dirblock_t dir_block;
	bfs.read_block(curr_dir, (void*) &dir_block);
	for (int i = 0; i < dir_block.num_entries; i++){
		char* file_name = dir_block.dir_entries[i].name;
		if (strcmp(file_name, name)==0){
			short file_num = dir_block.dir_entries[i].block_num;
			inode_t file_node;
			bfs.read_block(file_num, (void*) &file_node);
			
			if (file_node.magic == DIR_MAGIC_NUM){
				cerr << name << " is a directory" << endl;
				return;
			}
			
			if (n >= file_node.size){
				cat(name);
				return;
			}
			
			if (n <= BLOCK_SIZE){
				int data_index = file_node.size % BLOCK_SIZE;
				short data_num = file_node.blocks[data_index];
				struct datablock_t curr_block;
				bfs.read_block(data_num, (void*) &curr_block);
				for (int i = 0; i < n; i++)
					cout << curr_block.data[i];
				cout << endl;
				return;
				
			} else {
				int blocks = 0;
				if (file_node.size % BLOCK_SIZE == 0)
					blocks = file_node.size / BLOCK_SIZE;
				else
					blocks = file_node.size / BLOCK_SIZE + 1;
				
				for (int i = 0; i < blocks; i++){
					struct datablock_t curr_block;
					short data_num = file_node.blocks[i];
					bfs.read_block(data_num, (void*) &curr_block);
					
					if (i < blocks - 1){
						for (int j = 0; j < BLOCK_SIZE; j++)
							cout << curr_block.data[i];
						return;
					} else if (i == blocks - 1) {
						//partial block
						if (file_node.size % BLOCK_SIZE != 0){
							for (int j = 0; j < file_node.size % BLOCK_SIZE; j++)
								cout << curr_block.data[j];
							cout << endl;
							return;
							
						} else { 
							for (int j = 0; j < BLOCK_SIZE; j++)
								cout << curr_block.data[j];
							cout << endl;
							return;
						}
					}
				}
			}
		}
		cout << endl;
		return;
	}
	cerr << "File not found" << endl;
}

// delete a data file
void FileSys::rm(const char *name)
{
	int keep_dir = curr_dir;
	dirblock_t dir_block;
	bfs.read_block(keep_dir, (void*) &dir_block);
	for (int i = 0; i < dir_block.num_entries; i++){
		char* rmv_name = dir_block.dir_entries[i].name;
		if (strcmp(rmv_name, name)==0){
			short rmv_num = dir_block.dir_entries[i].block_num;
			inode_t rmv_file;
			bfs.read_block(rmv_num, (void*) &rmv_file);
			
			if(rmv_file.magic != INODE_MAGIC_NUM){
				cerr << name << " is a directory" << endl;
				return;
			}
			
			int blocks = 0;
			if (rmv_file.size % BLOCK_SIZE == 0)
				blocks = rmv_file.size/BLOCK_SIZE;
			else
				blocks = rmv_file.size/BLOCK_SIZE + 1;
			
			for (int i = 0; i < blocks; i++)
				bfs.reclaim_block(rmv_file.blocks[i]);
			
			bfs.reclaim_block(rmv_num);
			
			if (i != dir_block.num_entries - 1){
				for (int j = i; i < dir_block.num_entries; j++){
					strcpy(dir_block.dir_entries[j].name, dir_block.dir_entries[i+1].name);
					dir_block.dir_entries[i].block_num = dir_block.dir_entries[i+1].block_num;
				}
			}
			
			dir_block.dir_entries[dir_block.num_entries - 1].block_num = 0;
			dir_block.num_entries--;
			bfs.write_block(keep_dir, (void*) &dir_block);
			return;
		}
	}
	cerr << "File not found" << endl;		
}

// display stats about file or directory
void FileSys::stat(const char *name)
{
	dirblock_t dir_block;
	bfs.read_block(curr_dir, (void*) &dir_block);
	for (int i = 0; i < dir_block.num_entries; i++){
		char* dir_name = dir_block.dir_entries[i].name;
		if (strcmp(dir_name, name)==0){
			short stat_num = dir_block.dir_entries[i].block_num;
			dirblock_t stat_dir;
			inode_t stat_node;
			bfs.read_block(stat_num, (void*) &stat_dir);
			bfs.read_block(stat_num, (void*) &stat_node);
			if(stat_dir.magic == DIR_MAGIC_NUM){
				cout << "Directory Name: " << name << "/" << endl;
				cout << "Directory Block: " << stat_num << endl;
				return;
			} else if (stat_node.magic == INODE_MAGIC_NUM) {
				cout << "i-node Block: " << stat_num << endl;
				cout << "Bytes in File: " << stat_node.size << endl;
				int blockCount = 0;
				if (stat_node.size % BLOCK_SIZE == 0)
					blockCount = stat_node.size / BLOCK_SIZE;
				else
					blockCount = stat_node.size / BLOCK_SIZE + 1;
				cout << "Number of Blocks: " << blockCount << endl;
				cout << "First Block: " << curr_dir << endl;
				return;
			}
		}
	}
	cerr << name << " not found" << endl;
}

// HELPER FUNCTIONS (optional)
