#include <fstream>
#include <iostream>

#include <poll.h>
#include <unistd.h>

#include "CloudFactory.h"
#include "Utility/Utility.h"

using namespace cloudstorage;

using cloudstorage::util::log;

class EventLoop : public CloudEventLoop {
 public:
  void onEventAdded() override {
    std::unique_lock<std::mutex> lock(mutex_);
    empty_condition_.notify_one();
    events_ready_ = true;
  }

  int exec() {
    struct pollfd pollfd = {STDIN_FILENO, POLLIN | POLLPRI, 0};

    while (true) {
      if (poll(&pollfd, 1, 100)) {
        char string[10];
        scanf("%9s", string);
        if (string == std::string("q")) {
          break;
        }
      }
      {
        std::unique_lock<std::mutex> lock(mutex_);
        empty_condition_.wait_for(lock, std::chrono::milliseconds(100),
                                  [=] { return events_ready_; });
        events_ready_ = false;
      }
      processEvents();
    }
    return 0;
  }

 private:
  std::mutex mutex_;
  std::condition_variable empty_condition_;
  bool events_ready_ = false;
};

class Factory : public CloudFactory {
 public:
  using CloudFactory::CloudFactory;

  void onCloudCreated(std::shared_ptr<CloudAccess> d) {
    if (d->name() == "google") {
      d->generalData().then([](GeneralData d) {
        log("got general data here", d.username_, d.space_used_,
            d.space_total_);
      });
    } else if (d->name() == "animezone") {
      const auto path =
          "/Anime/D/Death Note/1: Odrodzenie./Death Note 1 [PL] "
          "[Openload.co].mp4";
      d->getItem(path)
          .then([d](IItem::Pointer item) { return d->getDaemonUrl(item); })
          .then([](std::string url) { log(url); });
    } else if (d->name() == "mega") {
      std::stringstream input;
      input << "dupa";
      d->uploadFile(d->root(), "test",
                    streamUploader(
                        std::make_unique<std::stringstream>(std::move(input))))
          .then([=](IItem::Pointer file) {
            return d->downloadFile(
                file, FullRange,
                streamDownloader(std::make_unique<std::stringstream>()));
          })
          .then([] { log("downloaded"); });
    }
  }

  void onCloudRemoved(std::shared_ptr<CloudAccess>) {}
};

int main() {
  EventLoop loop;
  CloudFactory::InitData init_data;
  init_data.base_url_ = "http://localhost:12345";
  init_data.http_ = IHttp::create();
  init_data.http_server_factory_ = IHttpServerFactory::create();
  init_data.crypto_ = ICrypto::create();
  init_data.thread_pool_ = IThreadPool::create(1);
  Factory factory(&loop, std::move(init_data));
  {
    std::ifstream config("config.json");
    factory.load(config);
  }

  for (const auto& d : factory.availableProviders()) {
    log(factory.authorizationUrl(d));
  }

  int exec = loop.exec();

  {
    std::ofstream config("config.json");
    factory.dump(config);
  }

  return exec;
}
