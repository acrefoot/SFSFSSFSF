#include <fuse.h>
#include <stdio.h>
#include <iostream>
#include "FuseService.h"

using namespace std;

//discard 4 lowest bits; use remaining static hash to indentify file.
static string sha256(string path){
	ostringstream command;
	uint16_t pcm_buf[SFSFSSFSF_CHUNK];
	uint16_t bytes_read;
	int i;
	
	command << "ffmpeg -i \"" << path << "\" -f "<<SFSFSSFSF_FORMAT<<" pipe:";
	FILE *pipein = popen(command, 'r');
	if (!pipein) err("Could not open ffmpeg");
	decode_bits(pipein, (uint8_t *) &pfi, sizeof(struct pstat));
	
	fread(pcm_buf,2,SFSFSSFSF_CHUNK,pipein);
	pclose(pipein);
	
	for (i=0;i<SFSFSSFSF_CHUNK;i++)
		bytes_read[i] = pcm_buf[i]>>4;

	// TODO: send to Crypt thread for result.

}

// _init()_       starts all threads (called by _fuse_main_), parses M3U playlist for non-superblock files
static void *fuse_service_init ()
{

	// TODO: Start up Encryption Threads
	
	string line;
	ifstream playlist_file (audiofile_list_file);
	
	playlist_file.open();
	while(playlist_file.good()){
		getline(playlist_file, line);
		MapOfAllFiles[sha256(line)] = line;
#ifdef DEBUG
		cout<<"Playlist file line:"<<line<<endl;
#endif
	}

	playlist_file.close();
	// TODO: add things to parse in from the superblock;
	string rootfile, freefile;
	ifstream SuperBlock(superblock_file);
	// TODO: ensure the right crypt header length is skipped.
	SuperBlock.seekg(SB_CRYPT_HDR_LENGTH);
	SuperBlock >> rootfile >> freefile;
	MapOfAllPaths['/']=rootfile;
	string curfreefile = freefile;
	while(curfreefile != ""){
		ifstream FreeFile(curfreefile);

		// TODO: make this list all the freefiles. Single master list, or SLL?
	}
	
	//MapOfDirFiles[key].push_back(filename);
	return NULL;
}
// _access()_      do nothing. Assume if we can decrypt, we can access. Maybe better not to implement, in case of readonly? (what's the right answer here?)

static int fuse_service_access (const char *path, int mask) {return -E_SUCCESS;}

// _create()_      add to directory special file, and remove from last freefile specialfile in SLL (singly-linked list). change dirfile

static int fuse_service_create (const char *path, mode_t mode, struct fuse_file_info *fi){
	
	string afile = FreeList.front();
	FreeList.pop_front();
	FreeFileBitmap[afile] = 0;
	
	MapOf



		return -E_SUCCESS;
}


// _mkdir()_       Create a new directory special file, add it to the parent directory's special file. change dirfile
// _unlink()_      change dirfile, add to freelist.
// _rmdir()_       Removes empty directory. May need to return //-ENOTEMPTY//: Directory not empty. Change dirfile, add to freelist
// _rename()_      change dirfile. Don't know how to deal with renaming to different level directory. Maybe we just don't support this.



// _destroy()_     takes return value of init. Should do an _fsync_, then shutdown all ThreadPools
// _getattr()_     if (seebelow),query Data structures above, fill in stbuf. Return //-ENOENT// if file does not exist.

if( (err = path_exists(path)) ) return err;

// _open()_        returns //-EACCES//, but not really. returns _path_exists()_
static int fuse_service_open(const char *path, struct fuse_file_info *fi){
	try {
		SFSFSSFSF_File *f = new SFSFSSFSF_File(superblock_file, NULL);
		fi->fh = (uint64_t)f;
		return -E_SUCCESS;
	}
	catch (char *e) return print_err(e);
  
}

// _read()_        (following maintains argument order) from rfile at //path//, fill //buf//, with //size// bytes, starting from //offset// to //offset//+size. Read directly from dirty cache if available.
static int fuse_service_read(const string path, char *buf, size_t size,
                             off_t offset, struct fuse_file_info *fi)
{
	SFSFSSFSF_File *f = (SFSFSSFSF_File *)(fi->fh);

	try f->read(offset, size, (uint8_t *)buf);
	catch (char *e) return print_err(e);


	return 0;
}


// _readdir()_     Something like below.

if( (err = path_exists(path)) ) return err;
filler (buf, ".", NULL, 0);
filler (buf, "..", NULL, 0);
filler (buf, "bazbar", NULL, 0);
filler (buf, "foobar", NULL, 0);

// _write()_       if offset is not zero, don't have a good way to determine clobbering order. Thus, force a _fsync_ if there is an offset write.
//// If there is nonzero offset: read original file up to offset, then copy partial into new buffer, append given buffer[0:size].
//// if offset==0: write given buffer[0:size]
//// queue an _fsync_ if the time is right (what this means is up for determination).
// _fsync()_       write dirty cache. AKA batch job to encryption threads.
