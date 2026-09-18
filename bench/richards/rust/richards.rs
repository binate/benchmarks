// richards — see ../c/richards.c for the algorithm. Dispatch is through
// `dyn Task` trait objects (Rust's vtable). Richards' object graph aliases nodes
// (blocks[] and the list both point at TCBs, packet links are shared and
// rewritten mid-run), which safe Box/&mut cannot express; the nodes are owned by
// Box arenas for their lifetime and the graph uses raw pointers, matching the C
// reference. The only unsafe is pointer dereference — no allocation tricks.
#![allow(dead_code)]

const I_IDLE: i32 = 1;
const I_WORK: i32 = 2;
const I_HANDLERA: i32 = 3;
const I_HANDLERB: i32 = 4;
const I_DEVA: i32 = 5;
const I_DEVB: i32 = 6;
const NUM_IDS: usize = 6;

const K_DEV: i32 = 1000;
const K_WORK: i32 = 1001;

const STATE_RUNNING: i32 = 0;
const STATE_RUNNABLE: i32 = 1;
const STATE_SUSPENDED: i32 = 2;
const STATE_HELD: i32 = 4;
const STATE_SUSPENDED_RUNNABLE: i32 = 3;

const DATA_SIZE: usize = 4;
const COUNT: i32 = 1000;

struct Packet {
    link: *mut Packet,
    id: i32,
    kind: i32,
    a1: i32,
    a2: [i32; DATA_SIZE],
}
unsafe fn add_to(p: *mut Packet, list: *mut Packet) -> *mut Packet {
    (*p).link = std::ptr::null_mut();
    if list.is_null() {
        return p;
    }
    let mut q = list;
    while !(*q).link.is_null() {
        q = (*q).link;
    }
    (*q).link = p;
    list
}

trait Task {
    fn run(&mut self, s: *mut Scheduler, p: *mut Packet) -> *mut TCB;
}

struct TCB {
    link: *mut TCB,
    id: i32,
    priority: i32,
    input: *mut Packet,
    state: i32,
    task: *mut dyn Task,
}
impl TCB {
    fn held_or_suspended(&self) -> bool {
        (self.state & STATE_HELD) != 0 || self.state == STATE_SUSPENDED
    }
}

struct Scheduler {
    queue_count: i64,
    hold_count: i64,
    blocks: [*mut TCB; NUM_IDS + 1],
    list: *mut TCB,
    current: *mut TCB,
    current_id: i32,
}
impl Scheduler {
    unsafe fn hold(&mut self) -> *mut TCB {
        self.hold_count += 1;
        (*self.current).state |= STATE_HELD;
        (*self.current).link
    }
    unsafe fn suspend(&mut self) -> *mut TCB {
        (*self.current).state |= STATE_SUSPENDED;
        self.current
    }
    unsafe fn release(&mut self, id: i32) -> *mut TCB {
        let t = self.blocks[id as usize];
        if t.is_null() {
            return std::ptr::null_mut();
        }
        (*t).state &= !STATE_HELD;
        if (*t).priority > (*self.current).priority {
            t
        } else {
            self.current
        }
    }
    unsafe fn check_priority_add(&mut self, t: *mut TCB, p: *mut Packet) -> *mut TCB {
        if (*t).input.is_null() {
            (*t).input = p;
            (*t).state |= STATE_RUNNABLE;
            if (*t).priority > (*self.current).priority {
                return t;
            }
        } else {
            (*t).input = add_to(p, (*t).input);
        }
        self.current
    }
    unsafe fn queue(&mut self, p: *mut Packet) -> *mut TCB {
        let t = self.blocks[(*p).id as usize];
        if t.is_null() {
            return std::ptr::null_mut();
        }
        self.queue_count += 1;
        (*p).link = std::ptr::null_mut();
        (*p).id = self.current_id;
        self.check_priority_add(t, p)
    }
    unsafe fn schedule(&mut self) {
        self.current = self.list;
        while !self.current.is_null() {
            if (*self.current).held_or_suspended() {
                self.current = (*self.current).link;
                continue;
            }
            self.current_id = (*self.current).id;
            let mut packet: *mut Packet = std::ptr::null_mut();
            if (*self.current).state == STATE_SUSPENDED_RUNNABLE {
                packet = (*self.current).input;
                (*self.current).input = (*packet).link;
                (*self.current).state = if (*self.current).input.is_null() {
                    STATE_RUNNING
                } else {
                    STATE_RUNNABLE
                };
            }
            let cur = self.current;
            let sp: *mut Scheduler = self;
            self.current = (*(*cur).task).run(sp, packet);
        }
    }
}

