/* richards: the classic Richards operating-system task-scheduler benchmark. A
 * scheduler runs a chain of tasks (idle, worker, two handlers, two devices),
 * each a different type dispatched polymorphically as packets flow between them.
 * This is the suite's dispatch probe: each language uses its natural dynamic
 * dispatch (interfaces / virtual methods / trait objects / function pointers).
 *
 * The scheduler is passed to each task's run() rather than stored on the task,
 * so the object graph has no task<->scheduler cycle (Binate is reference-counted
 * and would not collect one). The run reproduces the standard verification
 * counts; all implementations must print the same line. */
#include <stdio.h>
#include <stdlib.h>

#define I_IDLE 1
#define I_WORK 2
#define I_HANDLERA 3
#define I_HANDLERB 4
#define I_DEVA 5
#define I_DEVB 6
#define NUMBER_OF_IDS 6

#define K_DEV 1000
#define K_WORK 1001

#define STATE_RUNNING 0
#define STATE_RUNNABLE 1
#define STATE_SUSPENDED 2
#define STATE_HELD 4
#define STATE_SUSPENDED_RUNNABLE 3

#define DATA_SIZE 4
#define COUNT 1000

typedef struct Packet {
    struct Packet *link;
    int id;
    int kind;
    int a1;
    int a2[DATA_SIZE];
} Packet;

typedef struct Scheduler Scheduler;
typedef struct TCB TCB;

/* A task: a dispatch function plus generic state used differently per type. */
typedef struct Task {
    TCB *(*run)(struct Task *self, Scheduler *s, Packet *pkt);
    int i1, i2;      /* idle: i1=control i2=count; worker: i1=dest i2=counter */
    Packet *p1, *p2; /* device: p1=pending; handler: p1=work p2=device queues */
} Task;

struct TCB {
    TCB *link;
    int id;
    int priority;
    Packet *input;
    int state;
    Task *task;
};

struct Scheduler {
    long queueCount;
    long holdCount;
    TCB *blocks[NUMBER_OF_IDS + 1];
    TCB *list;
    TCB *current;
    int currentId;
};

/* Track every allocation of one iteration so it can be freed by original
 * pointer — the live graph rewrites packet links, so walking it to free would
 * double-free. */
static Packet *g_pkts[16];
static int g_np;
static TCB *g_tcbs[8];
static int g_nt;
static Task *g_tasks[8];
static int g_nta;

static Packet *packetNew(Packet *link, int id, int kind) {
    Packet *p = malloc(sizeof(Packet));
    p->link = link; p->id = id; p->kind = kind; p->a1 = 0;
    for (int i = 0; i < DATA_SIZE; i++) p->a2[i] = 0;
    g_pkts[g_np++] = p;
    return p;
}
static Packet *packetAddTo(Packet *self, Packet *list) {
    self->link = NULL;
    if (list == NULL) return self;
    Packet *p = list;
    while (p->link != NULL) p = p->link;
    p->link = self;
    return list;
}

static int isHeldOrSuspended(TCB *t) {
    return (t->state & STATE_HELD) != 0 || t->state == STATE_SUSPENDED;
}

/* scheduler primitives */
static TCB *schedHold(Scheduler *s) {
    s->holdCount++;
    s->current->state |= STATE_HELD;
    return s->current->link;
}
static TCB *schedSuspend(Scheduler *s) {
    s->current->state |= STATE_SUSPENDED;
    return s->current;
}
static TCB *schedRelease(Scheduler *s, int id) {
    TCB *t = s->blocks[id];
    if (t == NULL) return NULL;
    t->state &= ~STATE_HELD;
    if (t->priority > s->current->priority) return t;
    return s->current;
}
static TCB *tcbCheckPriorityAdd(TCB *self, TCB *task, Packet *packet, Scheduler *s) {
    if (self->input == NULL) {
        self->input = packet;
        self->state |= STATE_RUNNABLE;
        if (self->priority > task->priority) return self;
    } else {
        self->input = packetAddTo(packet, self->input);
    }
    return task;
}
static TCB *schedQueue(Scheduler *s, Packet *packet) {
    TCB *t = s->blocks[packet->id];
    if (t == NULL) return NULL;
    s->queueCount++;
    packet->link = NULL;
    packet->id = s->currentId;
    return tcbCheckPriorityAdd(t, s->current, packet, s);
}

