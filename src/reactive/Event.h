#ifndef REACTIVE_EVENT_H
#define REACTIVE_EVENT_H

#include "details/Event.h"

namespace reactive{

    template<typename ...Args>
    using Event = details::Event<Args...>;

}

#endif //REACTIVE_EVENT_H
