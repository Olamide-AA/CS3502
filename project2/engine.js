const Engine = (() => {
  /* ---------- helpers ---------- */

  function clone(procs) {
    return procs.map((p) => ({ ...p }));
  }

  function finish(procs) {
    for (const p of procs) {
      p.turnaround = p.completion - p.arrival;
      p.waiting = p.turnaround - p.burst;
    }
    return procs;
  }

  /* Merge back-to-back Gantt segments of the same process
     (unit-time simulation produces many 1-unit slices). */
  function mergeSegments(schedule) {
    const merged = [];
    for (const seg of schedule) {
      const last = merged[merged.length - 1];
      if (last && last.id === seg.id && last.end === seg.start) {
        last.end = seg.end;
      } else {
        merged.push({ ...seg });
      }
    }
    return merged;
  }

  /* Shared skeleton for all NON-preemptive algorithms.
     `pick(readyList, now)` decides who runs next - that one
     line is the entire difference between FCFS, SJF,
     Priority, and HRRN. */
  function nonPreemptive(procs, pick) {
    const pending = clone(procs);
    const done = [];
    const schedule = [];
    let t = 0;

    while (pending.length) {
      const ready = pending.filter((p) => p.arrival <= t);
      if (ready.length === 0) {
        // CPU idle: jump to the next arrival
        t = Math.min(...pending.map((p) => p.arrival));
        continue;
      }
      const p = pick(ready, t);
      p.response = t - p.arrival;
      schedule.push({ id: p.id, start: t, end: t + p.burst });
      t += p.burst;
      p.completion = t;
      pending.splice(pending.indexOf(p), 1);
      done.push(p);
    }
    return { schedule, procs: finish(done) };
  }

  /* ---------- the six algorithms ---------- */

  // First Come First Served: earliest arrival wins.
  function fcfs(procs) {
    return nonPreemptive(procs, (ready) =>
      ready.reduce((a, b) => (b.arrival < a.arrival ? b : a)),
    );
  }

  // Shortest Job First (non-preemptive): smallest burst wins.
  function sjf(procs) {
    return nonPreemptive(procs, (ready) =>
      ready.reduce((a, b) => (b.burst < a.burst ? b : a)),
    );
  }

  // Priority (non-preemptive): LOWEST priority number wins.
  function priority(procs) {
    return nonPreemptive(procs, (ready) =>
      ready.reduce((a, b) => (b.priority < a.priority ? b : a)),
    );
  }

  // Highest Response Ratio Next: max (wait + burst) / burst.
  // The ratio grows the longer a process waits, so long jobs
  // eventually win
  function hrrn(procs) {
    return nonPreemptive(procs, (ready, now) =>
      ready.reduce((a, b) => {
        const ra = (now - a.arrival + a.burst) / a.burst;
        const rb = (now - b.arrival + b.burst) / b.burst;
        return rb > ra ? b : a;
      }),
    );
  }

  // Round Robin: FIFO queue, each process runs at most
  // `quantum` units then goes to the back of the queue.
  function roundRobin(procs, quantum) {
    const ps = clone(procs).map((p) => ({
      ...p,
      remaining: p.burst,
      response: null,
    }));
    const byArrival = [...ps].sort(
      (a, b) =>
        a.arrival - b.arrival || String(a.id).localeCompare(String(b.id)),
    );
    const queue = [];
    const schedule = [];
    let i = 0,
      t = 0;

    const admit = (time) => {
      while (i < byArrival.length && byArrival[i].arrival <= time) {
        queue.push(byArrival[i++]);
      }
    };

    admit(t);
    while (queue.length > 0 || i < byArrival.length) {
      if (queue.length === 0) {
        t = byArrival[i].arrival; // idle gap
        admit(t);
      }
      const p = queue.shift();
      if (p.response === null) p.response = t - p.arrival;
      const run = Math.min(quantum, p.remaining);
      schedule.push({ id: p.id, start: t, end: t + run });
      t += run;
      p.remaining -= run;
      admit(t); // arrivals during the slice queue BEFORE the preempted process
      if (p.remaining > 0) queue.push(p);
      else p.completion = t;
    }
    return { schedule: mergeSegments(schedule), procs: finish(ps) };
  }

  // Shortest Remaining Time First: preemptive SJF.
  // Unit-time simulation - at every time unit, whichever
  // arrived process has the least remaining work runs.
  function srtf(procs) {
    const ps = clone(procs).map((p) => ({
      ...p,
      remaining: p.burst,
      response: null,
    }));
    const schedule = [];
    let t = 0,
      completed = 0;

    while (completed < ps.length) {
      const ready = ps.filter((p) => p.arrival <= t && p.remaining > 0);
      if (ready.length === 0) {
        t = Math.min(
          ...ps.filter((p) => p.remaining > 0).map((p) => p.arrival),
        );
        continue;
      }
      const p = ready.reduce((a, b) =>
        b.remaining < a.remaining ||
        (b.remaining === a.remaining && b.arrival < a.arrival)
          ? b
          : a,
      );
      if (p.response === null) p.response = t - p.arrival;
      schedule.push({ id: p.id, start: t, end: t + 1 });
      p.remaining -= 1;
      t += 1;
      if (p.remaining === 0) {
        p.completion = t;
        completed++;
      }
    }
    return { schedule: mergeSegments(schedule), procs: finish(ps) };
  }

  /* ---------- metrics ---------- */

  function metrics(result) {
    const { procs } = result;
    const n = procs.length;
    const totalBurst = procs.reduce((s, p) => s + p.burst, 0);
    const makespan = Math.max(...procs.map((p) => p.completion));
    return {
      awt: procs.reduce((s, p) => s + p.waiting, 0) / n,
      att: procs.reduce((s, p) => s + p.turnaround, 0) / n,
      art: procs.reduce((s, p) => s + p.response, 0) / n,
      utilization: (totalBurst / makespan) * 100,
      throughput: n / makespan,
      makespan,
    };
  }

  /* ---------- workload generator ---------- */

  // Deterministic RNG (mulberry32) so a seed always produces
  // the same workload
  function rng(seed) {
    let a = seed >>> 0;
    return () => {
      a |= 0;
      a = (a + 0x6d2b79f5) | 0;
      let x = Math.imul(a ^ (a >>> 15), 1 | a);
      x = (x + Math.imul(x ^ (x >>> 7), 61 | x)) ^ x;
      return ((x ^ (x >>> 14)) >>> 0) / 4294967296;
    };
  }

  function generate(count, type, seed) {
    const rand = rng(seed);
    const between = (lo, hi) => lo + Math.floor(rand() * (hi - lo + 1));
    const procs = [];
    for (let k = 1; k <= count; k++) {
      let burst;
      if (type === "cpu")
        burst = between(15, 40); // long bursts
      else if (type === "io")
        burst = between(1, 6); // short bursts
      else burst = rand() < 0.7 ? between(1, 6) : between(20, 40); // mixed
      procs.push({
        id: "P" + k,
        arrival: between(0, Math.max(4, Math.floor(count * 1.5))),
        burst,
        priority: between(1, 5),
      });
    }
    return procs;
  }

  const ALGORITHMS = {
    FCFS: { run: (p) => fcfs(p), usesQuantum: false, isNew: false },
    SJF: { run: (p) => sjf(p), usesQuantum: false, isNew: false },
    Priority: { run: (p) => priority(p), usesQuantum: false, isNew: false },
    RR: { run: (p, q) => roundRobin(p, q), usesQuantum: true, isNew: false },
    SRTF: { run: (p) => srtf(p), usesQuantum: false, isNew: true },
    HRRN: { run: (p) => hrrn(p), usesQuantum: false, isNew: true },
  };

  return {
    fcfs,
    sjf,
    priority,
    hrrn,
    roundRobin,
    srtf,
    metrics,
    generate,
    ALGORITHMS,
    mergeSegments,
  };
})();

