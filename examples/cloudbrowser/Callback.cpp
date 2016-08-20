/*****************************************************************************
 * Callback.cpp : Callback implementation
 *
 *****************************************************************************
 * Copyright (C) 2016 VideoLAN
 *
 * Authors: Paweł Wegner <pawel.wegner95@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#include "Callback.h"

#include <QDir>
#include <QSettings>
#include <iostream>

#include "Utility/Utility.h"
#include "Window.h"

using cloudstorage::make_unique;

DownloadFileCallback::DownloadFileCallback(Window* window, std::string filename)
    : window_(window), file_(filename.c_str()), filename_(filename) {
  file_.open(QFile::WriteOnly);
}

void DownloadFileCallback::receivedData(const char* data, uint32_t length) {
  file_.write(data, length);
}

void DownloadFileCallback::done() {
  {
    std::unique_lock<std::mutex> lock(window_->stream_mutex());
    std::cerr << "[OK] Finished download.\n";
  }
  file_.close();
  emit window_->downloadProgressChanged(0, 0);
}

void DownloadFileCallback::error(const std::string& desc) {
  std::unique_lock<std::mutex> lock(window_->stream_mutex());
  std::cerr << "[FAIL] Download: " << desc << "\n";
  emit window_->downloadProgressChanged(0, 0);
}

void DownloadFileCallback::progress(uint32_t total, uint32_t now) {
  emit window_->downloadProgressChanged(total, now);
}

UploadFileCallback::UploadFileCallback(Window* window, QUrl url)
    : window_(window), file_(url.toLocalFile()) {
  file_.open(QFile::ReadOnly);
}

void UploadFileCallback::reset() {
  {
    std::unique_lock<std::mutex> lock(window_->stream_mutex());
    std::cerr << "[DIAG] Starting transmission\n";
  }
  file_.reset();
}

uint32_t UploadFileCallback::putData(char* data, uint32_t maxlength) {
  return file_.read(data, maxlength);
}

uint64_t UploadFileCallback::size() { return file_.size(); }

void UploadFileCallback::done() {
  std::unique_lock<std::mutex> lock(window_->stream_mutex());
  std::cerr << "[OK] Successfuly uploaded\n";
  emit window_->uploadProgressChanged(0, 0);
}

void UploadFileCallback::error(const std::string& description) {
  std::unique_lock<std::mutex> lock(window_->stream_mutex());
  std::cerr << "[FAIL] Upload: " << description << "\n";
  emit window_->uploadProgressChanged(0, 0);
}

void UploadFileCallback::progress(uint32_t total, uint32_t now) {
  emit window_->uploadProgressChanged(total, now);
}

CloudProviderCallback::CloudProviderCallback(Window* w) : window_(w) {}

ICloudProvider::ICallback::Status CloudProviderCallback::userConsentRequired(
    const ICloudProvider& p) {
  std::unique_lock<std::mutex> lock(window_->stream_mutex());
  std::cerr << "[DIAG] User consent required: " << p.authorizeLibraryUrl()
            << "\n";
  emit window_->consentRequired(p.name().c_str());
  emit window_->openBrowser(p.authorizeLibraryUrl().c_str());
  return Status::WaitForAuthorizationCode;
}

void CloudProviderCallback::accepted(const ICloudProvider& drive) {
  QSettings settings;
  settings.setValue(drive.name().c_str(), drive.token().c_str());
  emit window_->closeBrowser();
  emit window_->successfullyAuthorized(drive.name().c_str());
}

void CloudProviderCallback::declined(const ICloudProvider&) {
  emit window_->closeBrowser();
}

void CloudProviderCallback::error(const ICloudProvider&,
                                  const std::string& desc) {
  std::unique_lock<std::mutex> lock(window_->stream_mutex());
  std::cerr << "[FAIL] Authorize " << desc.c_str() << "\n";
  emit window_->closeBrowser();
}

ListDirectoryCallback::ListDirectoryCallback(Window* w) : window_(w) {}

void ListDirectoryCallback::receivedItem(IItem::Pointer item) {
  emit window_->addedItem(item);
}

void ListDirectoryCallback::done(const std::vector<IItem::Pointer>&) {}

void ListDirectoryCallback::error(const std::string& str) {
  std::unique_lock<std::mutex> lock(window_->stream_mutex());
  std::cerr << "[FAIL] ListDirectory: " << str << "\n";
  emit window_->closeBrowser();
}

DownloadThumbnailCallback::DownloadThumbnailCallback(ItemModel* i) : item_(i) {}

void DownloadThumbnailCallback::receivedData(const char* data,
                                             uint32_t length) {
  data_ += std::string(data, data + length);
}

void DownloadThumbnailCallback::done() {
  {
    QFile file(QDir::tempPath() + "/" +
               Window::escapeFileName(item_->item()->filename()).c_str() +
               ".thumbnail");
    file.open(QFile::WriteOnly);
    file.write(data_.data(), data_.length());
  }
  emit item_->receivedImage();
}

void DownloadThumbnailCallback::error(const std::string& error) {
  std::unique_lock<std::mutex> lock(item_->window_->stream_mutex());
  std::cerr << "[FAIL] Thumbnail: " << error << "\n";
}

void DownloadThumbnailCallback::progress(uint32_t, uint32_t) {}
