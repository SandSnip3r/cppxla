#ifndef PJRT_EVENT_HPP_
#define PJRT_EVENT_HPP_

struct PJRT_Event;

namespace pjrt {

class Context;
  
class Event {
public:
  Event(const Context &context, PJRT_Event *event);
  ~Event();
  void wait();
public:
// private:
  const Context &context_;
  PJRT_Event *event_{nullptr};

  void destroyEvent();
};
  
} // namespace pjrt

#endif // PJRT_EVENT_HPP_