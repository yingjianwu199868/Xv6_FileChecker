#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdlib.h>
 
 
// copy from fs.h
 
// On-disk file system format.
// Both the kernel and user programs use this header file.
 
 
#define ROOTINO 1  // root i-number
#define BSIZE 512  // block size
 
// Disk layout:
// [ boot block | super block | log | inode blocks |
//                                          free bit map | data blocks]
//
// mkfs computes the super block and builds an initial file system. The
// super block describes the disk layout:
struct superblock {
 uint size;         // Size of file system image (blocks)
 uint nblocks;      // Number of data blocks
 uint ninodes;      // Number of inodes.
 uint nlog;         // Number of log blocks
 uint logstart;     // Block number of first log block
 uint inodestart;   // Block number of first inode block
 uint bmapstart;    // Block number of first free map block
};
 
#define NDIRECT 12
#define NINDIRECT (BSIZE / sizeof(uint))
#define MAXFILE (NDIRECT + NINDIRECT)
 
// On-disk inode structure
struct dinode {
 short type;           // File type
 short major;          // Major device number (T_DEV only)
 short minor;          // Minor device number (T_DEV only)
 short nlink;          // Number of links to inode in file system
 uint size;            // Size of file (bytes)
 uint addrs[NDIRECT+1];   // Data block addresses
};
 
// Inodes per block.
#define IPB           (BSIZE / sizeof(struct dinode))
 
// Block containing inode i
#define IBLOCK(i, sb)     ((i) / IPB + sb.inodestart)
 
// Bitmap bits per block
#define BPB           (BSIZE*8)
 
// Block of free map containing bit for block b
#define BBLOCK(b, sb) (b/BPB + sb.bmapstart)
 
// Directory is a file containing a sequence of dirent structures.
#define DIRSIZ 14
 
struct dirent {
 ushort inum;
 char name[DIRSIZ];
};
 
// copy from stat.h
#define T_DIR  1   // Directory
#define T_FILE 2   // File
#define T_DEV  3   // Device
 
 
/*
My Code Starts from here
*/
 
void* image_base = NULL; //base address of the image
struct superblock* sb = NULL; //base address of the superblock
void* inode_start_base = NULL; //base address of the inode (Note not the root)
 
//Entry function
static int check_image(const char* image_path);
//Function that Print the directory tree
static void show_directory(const char* name, const struct dinode* inode, int level);
//Function that Print the inode number and the count of the links
static void check_links();
 
 
int main(int argc, char* argv[])
{
   if (argc < 2) {
       fprintf(stderr, "Need a file image path");
       return 1;
   }
 
   return check_image(argv[1]);
}
 
int check_image(const char* image_path)
{
   int fd = open(image_path, O_RDWR, 0);
   if (fd < 0) {
       perror("open system call failure for the image path");
       return 1;
   }
 
   struct stat st;
   if (fstat(fd, &st)) {
       perror("stat system call failure for the image path");
       return 1;
   }
 
   image_base = mmap(NULL, st.st_size, PROT_READ|PROT_WRITE, MAP_PRIVATE, fd, 0);
   if (image_base == MAP_FAILED) {
       perror("mmap failure for the file image");
       return 1;
   }
 
   //In XV6, SuperBlock starts at BSIZE, the first BSIZE are for booting
   sb = (struct superblock*)((char*)image_base + BSIZE);
 
   //Note that after Booting and Superblock, there are blocks for logs
   inode_start_base = (void*)((char*)image_base + ((1 + 1 + sb->nlog) * BSIZE));
 
   struct dinode* root = (struct dinode*)inode_start_base + ROOTINO;
 
   //check if root Inode is directory
   if (root->type != T_DIR) {
       fprintf(stderr, "Bad fs image, root is not a dir\n");
       munmap(image_base, st.st_size);
       close(fd);
       return 1;
   }
 
   //Print the Directory Tree
   show_directory("/", root, 0);
 
   //Print out the Links from inode and its corresponding count from traversal
   check_links();
 
   munmap(image_base, st.st_size);
   close(fd);
 
   return 0;
}
 
