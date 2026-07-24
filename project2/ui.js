const $ = (sel) => document.querySelector(sel);
const el = (tag, cls, text) => {
  const n = document.createElement(tag);
  if (cls) n.className = cls;
  if (text !== undefined) n.textContent = text;
  return n;
};

/* Categorical colors for Gantt traces (cycled for big workloads) */
const TRACE_COLORS = [
  "#2E6E8E",
  "#C2571A",
  "#4E7A3A",
  "#7A4E9E",
  "#B03A48",
  "#8A6D1F",
  "#3A8A80",
  "#94512F",
  "#5A6EA8",
  "#6E8E2E",
  "#A8447A",
  "#447AA8",
];
const colorFor = (idx) => TRACE_COLORS[idx % TRACE_COLORS.length];

/* ---------- process table ---------- */

let rowCount = 0;

function addRow(p) {
  rowCount++;
  const tr = el("tr");
  const idTd = el("td");
  idTd.appendChild(
    Object.assign(el("input"), {
      value: p ? p.id : "P" + rowCount,
      className: "cell id",
    }),
  );
  tr.appendChild(idTd);
  for (const [field, val, min] of [
    ["arrival", p ? p.arrival : 0, 0],
    ["burst", p ? p.burst : 1, 1],
    ["priority", p ? p.priority : 1, 1],
  ]) {
    const td = el("td");
    const inp = Object.assign(el("input"), {
      type: "number",
      value: val,
      min,
      className: "cell " + field,
    });
    td.appendChild(inp);
    tr.appendChild(td);
  }
  const delTd = el("td");
  const del = el("button", "rowdel", "\u00d7");
  del.title = "Remove process";
  del.onclick = () => {
    tr.remove();
  };
  delTd.appendChild(del);
  tr.appendChild(delTd);
  $("#ptable tbody").appendChild(tr);
}

function readProcesses() {
  const procs = [];
  for (const tr of $("#ptable tbody").rows) {
    const [id, arrival, burst, priority] = [
      ...tr.querySelectorAll("input"),
    ].map((i) => i.value);
    if (!id) continue;
    procs.push({
      id: id.trim(),
      arrival: Math.max(0, parseInt(arrival, 10) || 0),
      burst: Math.max(1, parseInt(burst, 10) || 1),
      priority: Math.max(1, parseInt(priority, 10) || 1),
    });
  }
  return procs;
}

function loadProcesses(procs) {
  $("#ptable tbody").innerHTML = "";
  rowCount = 0;
  procs.forEach((p) => addRow(p));
}

let lastResults = null; // kept for CSV export

function runAll() {
  const procs = readProcesses();
  const status = $("#status");
  if (procs.length === 0) {
    status.textContent = "Add at least one process, or generate a workload.";
    return;
  }
  const ids = new Set();
  for (const p of procs) {
    if (ids.has(p.id)) {
      status.textContent = `Duplicate process ID "${p.id}" - IDs must be unique.`;
      return;
    }
    ids.add(p.id);
  }
  const quantum = Math.max(1, parseInt($("#quantum").value, 10) || 2);
  const results = {};
  for (const [name, alg] of Object.entries(Engine.ALGORITHMS)) {
    const r = alg.run(procs, quantum);
    results[name] = { ...r, metrics: Engine.metrics(r) };
  }
  lastResults = { procs, quantum, results };
  status.textContent =
    `Ran 6 algorithms on ${procs.length} processes` +
    ` (RR quantum = ${quantum}).`;
  renderComparison(results);
  renderDetails(results, procs);
  $("#results").hidden = false;
  $("#exportBtn").disabled = false;
}

function renderComparison(results) {
  const metricDefs = [
    ["awt", "Avg waiting", "min"],
    ["att", "Avg turnaround", "min"],
    ["art", "Avg response", "min"],
    ["utilization", "CPU util %", "max"],
    ["throughput", "Throughput", "max"],
  ];
  const names = Object.keys(results);
  const table = $("#cmp");
  table.innerHTML = "";

  const head = el("tr");
  head.appendChild(el("th", null, "Metric"));
  for (const n of names) {
    const th = el("th", null, n);
    if (Engine.ALGORITHMS[n].isNew) th.appendChild(el("span", "newtag", "NEW"));
    head.appendChild(th);
  }
  table.appendChild(head);

  for (const [key, label, best] of metricDefs) {
    const vals = names.map((n) => results[n].metrics[key]);
    const target = best === "min" ? Math.min(...vals) : Math.max(...vals);
    const tr = el("tr");
    tr.appendChild(el("th", null, label));
    vals.forEach((v) => {
      const td = el(
        "td",
        Math.abs(v - target) < 1e-9 ? "best" : null,
        key === "throughput" ? v.toFixed(4) : v.toFixed(2),
      );
      tr.appendChild(td);
    });
    table.appendChild(tr);
  }
}

