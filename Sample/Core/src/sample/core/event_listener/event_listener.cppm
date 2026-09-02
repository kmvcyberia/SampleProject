export module sample.core.event_listener;

export namespace sample {

class EventListener
{
public:
    virtual ~EventListener() = default;
    virtual void read() = 0;
    virtual void write() = 0;
};

} // namespace sample