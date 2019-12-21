# Xv6_FileChecker: Implementing Print_Directory Tree as well as checking Inode link consistency in Xv6 Operating System

## 1)check_image():
	This function takes in one parameter which is the path of the fs.img. 
  It loads the fs.img into the system. Furthermore, information such as 
  the base address of the image pointer, base address of superblock, base address of inode 
  and the base address of the root inode are initialized.
## 2)show_directory():
	This function takes in 3 parameters: 
  First one is the path (root), the second one is the inode (root inode), and the third one is an integer indicating the current level of the file or directory. Basically, this function is doing a depth first search starting from the root directory. Note that for xv6, there are 12 direct entry blocks and 1 indirect entry block. So we need to separate into two cases since the direct entry blocks contain the blk number of the datablock while the indirect block points to a bunch of inode pointer. Besides such distinction, implementations are pretty similar. For readability, if a name is a type of directory, I added “(d)” after the name as an identifier for a directory.
## 3)check_links():
	This function consists of two parts. 
  The first part is pretty similar to show_directory(). Additionally, a map, which is basically an array (key/index: inum, value: nlinks) is passed into the function so that when traversing the file system, the number of links pointing to different inodes can be recorded. The second part is to traverse all the inodes and then compare their field nlink with their corresponding record contained in the map. Furthermore, when the links equals to 0 for an inode but at least one of its data blocks are allocated, an error will be thrown. (Note normally the file should be moved to the /lost+found but I did not include such functionality) 
## Reminder: 
I have not written failure-case testing due to time constraint. On the other hand, everything works as expected under non-failure cases.
## Example of Execution:

#### make qemu-nox
#### mkdir test 			#Create a directory called test under root
#### mkdir test/test			#Create a directory under test created above and called it test
#### echo “hello” > test/test.txt          #Create a file under our first test directory
#### echo “hello” > test/test/test.txt   #Create a directory under our second test directory
#### Comment: Exit xv6 (Crtl+A+q)
#### Comment: Goes to the directory that you download the code
#### gcc main.c -o main, execute      #Compile the code
#### Comment:(if you copy fs.img under the same directory as the executable file main)
#### ./main fs.img 		            #Execute 

## After your execution, you should expect to have the following result
![](https://github.com/yingjianwu199868/Xv6_FileChecker/blob/master/Screen%20Shot%202019-12-21%20at%203.22.08%20PM.png)