struct IdleTask {
    control: i32,
    count: i32,
}
impl Task for IdleTask {
    fn run(&mut self, s: *mut Scheduler, _p: *mut Packet) -> *mut TCB {
        unsafe {
            self.count -= 1;
            if self.count == 0 {
                return (*s).hold();
            }
            if self.control & 1 == 0 {
                self.control >>= 1;
                (*s).release(I_DEVA)
            } else {
                self.control = (self.control >> 1) ^ 53256;
                (*s).release(I_DEVB)
            }
        }
    }
}
struct DeviceTask {
    pending: *mut Packet,
}
impl Task for DeviceTask {
    fn run(&mut self, s: *mut Scheduler, p: *mut Packet) -> *mut TCB {
        unsafe {
            if p.is_null() {
                if self.pending.is_null() {
                    return (*s).suspend();
                }
                let q = self.pending;
                self.pending = std::ptr::null_mut();
                (*s).queue(q)
            } else {
                self.pending = p;
                (*s).hold()
            }
        }
    }
}
struct WorkerTask {
    dest: i32,
    counter: i32,
}
impl Task for WorkerTask {
    fn run(&mut self, s: *mut Scheduler, p: *mut Packet) -> *mut TCB {
        unsafe {
            if p.is_null() {
                return (*s).suspend();
            }
            self.dest = if self.dest == I_HANDLERA { I_HANDLERB } else { I_HANDLERA };
            (*p).id = self.dest;
            (*p).a1 = 0;
            for i in 0..DATA_SIZE {
                self.counter += 1;
                if self.counter > 26 {
                    self.counter = 1;
                }
                (*p).a2[i] = 65 + self.counter - 1;
            }
            (*s).queue(p)
        }
    }
}
struct HandlerTask {
    work: *mut Packet,
    device: *mut Packet,
}
impl Task for HandlerTask {
    fn run(&mut self, s: *mut Scheduler, p: *mut Packet) -> *mut TCB {
        unsafe {
            if !p.is_null() {
                if (*p).kind == K_WORK {
                    self.work = add_to(p, self.work);
                } else {
                    self.device = add_to(p, self.device);
                }
            }
            if !self.work.is_null() {
                let c = (*self.work).a1;
                if (c as usize) < DATA_SIZE {
                    if !self.device.is_null() {
                        let v = self.device;
                        self.device = (*self.device).link;
                        (*v).a1 = (*self.work).a2[c as usize];
                        (*self.work).a1 = c + 1;
                        return (*s).queue(v);
                    }
                } else {
                    let v = self.work;
                    self.work = (*self.work).link;
                    return (*s).queue(v);
                }
            }
            (*s).suspend()
        }
    }
}

struct Arena {
    packets: Vec<Box<Packet>>,
    tcbs: Vec<Box<TCB>>,
    tasks: Vec<Box<dyn Task>>,
}
impl Arena {
    fn new() -> Arena { Arena { packets: Vec::new(), tcbs: Vec::new(), tasks: Vec::new() } }
    fn packet(&mut self, link: *mut Packet, id: i32, kind: i32) -> *mut Packet {
        self.packets.push(Box::new(Packet { link, id, kind, a1: 0, a2: [0; DATA_SIZE] }));
        &mut **self.packets.last_mut().unwrap()
    }
    fn task(&mut self, t: Box<dyn Task>) -> *mut dyn Task {
        self.tasks.push(t);
        &mut **self.tasks.last_mut().unwrap()
    }
    fn tcb(&mut self, link: *mut TCB, id: i32, priority: i32, input: *mut Packet, task: *mut dyn Task) -> *mut TCB {
        let state = if input.is_null() { STATE_SUSPENDED } else { STATE_SUSPENDED_RUNNABLE };
        self.tcbs.push(Box::new(TCB { link, id, priority, input, state, task }));
        &mut **self.tcbs.last_mut().unwrap()
    }
}

fn run_once() -> (i64, i64) {
    let mut arena = Arena::new();
    let mut s = Scheduler {
        queue_count: 0,
        hold_count: 0,
        blocks: [std::ptr::null_mut(); NUM_IDS + 1],
        list: std::ptr::null_mut(),
        current: std::ptr::null_mut(),
        current_id: 0,
    };
    unsafe {
        let add = |s: &mut Scheduler, arena: &mut Arena, id: i32, pri: i32, q: *mut Packet, task: *mut dyn Task| {
            let t = arena.tcb(s.list, id, pri, q, task);
            s.list = t;
            s.current = t;
            s.blocks[id as usize] = t;
        };

        let idle = arena.task(Box::new(IdleTask { control: 1, count: COUNT }));
        add(&mut s, &mut arena, I_IDLE, 0, std::ptr::null_mut(), idle);
        (*s.blocks[I_IDLE as usize]).state = STATE_RUNNING;

        let mut wq = arena.packet(std::ptr::null_mut(), I_WORK, K_WORK);
        wq = arena.packet(wq, I_WORK, K_WORK);
        let worker = arena.task(Box::new(WorkerTask { dest: I_HANDLERA, counter: 0 }));
        add(&mut s, &mut arena, I_WORK, 1000, wq, worker);

        let mut qa = arena.packet(std::ptr::null_mut(), I_DEVA, K_DEV);
        qa = arena.packet(qa, I_DEVA, K_DEV);
        qa = arena.packet(qa, I_DEVA, K_DEV);
        let ha = arena.task(Box::new(HandlerTask { work: std::ptr::null_mut(), device: std::ptr::null_mut() }));
        add(&mut s, &mut arena, I_HANDLERA, 2000, qa, ha);

        let mut qb = arena.packet(std::ptr::null_mut(), I_DEVB, K_DEV);
        qb = arena.packet(qb, I_DEVB, K_DEV);
        qb = arena.packet(qb, I_DEVB, K_DEV);
        let hb = arena.task(Box::new(HandlerTask { work: std::ptr::null_mut(), device: std::ptr::null_mut() }));
        add(&mut s, &mut arena, I_HANDLERB, 3000, qb, hb);

        let da = arena.task(Box::new(DeviceTask { pending: std::ptr::null_mut() }));
        add(&mut s, &mut arena, I_DEVA, 4000, std::ptr::null_mut(), da);
        let db = arena.task(Box::new(DeviceTask { pending: std::ptr::null_mut() }));
        add(&mut s, &mut arena, I_DEVB, 5000, std::ptr::null_mut(), db);

        s.schedule();
    }
    (s.queue_count, s.hold_count)
}

fn main() {
    let iters: i32 = std::env::args().nth(1).and_then(|s| s.parse().ok()).unwrap_or(100);
    let (mut q, mut h) = (0i64, 0i64);
    for _ in 0..iters {
        let (a, b) = run_once();
        q = a;
        h = b;
    }
    println!("queue {} hold {}", q, h);
}
