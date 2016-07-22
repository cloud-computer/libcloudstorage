/*****************************************************************************
 * YandexDisk.h : YandexDisk headers
 *
 *****************************************************************************
 * Copyright (C) 2016-2016 VideoLAN
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

#ifndef YANDEXDISK_H
#define YANDEXDISK_H

#include <json/forwards.h>

#include "CloudProvider.h"

namespace cloudstorage {

class YandexDisk : public CloudProvider {
 public:
  YandexDisk();

  std::string name() const;
  IItem::Pointer rootDirectory() const;

  GetItemDataRequest::Pointer getItemDataAsync(const std::string& id,
                                               GetItemDataCallback callback);
  DownloadFileRequest::Pointer downloadFileAsync(
      IItem::Pointer, IDownloadFileCallback::Pointer);
  UploadFileRequest::Pointer uploadFileAsync(IItem::Pointer, const std::string&,
                                             IUploadFileCallback::Pointer);
  CreateDirectoryRequest::Pointer createDirectoryAsync(IItem::Pointer,
                                                       const std::string&,
                                                       CreateDirectoryCallback);

  HttpRequest::Pointer listDirectoryRequest(const IItem&,
                                            const std::string& page_token,
                                            std::ostream& input_stream) const;
  HttpRequest::Pointer deleteItemRequest(const IItem&,
                                         std::ostream& input_stream) const;
  HttpRequest::Pointer moveItemRequest(const IItem&,
                                       const IItem&,
                                       std::ostream&) const;

  std::vector<IItem::Pointer> listDirectoryResponse(
      std::istream&, std::string& next_page_token) const;

  IItem::Pointer toItem(const Json::Value&) const;
  void authorizeRequest(HttpRequest&) const;

  class Auth : public cloudstorage::Auth {
   public:
    Auth();

    std::string authorizeLibraryUrl() const;

    HttpRequest::Pointer exchangeAuthorizationCodeRequest(
        std::ostream& input_data) const;
    HttpRequest::Pointer refreshTokenRequest(std::ostream& input_data) const;

    Token::Pointer exchangeAuthorizationCodeResponse(std::istream&) const;
    Token::Pointer refreshTokenResponse(std::istream&) const;
  };
};

}  // namespace cloudstorage

#endif  // YANDEXDISK_H
