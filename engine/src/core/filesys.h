#ifndef FILESYS_H
#define FILESYS_H

#include "types.h"

#include <fstream>

extern "C" {
	#include <physfs.h>
}

NS_BEGIN

using fsize = PHYSFS_uint64;

struct VirtualFile {
	VirtualFile() {}

	fsize size;
	PHYSFS_file *fh;

	u8* readAll();
	void close();

	template <typename T>
	T readObject() {
		u8* data = new u8[sizeof(T)];
		PHYSFS_read(fh, data, sizeof(T), 1);
		T ret; memcpy(&ret, data, sizeof(T));
		return ret;
	}
};

class VFS {
public:
	static void init(const String& archiveFile = "");
	static void deInit();

	static void addArchive(const String& fileName);
	static void removeArchive(const String& fileName);

	static bool exists(const String& fileName);

	static opt<VirtualFile> open(const String& fileName);
};

enum FileMode {
	FileModeRead = 0,
	FileModeWrite,
	FileModeAppend
};

struct RealFile {
	RealFile(std::fstream *fh) : fh(fh) {}

	size_t size;
	std::fstream *fh;

	u8* readAll();
	opt<String> getLine();

	void write(u8* data, size_t count);
	void write(const String& data);

	template <typename T>
	void writeObject(T val) {
		char* buf = new char[sizeof(T)];
		memcpy(buf, &val, sizeof(T));
		fh->write(buf, sizeof(T));
	}

	template <typename T>
	void readObject(T* obj) {
		char* buf = new char[sizeof(T)];
		fh->read(buf, sizeof(T));
		memcpy(obj, buf, sizeof(T));
	}

	void close();
};

class FIO {
public:
	static opt<RealFile> openFile(const String& fileName,
								  FileMode mode = FileMode::FileModeRead,
								  bool binary = false);
};

NS_END

#endif // FILESYS_H