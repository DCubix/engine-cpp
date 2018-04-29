#include "filesys.h"

NS_BEGIN

void VFS::init(const String& archiveFile) {
	if (!PHYSFS_isInit()) {
		PHYSFS_init(NULL);
	}

	if (!archiveFile.empty()) {
		VFS::addArchive(archiveFile);
	}
}

void VFS::deInit() {
	if (PHYSFS_isInit()) {
		PHYSFS_deinit();
	}
}

void VFS::addArchive(const String& fileName) {
	PHYSFS_mount(fileName.c_str(), NULL, 1);
}

void VFS::removeArchive(const String& fileName) {
	PHYSFS_unmount(fileName.c_str());
}

bool VFS::exists(const String& fileName) {
	return PHYSFS_exists(fileName.c_str()) != 0;
}

uptr<VirtualFile> VFS::open(const String& fileName) {
	PHYSFS_file* file = PHYSFS_openRead(fileName.c_str());
	
	if (file == NULL) {
		return {};
	}

	VirtualFile* f = new VirtualFile;
	f->size = fsize(PHYSFS_fileLength(file));
	f->fh = file;
	f->fileName = fileName;

	return uptr<VirtualFile>(f);
}

u8* VirtualFile::readAll() {
	u8* data = new u8[size];
	PHYSFS_readBytes(fh, data, size);
	return data;
}

void VirtualFile::close() {
	PHYSFS_close(fh);
}

uptr<RealFile> FIO::openFile(const String& fileName, FileMode mode, bool binary) {
	String m = "";
	
	bool getSize = false;
	switch (mode) {
		case FileModeRead: m = "r"; getSize = true; break;
		case FileModeWrite: m = "w"; break;
		case FileModeAppend: m = "a"; break;
	}
	if (binary) {
		m += "b";
	}

	FILE *fs = fopen(fileName.c_str(), m.c_str());
	if (!fs) {
		return {};
	}

	size_t flsize = 0;
	if (getSize) {
		size_t beg = ftell(fs);
		fseek(fs, 0, SEEK_END);
		size_t end = ftell(fs);
		flsize = end - beg;
		rewind(fs);
	}

	RealFile* rf = new RealFile(fs);
	rf->size = flsize;
	rf->fileName = fileName;

	return uptr<RealFile>(rf);
}

u8* RealFile::readAll() {
	u8* data = new u8[size];
	memset(data, 0, sizeof(u8) * size);
	fread(data, sizeof(u8), size, fh);
	return data;
}

opt<String> RealFile::getLine() {
	if (feof(fh)) return {};
	char ln[1024];
	fgets(ln, 1024, fh);
	return String(ln);
}

void RealFile::write(u8* data, size_t count) {
	fwrite(data, 1, count, fh);
}

void RealFile::write(const String& data) {
	fwrite(data.c_str(), sizeof(char), data.size(), fh);
}

void RealFile::close() {
	fclose(fh);
}

NS_END