static void print_space(int count)
{
   int i=0;
   for (i=0; i<count; i++) {
       printf("    ");
   }
}
 
 
void show_directory(const char* name, const struct dinode* inode, int level)
{
   int i, j;
 
   print_space(level);
   if (inode->type == T_DIR) {
       printf("%s(d)\n", name);
       //addr is the datablock blk#
       uint addr = 0;
       //Direct pointer to databalock
       for (i = 0; i < NDIRECT; i++) {
           uint addr = inode->addrs[i];
           if (addr == 0) {
               continue;
           }
           //Go to the base address of the DataBlock
           struct dirent* entry = (struct dirent*)((char*)image_base + (addr * BSIZE));
           //traverse all the entries, which are files or directory
           for (j = 0; j < (BSIZE/sizeof(struct dirent)); j++) {        
               if (entry->inum == 0 || entry->name[0] == '.') {
                   entry++;
                   continue;
               }
               struct dinode* child_inode = (struct dinode *)inode_start_base + entry->inum;
               show_directory(entry->name, child_inode, level+1);
               entry++;
           }
       }
 
       // indirect
       addr = inode->addrs[NDIRECT];
       if (addr == 0) {
           return;
       }
 
       //note each individual entry in the inndirect block is a inum_pointer
       uint* inum_ptr = (uint*)((char*)image_base + (addr * BSIZE));
       for (i = 0; i < NINDIRECT; i++) {
           //grab the blockNumber
           addr = *(inum_ptr++);
           if (addr == 0) {
               continue;
           }
 
           struct dirent* entry = (struct dirent*)((char*)image_base + (addr * BSIZE));
           for (j = 0; j < (BSIZE/sizeof(struct dirent)); j++) {
               // printf("file:%s\n", entry->name);
               if (entry->inum == 0 || entry->name[0] == '.') {
                   entry++;
                   continue;
               }
 
               struct dinode* child_inode = (struct dinode *)inode_start_base + entry->inum;
               show_directory(entry->name, child_inode, level+1);
               entry++;
           }
       }
 
   } else {
       printf("%s\n", name);
   }
}
 
//Similar to show directory with small modification
static void traverse_fs(const struct dinode* inode, uint* reference_count_map)
{
   int i, j;
   if (inode->type == T_DIR ) {
       uint addr = 0;
       for (i = 0; i < NDIRECT; i++) {
           uint addr = inode->addrs[i];
           if (addr == 0) {
               continue;
           }
 
           struct dirent* entry = (struct dirent*)((char*)image_base + (addr * BSIZE));
           for (j = 0; j < (BSIZE/sizeof(struct dirent)); j++) {
              
               if (entry->inum == 0 || entry->name[0] == '.') {
                   if (entry->name[1] != '.') {
                       entry++;
                   } else {
                       //reference back to parent needs to be counted
                       reference_count_map[entry->inum] += 1;
                       entry++;
                   }
                   continue;
               }
 
               //record the link counts
               reference_count_map[entry->inum] += 1;
 
               struct dinode* child_inode = (struct dinode *)inode_start_base + entry->inum;
               traverse_fs(child_inode, reference_count_map);
               entry++;
           }
       }
 
       // indirect
       addr = inode->addrs[NDIRECT];
       if (addr == 0) {
           return;
       }
 
       uint* inum_ptr = (uint*)((char*)image_base + (addr * BSIZE));
       for (i = 0; i < NINDIRECT; i++) {
           addr = *(inum_ptr++);
           if (addr == 0) {
               continue;
           }
 
           struct dirent* entry = (struct dirent*)((char*)image_base + (addr * BSIZE));
           for (j = 0; j < (BSIZE/sizeof(struct dirent)); j++) {
               // printf("file:%s\n", entry->name);
               if (entry->inum == 0 || entry->name[0] == '.') {
                   if (entry->name[1] != '.') {
                       entry++;
                   } else {
                       reference_count_map[entry->inum] += 1;
                       entry++;
                   }
                   continue;
               }
 
               reference_count_map[entry->inum] += 1;
               struct dinode* child_inode = (struct dinode *)inode_start_base + entry->inum;
               traverse_fs(child_inode, reference_count_map);
               entry++;
           }
       }
   }
}
 
 
void check_links()
{
   int i,j;
   //Basically an Array where index is the inum
   uint* reference_count_map = (uint*)calloc(sb->ninodes, sizeof(uint));
 
   //Start recording links from the root
   struct dinode* root = (struct dinode*)inode_start_base + ROOTINO;
   traverse_fs(root, reference_count_map);
 
   //Traverse individual inodes to compare their links with the result we get after the traversal
   struct dinode* inode = (struct dinode *)inode_start_base + ROOTINO;
   for (i = 1; i < sb->ninodes; i++) {
       //TODO: Move to Lost + Found
       if (inode->nlink == 0) {
           int unallocated = 0;
           for (j = 0; j < NDIRECT; j++) {
               if (inode -> addrs[j] != 0) {
                   fprintf(stderr, "Move to Lost and Found: inode:%d link=0\n", j);
                   break;
               }
           }
           inode++;
       } else {
           if (reference_count_map[i] != inode->nlink) {
               fprintf(stderr, "inode%d link and reference count doesnot match, link:%d, reference_count:%d\n", i, inode->nlink, reference_count_map[i]);
               inode++;
           } else {
               printf("Inode%d link and reference count matches, link:%d, reference_count:%d\n", i, inode->nlink, reference_count_map[i]);
               inode++;
               }
       }
   }
}
 


