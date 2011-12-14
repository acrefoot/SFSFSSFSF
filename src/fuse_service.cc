#include <sfsfssfsf.h>
#include <fuse_service.h>

using namespace std;

//hash->apath
map <string, string> MapOfAllFiles;
//rpath->hash
map <string, string> MapOfAllPaths;
//hash->vector<hash>
map <string, vector<string> > MapOfDirFiles;
////hash->vector<hash>
//map <string, vector<string> > MapOfFreeFiles;
//hash->int (boolean)
map <string, int> FreeFileBitmap;
list <string> FreeList;
//hash->int (boolean)
map <string, int> DirFileDirtyBitmap;

string audiofile_list_file;
string superblock_file;

//discard 4 lowest bits; use remaining static hash to indentify file.
static string sha256(string path)
{
	debug_print("In sha256");
	ostringstream command;
	static uint16_t pcm_buf[SFSFSSFSF_CHUNK];
	static struct pstat pfi;
	int i;
	
	command << "ffmpeg -i \"" << path << "\" -f u16le pipe:";
	FILE *pipein = popen(command.str().c_str(), "r");
	if (!pipein) err("Could not open ffmpeg");
	decode_bits(pipein, (uint8_t *) &pfi, sizeof(struct pstat));
	
	fread(pcm_buf,2,SFSFSSFSF_CHUNK,pipein);
	pclose(pipein);
	
	for (i=0;i<SFSFSSFSF_CHUNK;i++)
		pcm_buf[i] = pcm_buf[i]>>4;

	// TODO: send to Crypt thread for result.
	return string("");
}

// _init()_       starts all threads (called by _fuse_main_), parses M3U playlist for non-superblock files
static void *fuse_service_init (struct fuse_conn_info *conn)
{
	debug_print("In init()\n");
	// TODO: Start up Encryption Threads
	
	string line;
	ifstream playlist_file (audiofile_list_file.c_str());
	
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
	ifstream SuperBlock(superblock_file.c_str());
	// TODO: ensure the right crypt header length is skipped.
	SuperBlock.seekg(SB_CRYPT_HDR_LENGTH);
	SuperBlock >> rootfile >> freefile;
	MapOfAllPaths["/"]=rootfile;
	string curfreefile = freefile;
	while(curfreefile != ""){
		ifstream FreeFile(curfreefile.c_str());

		// TODO: make this list all the freefiles. Single master list, or SLL?
	}
	
	//MapOfDirFiles[key].push_back(filename);
	return NULL;
}
// _access()_      do nothing. Assume if we can decrypt, we can access. Maybe better not to implement, in case of readonly? (what's the right answer here?)

static int fuse_service_access (const char *path, int mask) {return -E_SUCCESS;}

// _create()_      add to directory special file, and remove from last freefile specialfile in SLL (singly-linked list). change dirfile

static int fuse_service_create (const char *path, mode_t mode, struct fuse_file_info *fi)
{

	debug_print("In create()\n");
	string afile = FreeList.front();
	FreeList.pop_front();
	FreeFileBitmap[afile] = 0;
	
	return -E_SUCCESS;
}


// _mkdir()_       Create a new directory special file, add it to the parent directory's special file. change dirfile
// _unlink()_      change dirfile, add to freelist.
// _rmdir()_       Removes empty directory. May need to return //-ENOTEMPTY//: Directory not empty. Change dirfile, add to freelist
// _rename()_      change dirfile. Don't know how to deal with renaming to different level directory. Maybe we just don't support this.
// _destroy()_     takes return value of init. Should do an _fsync_, then shutdown all ThreadPools

// _getattr()_     if (seebelow),query Data structures above, fill in stbuf. Return //-ENOENT// if file does not exist.

static int fuse_service_getattr(const char *path, struct stat *stbuf)
{
	debug_print("In getattr()\n");
	memset(stbuf, 0, sizeof(struct stat));
    if(strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
    }
    else if(strcmp(path, "/file") == 0) {
        stbuf->st_mode = S_IFREG | 0777;
        stbuf->st_nlink = 1;
        stbuf->st_size = 0; // FIXME: Implement
    }
    else
        return -ENOENT;

	return -E_SUCCESS;
}

// _open()_        returns //-EACCES//, but not really. returns _path_exists()_
static int fuse_service_open(const char *path, struct fuse_file_info *fi)
{
	debug_print("In open()\n");
	try {
		SFSFSSFSF_File *f = new SFSFSSFSF_File(superblock_file, NULL);
		fi->fh = (uint64_t)f;
		return -E_SUCCESS;
	}
	catch (char *e) { return print_err(e); }
  
}

// _read()_        (following maintains argument order) from rfile at //path//, fill //buf//, with //size// bytes, starting from //offset// to //offset//+size. Read directly from dirty cache if available.
static int fuse_service_read(const char *path, char *buf, size_t size,
                             off_t offset, struct fuse_file_info *fi)
{
	debug_print("In read()\n");
	SFSFSSFSF_File *f = (SFSFSSFSF_File *)(fi->fh);
	debug_print("1\n");
	try {
		f->read(offset, size, (uint8_t *)buf);
		debug_print("2\n");
	}
	catch (char *e) {
		debug_print("3\n");
		return print_err(e); 
	}
	
 
	return 0;
}

static int fuse_service_write(const char *path, const char *buf, size_t size,
                              off_t offset, struct fuse_file_info *fi)
{
	debug_print("write()\n");
	SFSFSSFSF_File *f = (SFSFSSFSF_File *)(fi->fh);

	try { f->write(offset, size, (uint8_t *)buf); }
	catch (char *e) { return print_err(e); }

	return 0;
}

//// queue an _fsync_ if the time is right (what this means is up for determination).
// _fsync()_       write dirty cache. AKA batch job to encryption threads.
static int fuse_service_fsync(const char *path, int sync_metadata, struct fuse_file_info *fi)
{
	debug_print("In fsync()\n");
	SFSFSSFSF_File *f = (SFSFSSFSF_File *)(fi->fh);
	try { f->fsync(); }
	catch (char *e) {
		print_err(e);
		return -1;
	}

	return 0;
}

// TODO: implement for real
static int fuse_service_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                             off_t offset, struct fuse_file_info *fi)
{
	debug_print("In readdir()\n");
	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);
	filler(buf, "file", NULL, 0);

	return 0;
}

void fuse_service_ops(struct fuse_operations *ops)
{
	debug_print("In ops()\n");
	ops->init = fuse_service_init;
	ops->getattr = fuse_service_getattr;
	ops->readdir = fuse_service_readdir;
	ops->open = fuse_service_open;
	ops->read = fuse_service_read;
	ops->write = fuse_service_write;
	ops->fsync = fuse_service_fsync;
	ops->access = fuse_service_access;
	ops->create = fuse_service_create;
}