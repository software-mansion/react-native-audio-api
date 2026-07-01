#include <audioapi/android/system/NativeFileInfo.hpp>

#include <atomic>
#include <string>

namespace audioapi {

namespace {

std::string cachedFilesDir;
std::string cachedCacheDir;
std::atomic<bool> cacheReady{false};

std::string fetchFilesDirFromJava() {
  static const auto method =
      NativeFileInfo::javaClassStatic()->getStaticMethod<jni::JString()>("getFilesDir");
  return method(NativeFileInfo::javaClassStatic())->toStdString();
}

std::string fetchCacheDirFromJava() {
  static const auto method =
      NativeFileInfo::javaClassStatic()->getStaticMethod<jni::JString()>("getCacheDir");
  return method(NativeFileInfo::javaClassStatic())->toStdString();
}

} // namespace

void NativeFileInfo::warmCache() {
  cachedFilesDir = fetchFilesDirFromJava();
  cachedCacheDir = fetchCacheDirFromJava();
  cacheReady.store(true, std::memory_order_release);
}

std::string NativeFileInfo::getFilesDir() {
  if (cacheReady.load(std::memory_order_acquire)) {
    return cachedFilesDir;
  }
  return fetchFilesDirFromJava();
}

std::string NativeFileInfo::getCacheDir() {
  if (cacheReady.load(std::memory_order_acquire)) {
    return cachedCacheDir;
  }
  return fetchCacheDirFromJava();
}

} // namespace audioapi