function renderDetails(results, procs) {
  const tabs = $("#tabs");
  const panes = $("#panes");
  tabs.innerHTML = "";
  panes.innerHTML = "";
  const colorIndex = {};
  procs.forEach((p, i) => (colorIndex[p.id] = i));

  Object.entries(results).forEach(([name, r], k) => {
    const btn = el("button", "tab" + (k === 0 ? " active" : ""), name);
    btn.onclick = () => {
      [...tabs.children].forEach((b) => b.classList.remove("active"));
      [...panes.children].forEach((p) => (p.hidden = true));
      btn.classList.add("active");
      pane.hidden = false;
    };
    tabs.appendChild(btn);

    const pane = el("div", "pane");
    pane.hidden = k !== 0;
    pane.appendChild(gantt(r.schedule, colorIndex));
    pane.appendChild(perProcessTable(r.procs));
    panes.appendChild(pane);
  });
}

/* The signature element: Gantt as a logic-analyzer style trace
   with a time ruler underneath. */
function gantt(schedule, colorIndex) {
  const wrap = el("div", "gantt");
  const makespan = schedule[schedule.length - 1].end;
  const lane = el("div", "lane");

  let cursor = 0;
  for (const seg of schedule) {
    if (seg.start > cursor) {
      // idle gap
      const idle = el("div", "seg idle");
      idle.style.flexGrow = seg.start - cursor;
      idle.title = `idle ${cursor}\u2013${seg.start}`;
      lane.appendChild(idle);
    }
    const d = el("div", "seg");
    d.style.flexGrow = seg.end - seg.start;
    d.style.background = colorFor(colorIndex[seg.id] ?? 0);
    d.title = `${seg.id}: ${seg.start}\u2013${seg.end}`;
    if ((seg.end - seg.start) / makespan > 0.03) d.textContent = seg.id;
    lane.appendChild(d);
    cursor = seg.end;
  }
  wrap.appendChild(lane);

  const ruler = el("div", "ruler");
  const step = Math.max(1, Math.ceil(makespan / 12));
  for (let t = 0; t <= makespan; t += step) {
    const tick = el("span", "tick", t);
    tick.style.left = (t / makespan) * 100 + "%";
    ruler.appendChild(tick);
  }
  const last = el("span", "tick end", makespan);
  ruler.appendChild(last);
  wrap.appendChild(ruler);
  return wrap;
}

function perProcessTable(procs) {
  const t = el("table", "proctable");
  const head = el("tr");
  for (const h of [
    "ID",
    "Arrival",
    "Burst",
    "Priority",
    "Completion",
    "Turnaround",
    "Waiting",
    "Response",
  ])
    head.appendChild(el("th", null, h));
  t.appendChild(head);
  const sorted = [...procs].sort((a, b) =>
    String(a.id).localeCompare(String(b.id), undefined, { numeric: true }),
  );
  for (const p of sorted) {
    const tr = el("tr");
    for (const v of [
      p.id,
      p.arrival,
      p.burst,
      p.priority,
      p.completion,
      p.turnaround,
      p.waiting,
      p.response,
    ])
      tr.appendChild(el("td", null, v));
    t.appendChild(tr);
  }
  const scroller = el("div", "scroll");
  scroller.appendChild(t);
  return scroller;
}

/* ---------- CSV export---------- */

function exportCsv() {
  if (!lastResults) return;
  const { procs, quantum, results } = lastResults;
  const lines = [];
  lines.push(`# ${procs.length} processes, RR quantum ${quantum}`);
  lines.push(
    "Algorithm,AvgWaiting,AvgTurnaround,AvgResponse,CPUUtil,Throughput",
  );
  for (const [name, r] of Object.entries(results)) {
    const m = r.metrics;
    lines.push(
      [
        name,
        m.awt.toFixed(3),
        m.att.toFixed(3),
        m.art.toFixed(3),
        m.utilization.toFixed(2),
        m.throughput.toFixed(5),
      ].join(","),
    );
  }
  const blob = new Blob([lines.join("\n")], { type: "text/csv" });
  const a = el("a");
  a.href = URL.createObjectURL(blob);
  a.download = "scheduler-metrics.csv";
  a.click();
  URL.revokeObjectURL(a.href);
}

/* ---------- wiring ---------- */

document.addEventListener("DOMContentLoaded", () => {
  $("#genBtn").onclick = () => {
    const count = Math.max(
      1,
      Math.min(500, parseInt($("#count").value, 10) || 10),
    );
    const type = $("#wtype").value;
    const seed = parseInt($("#seed").value, 10) || 1;
    loadProcesses(Engine.generate(count, type, seed));
    $("#status").textContent = `Generated ${count} ${
      type === "cpu" ? "CPU-bound" : type === "io" ? "I/O-bound" : "mixed"
    } processes (seed ${seed}).`;
  };
  $("#addBtn").onclick = () => addRow();
  $("#clearBtn").onclick = () => {
    loadProcesses([]);
    $("#results").hidden = true;
  };
  $("#runBtn").onclick = runAll;
  $("#exportBtn").onclick = exportCsv;

  // start with the assignment spec's example workload
  loadProcesses([
    { id: "P1", arrival: 0, burst: 8, priority: 3 },
    { id: "P2", arrival: 1, burst: 4, priority: 1 },
    { id: "P3", arrival: 2, burst: 9, priority: 4 },
    { id: "P4", arrival: 3, burst: 5, priority: 2 },
  ]);
});
