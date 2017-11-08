#include "MapBasedGlobalLockImpl.h"
#include <mutex>

namespace Afina {
namespace Backend {

// See MapBasedGlobalLockImpl.h
bool MapBasedGlobalLockImpl::Put(const std::string &key, const std::string &value) {
	if (_backend.find(key) == _backend.end()) return PutIfAbsent(key, value);//there is no such key
	Delete (key);
	return PutIfAbsent(key, value);
}

// See MapBasedGlobalLockImpl.h
bool MapBasedGlobalLockImpl::PutIfAbsent(const std::string &key, const std::string &value) {
    if (_backend.find(key) != _backend.end()) return false;//there is such key
	if (_backend.size() == _max_size) delete_old(); //check overflow
	auto key_value = std::pair<std::string, std::string>(key, value);
	_backend.insert(key_value);
	line.push_back(key);
	return true;
}

// See MapBasedGlobalLockImpl.h
bool MapBasedGlobalLockImpl::Set(const std::string &key, const std::string &value) {
    if (_backend.find(key) == _backend.end()) return false;
	_backend[key] = value;
    return true;
}

// See MapBasedGlobalLockImpl.h
bool MapBasedGlobalLockImpl::Delete(const std::string &key) {
    if (_backend.find(key) == _backend.end()) return false; //there is no such key
	_backend.erase(key);
	line.remove(key);
    return true;
}

// See MapBasedGlobalLockImpl.h
bool MapBasedGlobalLockImpl::Get(const std::string &key, std::string &value) const {
    if (_backend.find(key) == _backend.end()) return false; //there is no such key
    value = _backend.at(key); // put value in variable value
    return true;
}

void MapBasedGlobalLockImpl::delete_old(){
	std::string key = line.front();
	_backend.erase(key);
	line.pop_front();
}
} // namespace Backend
} // namespace Afina
