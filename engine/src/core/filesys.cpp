#include "filesys.h"

#include "logging/log.h"

NS_BEGIN

VFS VFS::g_instance;

void VFS::mount(const String& virtualPath, const String& physPath) {
	if (PHYSFS_mount(physPath.c_str(), virtualPath.c_str(), 1) == 0) {
		LogError(PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
	}
}

void VFS::mountDefault() {
	if (PHYSFS_mount(PHYSFS_getBaseDir(), NULL, 1) == 0) {
		LogError(PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
	}
}

void VFS::unmount(const String& virtualPath) {
	if (PHYSFS_unmount(virtualPath.c_str()) == 0) {
		LogError(PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
	}
}

bool VFS::checkFile() {
	return m_file == NULL;
}

bool VFS::openRead(const String& path) {
	if (!checkFile()) { LogError("A file is already open!"); return false; }
	m_file = PHYSFS_openRead(path.c_str());
	if (!m_file) {
		LogError(path, ": ", PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
		return false;
	}
	return true;
}

bool VFS::openWrite(const String& path) {
	if (!checkFile()) { LogError("A file is already open!"); return false; }
	m_file = PHYSFS_openWrite(path.c_str());
	if (!m_file) {
		LogError(path, ": ", PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
		return false;
	}
	return true;
}

bool VFS::openAppend(const String& path) {
	if (!checkFile()) { LogError("A file is already open!"); return false; }
	m_file = PHYSFS_openAppend(path.c_str());
	if (!m_file) {
		LogError(path, ": ", PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
		return false;
	}
	return true;
}

Vector<VFSFileInfo> VFS::listFiles(const String& dir) {
	Vector<VFSFileInfo> ret;
	char** files = PHYSFS_enumerateFiles(dir.c_str());
	for (char **i = files; *i != NULL; i++) {
		VFSFileInfo fi;
		fi.fileName = *i;
		size_t lpt = fi.fileName.find_last_of(".");
		if (lpt != String::npos)
			fi.ext = fi.fileName.substr(lpt);
		fi.directory = PHYSFS_isDirectory(*i) == 0 ? false : true;
		fi.link = PHYSFS_isSymbolicLink(*i) == 0 ? false : true;
		ret.push_back(fi);
	}
	return ret;
}

bool VFS::exists(const String& dir) {
	return PHYSFS_exists(dir.c_str()) == 0 ? false : true;
}

u8* VFS::read(fsize* size) {
	if (checkFile()) {
		LogError("File was not open.");
		return nullptr;
	}

	fsize flen = PHYSFS_fileLength(m_file);

	u8* data = new u8[flen];
	if (PHYSFS_readBytes(m_file, data, flen) == -1) {
		LogError(PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
		delete[] data;
		data = nullptr;
	}

	if (size) *size = flen;
	return data;
}

String VFS::readText() {
	if (checkFile()) {
		LogError("File was not open.");
		return nullptr;
	}

	fsize flen = PHYSFS_fileLength(m_file);

	char* data = new char[flen / sizeof(char)];
	if (PHYSFS_readBytes(m_file, data, flen * sizeof(char)) == -1) {
		LogError(PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
		delete[] data;
		data = nullptr;
		return "";
	}
	data[flen] = 0;
	return String(data);
}

void VFS::write(const u8* buffer, fsize size) {
	if (checkFile()) {
		LogError("File was not open.");
		return;
	}

	if (PHYSFS_writeBytes(m_file, buffer, size) == -1) {
		LogError(PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
	}
}

void VFS::writeText(const String& text) {
	write((const u8*)text.c_str(), text.size() * sizeof(char));
}

bool VFS::close() {
	if (checkFile()) {
		LogError("File was not open.");
		return false;
	}
	PHYSFS_close(m_file);
	m_file = nullptr;
	return true;
}

NS_END