/* task run functions (the polymorphic dispatch targets) */
static TCB *idleRun(Task *self, Scheduler *s, Packet *pkt) {
    (void)pkt;
    self->i2--; /* count */
    if (self->i2 == 0) return schedHold(s);
    if ((self->i1 & 1) == 0) {
        self->i1 = self->i1 >> 1;
        return schedRelease(s, I_DEVA);
    }
    self->i1 = (self->i1 >> 1) ^ 53256;
    return schedRelease(s, I_DEVB);
}
static TCB *deviceRun(Task *self, Scheduler *s, Packet *pkt) {
    if (pkt == NULL) {
        if (self->p1 == NULL) return schedSuspend(s);
        Packet *p = self->p1;
        self->p1 = NULL;
        return schedQueue(s, p);
    }
    self->p1 = pkt;
    return schedHold(s);
}
static TCB *workerRun(Task *self, Scheduler *s, Packet *pkt) {
    if (pkt == NULL) return schedSuspend(s);
    self->i1 = (self->i1 == I_HANDLERA) ? I_HANDLERB : I_HANDLERA;
    pkt->id = self->i1;
    pkt->a1 = 0;
    for (int i = 0; i < DATA_SIZE; i++) {
        self->i2++;
        if (self->i2 > 26) self->i2 = 1;
        pkt->a2[i] = 65 + self->i2 - 1;
    }
    return schedQueue(s, pkt);
}
static TCB *handlerRun(Task *self, Scheduler *s, Packet *pkt) {
    if (pkt != NULL) {
        if (pkt->kind == K_WORK) self->p1 = packetAddTo(pkt, self->p1);
        else self->p2 = packetAddTo(pkt, self->p2);
    }
    if (self->p1 != NULL) {
        int count = self->p1->a1;
        if (count < DATA_SIZE) {
            if (self->p2 != NULL) {
                Packet *v = self->p2;
                self->p2 = self->p2->link;
                v->a1 = self->p1->a2[count];
                self->p1->a1 = count + 1;
                return schedQueue(s, v);
            }
        } else {
            Packet *v = self->p1;
            self->p1 = self->p1->link;
            return schedQueue(s, v);
        }
    }
    return schedSuspend(s);
}

static Task *taskNew(TCB *(*run)(Task *, Scheduler *, Packet *)) {
    Task *t = malloc(sizeof(Task));
    t->run = run; t->i1 = 0; t->i2 = 0; t->p1 = NULL; t->p2 = NULL;
    g_tasks[g_nta++] = t;
    return t;
}
static void addTask(Scheduler *s, int id, int priority, Packet *queue, Task *task) {
    TCB *t = malloc(sizeof(TCB));
    t->link = s->list; t->id = id; t->priority = priority; t->input = queue;
    t->state = queue == NULL ? STATE_SUSPENDED : STATE_SUSPENDED_RUNNABLE;
    t->task = task;
    s->list = t;
    s->current = t;
    s->blocks[id] = t;
    g_tcbs[g_nt++] = t;
}
static void schedule(Scheduler *s) {
    s->current = s->list;
    while (s->current != NULL) {
        if (isHeldOrSuspended(s->current)) {
            s->current = s->current->link;
        } else {
            s->currentId = s->current->id;
            /* TCB.run: pick a packet if runnable, then dispatch to the task */
            Packet *packet = NULL;
            if (s->current->state == STATE_SUSPENDED_RUNNABLE) {
                packet = s->current->input;
                s->current->input = packet->link;
                s->current->state = s->current->input == NULL ? STATE_RUNNING : STATE_RUNNABLE;
            }
            TCB *cur = s->current;
            s->current = cur->task->run(cur->task, s, packet);
        }
    }
}

/* Free everything an iteration allocated (packets recirculate a fixed set). */
static void runOnce(long *outQ, long *outH) {
    Scheduler s;
    s.queueCount = 0; s.holdCount = 0; s.list = NULL; s.current = NULL; s.currentId = 0;
    for (int i = 0; i <= NUMBER_OF_IDS; i++) s.blocks[i] = NULL;
    g_np = 0; g_nt = 0; g_nta = 0;

    Task *idle = taskNew(idleRun); idle->i1 = 1; idle->i2 = COUNT;
    addTask(&s, I_IDLE, 0, NULL, idle);
    s.blocks[I_IDLE]->state = STATE_RUNNING; /* the idle task starts running */

    Packet *wq = packetNew(NULL, I_WORK, K_WORK);
    wq = packetNew(wq, I_WORK, K_WORK);
    Task *worker = taskNew(workerRun); worker->i1 = I_HANDLERA; worker->i2 = 0;
    addTask(&s, I_WORK, 1000, wq, worker);

    Packet *qa = packetNew(NULL, I_DEVA, K_DEV);
    qa = packetNew(qa, I_DEVA, K_DEV);
    qa = packetNew(qa, I_DEVA, K_DEV);
    addTask(&s, I_HANDLERA, 2000, qa, taskNew(handlerRun));

    Packet *qb = packetNew(NULL, I_DEVB, K_DEV);
    qb = packetNew(qb, I_DEVB, K_DEV);
    qb = packetNew(qb, I_DEVB, K_DEV);
    addTask(&s, I_HANDLERB, 3000, qb, taskNew(handlerRun));

    addTask(&s, I_DEVA, 4000, NULL, taskNew(deviceRun));
    addTask(&s, I_DEVB, 5000, NULL, taskNew(deviceRun));

    schedule(&s);

    *outQ = s.queueCount; *outH = s.holdCount;

    /* free by original allocation — never by walking the live graph */
    for (int i = 0; i < g_np; i++) free(g_pkts[i]);
    for (int i = 0; i < g_nta; i++) free(g_tasks[i]);
    for (int i = 0; i < g_nt; i++) free(g_tcbs[i]);
}

int main(int argc, char **argv) {
    int iters = argc > 1 ? atoi(argv[1]) : 100;
    long q = 0, h = 0;
    for (int i = 0; i < iters; i++) runOnce(&q, &h);
    printf("queue %ld hold %ld\n", q, h);
    return 0;
}
