#pragma once

#include <audioapi/utils/Macros.h>

namespace audioapi {

class CommonPlayer {
 public:
  CommonPlayer() = default;
  DELETE_COPY_AND_MOVE(CommonPlayer);
  virtual ~CommonPlayer() = default;

  virtual bool start() = 0;
  virtual void stop() = 0;
  virtual bool resume() = 0;
  virtual void suspend() = 0;
  virtual void cleanup() = 0;

  [[nodiscard]] virtual bool isRunning() const = 0;
};

} // namespace audioapi
