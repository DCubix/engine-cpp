#ifndef FILESYS_H
#define FILESYS_H

#include "types.h"

#include <cstdio>
#include <cstdint>

#include <physfs.h>

NS_BEGIN

using fsize = uint64_t;

class VFS {
public:
	void mount(const String& virtualPath, const String& physPath);
	void mountDefault();
	void unmount(const String& virtualPath);
	
	bool openRead(const String& path);
	bool openWrite(const String& path);
	bool openAppend(const String& path);
	
	u8* read(fsize* size);
	String readText();

	void write(const u8* buffer, fsize size);
	void writeText(const String& text);
	
	bool close();
	
	void shutdown() { PHYSFS_deinit(); }
	
	static VFS& get() { return g_instance; }
	
private:
	VFS() { if (!PHYSFS_isInit()) PHYSFS_init(NULL); }
	
	static VFS g_instance;
	Map<String, Vector<String>> m_mountPoints;
	
	PHYSFS_File* m_file;
	
	bool checkFile();
};

NS_END

#endif // FILESYS_H