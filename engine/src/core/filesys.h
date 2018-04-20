#ifndef FILESYS_H
#define FILESYS_H

#include "types.h"

#include <cstdio>

extern "C" {
	#include <physfs.h>
}

NS_BEGIN

using fsize = PHYSFS_uint64;

struct VirtualFile {
	VirtualFile() {}

	String fileName;
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

	static uptr<VirtualFile> open(const String& fileName);
};

enum FileMode {
	FileModeRead = 0,
	FileModeWrite,
	FileModeAppend
};

struct RealFile {
	RealFile(FILE *fh) : fh(fh) {}

	String fileName;
	size_t size;
	FILE *fh;

	u8* readAll();
	opt<String> getLine();

	void write(u8* data, size_t count);
	void write(const String& data);

	template <typename T>
	void writeObject(T val) {
		char* buf = new char[sizeof(T)];
		memcpy(buf, &val, sizeof(T));
		fwrite(buf, sizeof(T), 1, fh);
	}

	template <typename T>
	void readObject(T* obj) {
		char* buf = new char[sizeof(T)];
		fread(buf, sizeof(T), 1, fh);
		memcpy(obj, buf, sizeof(T));
	}

	void close();
};

class FIO {
public:
	static uptr<RealFile> openFile(const String& fileName,
							FileMode mode = FileMode::FileModeRead,
							bool binary = false);
};

NS_END

#endif // FILESYS_H