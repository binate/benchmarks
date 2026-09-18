# richards — see ../c/richards.c for the algorithm.
import sys

I_IDLE, I_WORK, I_HANDLERA, I_HANDLERB, I_DEVA, I_DEVB, NUM_IDS = 1, 2, 3, 4, 5, 6, 6
K_DEV, K_WORK = 1000, 1001
STATE_RUNNING, STATE_RUNNABLE, STATE_SUSPENDED, STATE_HELD, STATE_SUSPENDED_RUNNABLE = 0, 1, 2, 4, 3
DATA_SIZE, COUNT = 4, 1000


class Packet:
    __slots__ = ("link", "id", "kind", "a1", "a2")

    def __init__(self, link, id, kind):
        self.link, self.id, self.kind, self.a1 = link, id, kind, 0
        self.a2 = [0] * DATA_SIZE

    def add_to(self, lst):
        self.link = None
        if lst is None:
            return self
        p = lst
        while p.link is not None:
            p = p.link
        p.link = self
        return lst


class TCB:
    __slots__ = ("link", "id", "priority", "input", "state", "task")

    def __init__(self, link, id, priority, queue, task):
        self.link, self.id, self.priority, self.input, self.task = link, id, priority, queue, task
        self.state = STATE_SUSPENDED if queue is None else STATE_SUSPENDED_RUNNABLE

    def held_or_suspended(self):
        return (self.state & STATE_HELD) != 0 or self.state == STATE_SUSPENDED


class Scheduler:
    def __init__(self):
        self.queue_count = self.hold_count = self.current_id = 0
        self.blocks = [None] * (NUM_IDS + 1)
        self.list = self.current = None

    def hold(self):
        self.hold_count += 1
        self.current.state |= STATE_HELD
        return self.current.link

    def suspend(self):
        self.current.state |= STATE_SUSPENDED
        return self.current

    def release(self, id):
        t = self.blocks[id]
        if t is None:
            return None
        t.state &= ~STATE_HELD
        return t if t.priority > self.current.priority else self.current

    def check_priority_add(self, t, p):
        if t.input is None:
            t.input = p
            t.state |= STATE_RUNNABLE
            if t.priority > self.current.priority:
                return t
        else:
            t.input = p.add_to(t.input)
        return self.current

    def queue(self, p):
        t = self.blocks[p.id]
        if t is None:
            return None
        self.queue_count += 1
        p.link = None
        p.id = self.current_id
        return self.check_priority_add(t, p)

    def add_task(self, id, priority, q, task):
        t = TCB(self.list, id, priority, q, task)
        self.list = self.current = t
        self.blocks[id] = t

    def schedule(self):
        self.current = self.list
        while self.current is not None:
            if self.current.held_or_suspended():
                self.current = self.current.link
                continue
            self.current_id = self.current.id
            p = None
            if self.current.state == STATE_SUSPENDED_RUNNABLE:
                p = self.current.input
                self.current.input = p.link
                self.current.state = STATE_RUNNING if self.current.input is None else STATE_RUNNABLE
            cur = self.current
            self.current = cur.task.run(self, p)


class IdleTask:
    __slots__ = ("control", "count")

    def __init__(self):
        self.control, self.count = 1, COUNT

    def run(self, s, p):
        self.count -= 1
        if self.count == 0:
            return s.hold()
        if self.control & 1 == 0:
            self.control >>= 1
            return s.release(I_DEVA)
        self.control = (self.control >> 1) ^ 53256
        return s.release(I_DEVB)


class DeviceTask:
    __slots__ = ("pending",)

    def __init__(self):
        self.pending = None

    def run(self, s, p):
        if p is None:
            if self.pending is None:
                return s.suspend()
            q = self.pending
            self.pending = None
            return s.queue(q)
        self.pending = p
        return s.hold()


class WorkerTask:
    __slots__ = ("dest", "counter")

    def __init__(self):
        self.dest, self.counter = I_HANDLERA, 0

    def run(self, s, p):
        if p is None:
            return s.suspend()
        self.dest = I_HANDLERB if self.dest == I_HANDLERA else I_HANDLERA
        p.id = self.dest
        p.a1 = 0
        for i in range(DATA_SIZE):
            self.counter += 1
            if self.counter > 26:
                self.counter = 1
            p.a2[i] = 65 + self.counter - 1
        return s.queue(p)


class HandlerTask:
    __slots__ = ("work", "device")

    def __init__(self):
        self.work = self.device = None

    def run(self, s, p):
        if p is not None:
            if p.kind == K_WORK:
                self.work = p.add_to(self.work)
            else:
                self.device = p.add_to(self.device)
        if self.work is not None:
            c = self.work.a1
            if c < DATA_SIZE:
                if self.device is not None:
                    v = self.device
                    self.device = self.device.link
                    v.a1 = self.work.a2[c]
                    self.work.a1 = c + 1
                    return s.queue(v)
            else:
                v = self.work
                self.work = self.work.link
                return s.queue(v)
        return s.suspend()


def run_once():
    s = Scheduler()
    s.add_task(I_IDLE, 0, None, IdleTask())
    s.blocks[I_IDLE].state = STATE_RUNNING
    wq = Packet(None, I_WORK, K_WORK)
    wq = Packet(wq, I_WORK, K_WORK)
    s.add_task(I_WORK, 1000, wq, WorkerTask())
    qa = Packet(None, I_DEVA, K_DEV)
    qa = Packet(qa, I_DEVA, K_DEV)
    qa = Packet(qa, I_DEVA, K_DEV)
    s.add_task(I_HANDLERA, 2000, qa, HandlerTask())
    qb = Packet(None, I_DEVB, K_DEV)
    qb = Packet(qb, I_DEVB, K_DEV)
    qb = Packet(qb, I_DEVB, K_DEV)
    s.add_task(I_HANDLERB, 3000, qb, HandlerTask())
    s.add_task(I_DEVA, 4000, None, DeviceTask())
    s.add_task(I_DEVB, 5000, None, DeviceTask())
    s.schedule()
    return s.queue_count, s.hold_count


def main():
    iters = int(sys.argv[1]) if len(sys.argv) > 1 else 100
    q = h = 0
    for _ in range(iters):
        q, h = run_once()
    print("queue %d hold %d" % (q, h))


main()
