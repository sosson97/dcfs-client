
#include <filesystem>

#include <cstring>
#include <cassert>
#include "backend.hpp"

namespace fs = std::filesystem;

void init_backend(StorageBackend *backend) {
	backend = new FileBackend();
	fs::create_directories(BACKEND_MNT_POINT);
}

