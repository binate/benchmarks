// richards — see ../c/richards.c for the algorithm. Task is an interface; each
// task type is a distinct implementer, so run() dispatches dynamically.
package main

import (
	"fmt"
	"os"
	"strconv"
)

const (
	iIdle     = 1
	iWork     = 2
	iHandlerA = 3
	iHandlerB = 4
	iDevA     = 5
	iDevB     = 6
	numIDs    = 6

	kDev  = 1000
	kWork = 1001

	stateRunning           = 0
	stateRunnable          = 1
	stateSuspended         = 2
	stateHeld              = 4
	stateSuspendedRunnable = 3

	dataSize = 4
	count    = 1000
)

type packet struct {
	link *packet
	id   int
	kind int
	a1   int
	a2   [dataSize]int
}

func (p *packet) addTo(list *packet) *packet {
	p.link = nil
	if list == nil {
		return p
	}
	q := list
	for q.link != nil {
		q = q.link
	}
	q.link = p
	return list
}

type tcb struct {
	link     *tcb
	id       int
	priority int
	input    *packet
	state    int
	task     task
}

func (t *tcb) heldOrSuspended() bool {
	return t.state&stateHeld != 0 || t.state == stateSuspended
}

type scheduler struct {
	queueCount int
	holdCount  int
	blocks     [numIDs + 1]*tcb
	list       *tcb
	current    *tcb
	currentID  int
}

func (s *scheduler) hold() *tcb {
	s.holdCount++
	s.current.state |= stateHeld
	return s.current.link
}
func (s *scheduler) suspend() *tcb {
	s.current.state |= stateSuspended
	return s.current
}
func (s *scheduler) release(id int) *tcb {
	t := s.blocks[id]
	if t == nil {
		return nil
	}
	t.state &^= stateHeld
	if t.priority > s.current.priority {
		return t
	}
	return s.current
}
func (s *scheduler) checkPriorityAdd(t *tcb, p *packet) *tcb {
	if t.input == nil {
		t.input = p
		t.state |= stateRunnable
		if t.priority > s.current.priority {
			return t
		}
	} else {
		t.input = p.addTo(t.input)
	}
	return s.current
}
func (s *scheduler) queue(p *packet) *tcb {
	t := s.blocks[p.id]
	if t == nil {
		return nil
	}
	s.queueCount++
	p.link = nil
	p.id = s.currentID
	return s.checkPriorityAdd(t, p)
}
func (s *scheduler) addTask(id, priority int, q *packet, tk task) {
	t := &tcb{link: s.list, id: id, priority: priority, input: q, task: tk}
	if q == nil {
		t.state = stateSuspended
	} else {
		t.state = stateSuspendedRunnable
	}
	s.list = t
	s.current = t
	s.blocks[id] = t
}
func (s *scheduler) schedule() {
	s.current = s.list
	for s.current != nil {
		if s.current.heldOrSuspended() {
			s.current = s.current.link
			continue
		}
		s.currentID = s.current.id
		var p *packet
		if s.current.state == stateSuspendedRunnable {
			p = s.current.input
			s.current.input = p.link
			if s.current.input == nil {
				s.current.state = stateRunning
			} else {
				s.current.state = stateRunnable
			}
		}
		cur := s.current
		s.current = cur.task.run(s, p)
	}
}

type task interface {
	run(s *scheduler, p *packet) *tcb
}

type idleTask struct {
	control, count int
}

func (t *idleTask) run(s *scheduler, p *packet) *tcb {
	t.count--
	if t.count == 0 {
		return s.hold()
	}
	if t.control&1 == 0 {
		t.control >>= 1
		return s.release(iDevA)
	}
	t.control = (t.control >> 1) ^ 53256
	return s.release(iDevB)
}

type deviceTask struct{ pending *packet }

func (t *deviceTask) run(s *scheduler, p *packet) *tcb {
	if p == nil {
		if t.pending == nil {
			return s.suspend()
		}
		q := t.pending
		t.pending = nil
		return s.queue(q)
	}
	t.pending = p
	return s.hold()
}

type workerTask struct{ dest, counter int }

func (t *workerTask) run(s *scheduler, p *packet) *tcb {
	if p == nil {
		return s.suspend()
	}
	if t.dest == iHandlerA {
		t.dest = iHandlerB
	} else {
		t.dest = iHandlerA
	}
	p.id = t.dest
	p.a1 = 0
	for i := 0; i < dataSize; i++ {
		t.counter++
		if t.counter > 26 {
			t.counter = 1
		}
		p.a2[i] = 65 + t.counter - 1
	}
	return s.queue(p)
}

type handlerTask struct{ work, device *packet }

func (t *handlerTask) run(s *scheduler, p *packet) *tcb {
	if p != nil {
		if p.kind == kWork {
			t.work = p.addTo(t.work)
		} else {
			t.device = p.addTo(t.device)
		}
	}
	if t.work != nil {
		c := t.work.a1
		if c < dataSize {
			if t.device != nil {
				v := t.device
				t.device = t.device.link
				v.a1 = t.work.a2[c]
				t.work.a1 = c + 1
				return s.queue(v)
			}
		} else {
			v := t.work
			t.work = t.work.link
			return s.queue(v)
		}
	}
	return s.suspend()
}

func runOnce() (int, int) {
	s := &scheduler{}
	idle := &idleTask{control: 1, count: count}
	s.addTask(iIdle, 0, nil, idle)
	s.blocks[iIdle].state = stateRunning

	wq := (&packet{id: iWork, kind: kWork})
	wq = &packet{link: wq, id: iWork, kind: kWork}
	s.addTask(iWork, 1000, wq, &workerTask{dest: iHandlerA})

	qa := &packet{id: iDevA, kind: kDev}
	qa = &packet{link: qa, id: iDevA, kind: kDev}
	qa = &packet{link: qa, id: iDevA, kind: kDev}
	s.addTask(iHandlerA, 2000, qa, &handlerTask{})

	qb := &packet{id: iDevB, kind: kDev}
	qb = &packet{link: qb, id: iDevB, kind: kDev}
	qb = &packet{link: qb, id: iDevB, kind: kDev}
	s.addTask(iHandlerB, 3000, qb, &handlerTask{})

	s.addTask(iDevA, 4000, nil, &deviceTask{})
	s.addTask(iDevB, 5000, nil, &deviceTask{})

	s.schedule()
	return s.queueCount, s.holdCount
}

func main() {
	iters := 100
	if len(os.Args) > 1 {
		if v, err := strconv.Atoi(os.Args[1]); err == nil {
			iters = v
		}
	}
	q, h := 0, 0
	for i := 0; i < iters; i++ {
		q, h = runOnce()
	}
	fmt.Printf("queue %d hold %d\n", q, h)
}