/* ---------- node test harness (ignored by the browser) ---------- */
if (typeof module !== "undefined") {
  const spec = [
    { id: "P1", arrival: 0, burst: 8, priority: 3 },
    { id: "P2", arrival: 1, burst: 4, priority: 1 },
    { id: "P3", arrival: 2, burst: 9, priority: 4 },
    { id: "P4", arrival: 3, burst: 5, priority: 2 },
  ];
  const close = (a, b) => Math.abs(a - b) < 1e-9;
  const check = (name, got, want) => {
    const ok = close(got, want);
    console.log(
      `${ok ? "PASS" : "FAIL"} ${name}: AWT ${got} (expected ${want})`,
    );
    if (!ok) process.exitCode = 1;
  };

  check("FCFS", Engine.metrics(Engine.fcfs(spec)).awt, 8.75);
  check("SJF", Engine.metrics(Engine.sjf(spec)).awt, 7.75);
  check("Prio", Engine.metrics(Engine.priority(spec)).awt, 7.75);
  check("RR q2", Engine.metrics(Engine.roundRobin(spec, 2)).awt, 12.75);
  check("SRTF", Engine.metrics(Engine.srtf(spec)).awt, 6.5);
  check("HRRN", Engine.metrics(Engine.hrrn(spec)).awt, 7.75);

  for (const [name, alg] of Object.entries(Engine.ALGORITHMS)) {
    const r = alg.run(spec, 2);
    const scheduled = r.schedule.reduce(
      (s, seg) => s + (seg.end - seg.start),
      0,
    );
    const total = spec.reduce((s, p) => s + p.burst, 0);
    if (
      scheduled !== total ||
      r.procs.some((p) => p.completion === undefined)
    ) {
      console.log(`FAIL ${name}: conservation check`);
      process.exitCode = 1;
    }
  }

  // large-workload smoke test: 150 mixed processes through everything
  const big = Engine.generate(150, "mixed", 42);
  for (const [name, alg] of Object.entries(Engine.ALGORITHMS)) {
    const m = Engine.metrics(alg.run(big, 3));
    console.log(
      `OK   ${name} x150: AWT ${m.awt.toFixed(1)}, util ${m.utilization.toFixed(1)}%`,
    );
  }
}
