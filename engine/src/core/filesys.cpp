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
	PHYSFS_addToSearchPath(fileName.c_str(), 1);
}

void VFS::removeArchive(const String& fileName) {
	PHYSFS_removeFromSearchPath(fileName.c_str());
}

bool VFS::exists(const String& fileName) {
	return PHYSFS_exists(fileName.c_str()) != 0;
}

opt<VirtualFile> VFS::open(const String& fileName) {
	PHYSFS_file* file = PHYSFS_openRead(fileName.c_str());
	
	if (file == NULL) {
		return {};
	}

	VirtualFile f;
	f.size = fsize(PHYSFS_fileLength(file));
	f.fh = file;

	return f;
}

u8* VirtualFile::readAll() {
	u8* data = new u8[size];
	PHYSFS_readBytes(fh, data, size);
	return data;
}

void VirtualFile::close() {
	PHYSFS_close(fh);
}

opt<RealFile> FIO::openFile(const String& fileName, FileMode mode, bool binary) {
	int m = 0;
	if (binary) {
		m = std::ios::binary;
	}

	bool getSize = false;
	switch (mode) {
		case FileModeRead: m |= std::ios::in; getSize = true; break;
		case FileModeWrite: m |= std::ios::out; break;
		case FileModeAppend: m |= std::ios::app; break;
	}

	std::fstream *fs = new std::fstream(fileName, m);
	
	if (fs->bad()) {
		return {};
	}

	size_t flsize = 0;
	if (getSize) {
		size_t beg = fs->tellg();
		fs->seekg(0, std::ios::end);
		size_t end = fs->tellg();
		flsize = end - beg;
	}

	RealFile rf(fs);
	rf.size = flsize;

	return rf;
}

u8* RealFile::readAll() {
	u8* data = new u8[size];
	fh->read((char*)data, size);
	return data;
}

opt<String> RealFile::getLine() {
	if (fh->eof()) return {};
	char ln[1024];
	fh->getline(ln, 1024);
	return String(ln);
}

void RealFile::write(u8* data, size_t count) {
	fh->write(reinterpret_cast<char*>(data), count);
}

void RealFile::write(const String& data) {
	(*fh) << data;
}

void RealFile::close() {
	fh->close();
}

NS_END
