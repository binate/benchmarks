// richards — see ../c/richards.c for the algorithm. Task is abstract with a
// virtual run(); each task type overrides it.
public final class Richards {
    static final int I_IDLE = 1, I_WORK = 2, I_HANDLERA = 3, I_HANDLERB = 4, I_DEVA = 5, I_DEVB = 6, NUM_IDS = 6;
    static final int K_DEV = 1000, K_WORK = 1001;
    static final int STATE_RUNNING = 0, STATE_RUNNABLE = 1, STATE_SUSPENDED = 2, STATE_HELD = 4, STATE_SUSPENDED_RUNNABLE = 3;
    static final int DATA_SIZE = 4, COUNT = 1000;

    static final class Packet {
        Packet link;
        int id, kind, a1;
        int[] a2 = new int[DATA_SIZE];
        Packet(Packet link, int id, int kind) { this.link = link; this.id = id; this.kind = kind; }
        Packet addTo(Packet list) {
            link = null;
            if (list == null) return this;
            Packet p = list;
            while (p.link != null) p = p.link;
            p.link = this;
            return list;
        }
    }

    static abstract class Task { abstract TCB run(Scheduler s, Packet p); }

    static final class TCB {
        TCB link;
        int id, priority, state;
        Packet input;
        Task task;
        TCB(TCB link, int id, int priority, Packet queue, Task task) {
            this.link = link; this.id = id; this.priority = priority; this.input = queue; this.task = task;
            this.state = queue == null ? STATE_SUSPENDED : STATE_SUSPENDED_RUNNABLE;
        }
        boolean heldOrSuspended() { return (state & STATE_HELD) != 0 || state == STATE_SUSPENDED; }
    }

    static final class Scheduler {
        long queueCount, holdCount;
        TCB[] blocks = new TCB[NUM_IDS + 1];
        TCB list, current;
        int currentId;

        TCB hold() { holdCount++; current.state |= STATE_HELD; return current.link; }
        TCB suspend() { current.state |= STATE_SUSPENDED; return current; }
        TCB release(int id) {
            TCB t = blocks[id];
            if (t == null) return null;
            t.state &= ~STATE_HELD;
            return t.priority > current.priority ? t : current;
        }
        TCB checkPriorityAdd(TCB t, Packet p) {
            if (t.input == null) {
                t.input = p;
                t.state |= STATE_RUNNABLE;
                if (t.priority > current.priority) return t;
            } else {
                t.input = p.addTo(t.input);
            }
            return current;
        }
        TCB queue(Packet p) {
            TCB t = blocks[p.id];
            if (t == null) return null;
            queueCount++;
            p.link = null;
            p.id = currentId;
            return checkPriorityAdd(t, p);
        }
        void addTask(int id, int priority, Packet q, Task task) {
            TCB t = new TCB(list, id, priority, q, task);
            list = current = t;
            blocks[id] = t;
        }
        void schedule() {
            current = list;
            while (current != null) {
                if (current.heldOrSuspended()) { current = current.link; continue; }
                currentId = current.id;
                Packet p = null;
                if (current.state == STATE_SUSPENDED_RUNNABLE) {
                    p = current.input;
                    current.input = p.link;
                    current.state = current.input == null ? STATE_RUNNING : STATE_RUNNABLE;
                }
                TCB cur = current;
                current = cur.task.run(this, p);
            }
        }
    }

    static final class IdleTask extends Task {
        int control = 1, count = COUNT;
        TCB run(Scheduler s, Packet p) {
            count--;
            if (count == 0) return s.hold();
            if ((control & 1) == 0) { control >>= 1; return s.release(I_DEVA); }
            control = (control >> 1) ^ 53256;
            return s.release(I_DEVB);
        }
    }
    static final class DeviceTask extends Task {
        Packet pending;
        TCB run(Scheduler s, Packet p) {
            if (p == null) {
                if (pending == null) return s.suspend();
                Packet q = pending; pending = null; return s.queue(q);
            }
            pending = p;
            return s.hold();
        }
    }
    static final class WorkerTask extends Task {
        int dest = I_HANDLERA, counter = 0;
        TCB run(Scheduler s, Packet p) {
            if (p == null) return s.suspend();
            dest = (dest == I_HANDLERA) ? I_HANDLERB : I_HANDLERA;
            p.id = dest;
            p.a1 = 0;
            for (int i = 0; i < DATA_SIZE; i++) {
                counter++;
                if (counter > 26) counter = 1;
                p.a2[i] = 65 + counter - 1;
            }
            return s.queue(p);
        }
    }
    static final class HandlerTask extends Task {
        Packet work, device;
        TCB run(Scheduler s, Packet p) {
            if (p != null) {
                if (p.kind == K_WORK) work = p.addTo(work);
                else device = p.addTo(device);
            }
            if (work != null) {
                int c = work.a1;
                if (c < DATA_SIZE) {
                    if (device != null) {
                        Packet v = device;
                        device = device.link;
                        v.a1 = work.a2[c];
                        work.a1 = c + 1;
                        return s.queue(v);
                    }
                } else {
                    Packet v = work;
                    work = work.link;
                    return s.queue(v);
                }
            }
            return s.suspend();
        }
    }

    static long[] runOnce() {
        Scheduler s = new Scheduler();
        s.addTask(I_IDLE, 0, null, new IdleTask());
        s.blocks[I_IDLE].state = STATE_RUNNING;

        Packet wq = new Packet(null, I_WORK, K_WORK);
        wq = new Packet(wq, I_WORK, K_WORK);
        s.addTask(I_WORK, 1000, wq, new WorkerTask());

        Packet qa = new Packet(null, I_DEVA, K_DEV);
        qa = new Packet(qa, I_DEVA, K_DEV);
        qa = new Packet(qa, I_DEVA, K_DEV);
        s.addTask(I_HANDLERA, 2000, qa, new HandlerTask());

        Packet qb = new Packet(null, I_DEVB, K_DEV);
        qb = new Packet(qb, I_DEVB, K_DEV);
        qb = new Packet(qb, I_DEVB, K_DEV);
        s.addTask(I_HANDLERB, 3000, qb, new HandlerTask());

        s.addTask(I_DEVA, 4000, null, new DeviceTask());
        s.addTask(I_DEVB, 5000, null, new DeviceTask());

        s.schedule();
        return new long[] {s.queueCount, s.holdCount};
    }

    public static void main(String[] args) {
        int iters = args.length > 0 ? Integer.parseInt(args[0]) : 100;
        long[] r = {0, 0};
        for (int i = 0; i < iters; i++) r = runOnce();
        System.out.printf("queue %d hold %d%n", r[0], r[1]);
    }
}
