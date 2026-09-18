// richards — see ../c/richards.c for the algorithm. Task is an abstract base
// with a virtual run(); each iteration's objects are owned by unique_ptr arenas
// (freed at scope exit), and the graph links are non-owning raw pointers.
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <vector>

enum { I_IDLE = 1, I_WORK = 2, I_HANDLERA = 3, I_HANDLERB = 4, I_DEVA = 5, I_DEVB = 6, NUM_IDS = 6 };
enum { K_DEV = 1000, K_WORK = 1001 };
enum { STATE_RUNNING = 0, STATE_RUNNABLE = 1, STATE_SUSPENDED = 2, STATE_HELD = 4, STATE_SUSPENDED_RUNNABLE = 3 };
enum { DATA_SIZE = 4, COUNT = 1000 };

struct Packet {
    Packet *link = nullptr;
    int id, kind, a1 = 0;
    int a2[DATA_SIZE] = {0, 0, 0, 0};
    Packet(Packet *l, int i, int k) : link(l), id(i), kind(k) {}
    Packet *addTo(Packet *list) {
        link = nullptr;
        if (!list) return this;
        Packet *p = list;
        while (p->link) p = p->link;
        p->link = this;
        return list;
    }
};

struct Scheduler;
struct TCB;
struct Task {
    virtual TCB *run(Scheduler &s, Packet *p) = 0;
    virtual ~Task() {}
};

struct TCB {
    TCB *link;
    int id, priority;
    Packet *input;
    int state;
    Task *task;
    TCB(TCB *l, int i, int pr, Packet *q, Task *t)
        : link(l), id(i), priority(pr), input(q), task(t) {
        state = q ? STATE_SUSPENDED_RUNNABLE : STATE_SUSPENDED;
    }
    bool heldOrSuspended() const { return (state & STATE_HELD) != 0 || state == STATE_SUSPENDED; }
};

struct Scheduler {
    long queueCount = 0, holdCount = 0;
    TCB *blocks[NUM_IDS + 1] = {nullptr};
    TCB *list = nullptr, *current = nullptr;
    int currentId = 0;
    std::vector<std::unique_ptr<Packet>> pkts;
    std::vector<std::unique_ptr<TCB>> tcbs;
    std::vector<std::unique_ptr<Task>> tasks;

    Packet *newPacket(Packet *l, int i, int k) {
        pkts.push_back(std::make_unique<Packet>(l, i, k));
        return pkts.back().get();
    }
    template <class T> T *newTask() {
        tasks.push_back(std::make_unique<T>());
        return static_cast<T *>(tasks.back().get());
    }
    TCB *hold() { holdCount++; current->state |= STATE_HELD; return current->link; }
    TCB *suspend() { current->state |= STATE_SUSPENDED; return current; }
    TCB *release(int id) {
        TCB *t = blocks[id];
        if (!t) return nullptr;
        t->state &= ~STATE_HELD;
        return t->priority > current->priority ? t : current;
    }
    TCB *checkPriorityAdd(TCB *t, Packet *p) {
        if (!t->input) {
            t->input = p;
            t->state |= STATE_RUNNABLE;
            if (t->priority > current->priority) return t;
        } else {
            t->input = p->addTo(t->input);
        }
        return current;
    }
    TCB *queue(Packet *p) {
        TCB *t = blocks[p->id];
        if (!t) return nullptr;
        queueCount++;
        p->link = nullptr;
        p->id = currentId;
        return checkPriorityAdd(t, p);
    }
    void addTask(int id, int priority, Packet *q, Task *task) {
        tcbs.push_back(std::make_unique<TCB>(list, id, priority, q, task));
        list = current = tcbs.back().get();
        blocks[id] = list;
    }
    void schedule() {
        current = list;
        while (current) {
            if (current->heldOrSuspended()) { current = current->link; continue; }
            currentId = current->id;
            Packet *p = nullptr;
            if (current->state == STATE_SUSPENDED_RUNNABLE) {
                p = current->input;
                current->input = p->link;
                current->state = current->input ? STATE_RUNNABLE : STATE_RUNNING;
            }
            TCB *cur = current;
            current = cur->task->run(*this, p);
        }
    }
};

struct IdleTask : Task {
    int control = 1, count = COUNT;
    TCB *run(Scheduler &s, Packet *) override {
        count--;
        if (count == 0) return s.hold();
        if ((control & 1) == 0) { control >>= 1; return s.release(I_DEVA); }
        control = (control >> 1) ^ 53256;
        return s.release(I_DEVB);
    }
};
struct DeviceTask : Task {
    Packet *pending = nullptr;
    TCB *run(Scheduler &s, Packet *p) override {
        if (!p) {
            if (!pending) return s.suspend();
            Packet *q = pending; pending = nullptr; return s.queue(q);
        }
        pending = p;
        return s.hold();
    }
};
struct WorkerTask : Task {
    int dest = I_HANDLERA, counter = 0;
    TCB *run(Scheduler &s, Packet *p) override {
        if (!p) return s.suspend();
        dest = (dest == I_HANDLERA) ? I_HANDLERB : I_HANDLERA;
        p->id = dest;
        p->a1 = 0;
        for (int i = 0; i < DATA_SIZE; i++) {
            counter++;
            if (counter > 26) counter = 1;
            p->a2[i] = 65 + counter - 1;
        }
        return s.queue(p);
    }
};
struct HandlerTask : Task {
    Packet *work = nullptr, *device = nullptr;
    TCB *run(Scheduler &s, Packet *p) override {
        if (p) {
            if (p->kind == K_WORK) work = p->addTo(work);
            else device = p->addTo(device);
        }
        if (work) {
            int c = work->a1;
            if (c < DATA_SIZE) {
                if (device) {
                    Packet *v = device;
                    device = device->link;
                    v->a1 = work->a2[c];
                    work->a1 = c + 1;
                    return s.queue(v);
                }
            } else {
                Packet *v = work;
                work = work->link;
                return s.queue(v);
            }
        }
        return s.suspend();
    }
};

static void runOnce(long &q, long &h) {
    Scheduler s;
    s.addTask(I_IDLE, 0, nullptr, s.newTask<IdleTask>());
    s.blocks[I_IDLE]->state = STATE_RUNNING;

    Packet *wq = s.newPacket(nullptr, I_WORK, K_WORK);
    wq = s.newPacket(wq, I_WORK, K_WORK);
    s.addTask(I_WORK, 1000, wq, s.newTask<WorkerTask>());

    Packet *qa = s.newPacket(nullptr, I_DEVA, K_DEV);
    qa = s.newPacket(qa, I_DEVA, K_DEV);
    qa = s.newPacket(qa, I_DEVA, K_DEV);
    s.addTask(I_HANDLERA, 2000, qa, s.newTask<HandlerTask>());

    Packet *qb = s.newPacket(nullptr, I_DEVB, K_DEV);
    qb = s.newPacket(qb, I_DEVB, K_DEV);
    qb = s.newPacket(qb, I_DEVB, K_DEV);
    s.addTask(I_HANDLERB, 3000, qb, s.newTask<HandlerTask>());

    s.addTask(I_DEVA, 4000, nullptr, s.newTask<DeviceTask>());
    s.addTask(I_DEVB, 5000, nullptr, s.newTask<DeviceTask>());

    s.schedule();
    q = s.queueCount;
    h = s.holdCount;
}

int main(int argc, char **argv) {
    int iters = argc > 1 ? std::atoi(argv[1]) : 100;
    long q = 0, h = 0;
    for (int i = 0; i < iters; i++) runOnce(q, h);
    std::printf("queue %ld hold %ld\n", q, h);
    return 0;
}
